/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_selftest.c
 ***************************************************************************
 *
 * DESCRIPTION: MUT Self Test
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin initial version
 *    10/17/2007   Alexander Gorlov (gorloa) test cases for assert implementation
 *    10/23/2007   Igor Bobrovnikov (bobroi) test cases revised and extended
 *    10/07/2007   Igor Bobrovnikov (bobroi) compare files generation added
 *    01/10/2008   Igor Bobrovnikov (bobroi) 
 *                 epsilon argument for float and double compare added
 *    02/29/2008   Igor Bobrovnikov  new log format
 *    03/07/2008   Igor Bobrovnikov (bobroi) files comparing rewritten in C
 *    04/08/2008   Igor Bobrovnikov (bobroi) flexible format supported
 *    08/29/2008   Alexander Alanov (alanoa) added test cases for pointer comparison predicates
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
#include <stdarg.h>
//#include <process.h>
//#include <excpt.h>
#include <windows.h> /* Need for declaration of Sleep and DeleteFunction */

#include "mut_cpp.h" /* Include standard MUT header */
//
#include "mut_private.h"
#include "mut_log_private.h"
#include "mut_testsuite.h"
#include "simulation/Program_Options.h"
#include "simulation/arguments.h"

#include "EmcPAL_Misc.h"

#include "mut_sdk.h"
#include "mut_selftest_opt.h"
extern "C"{
#include "mut_selftest_tools.h"
void _stdcall mut_selftest_cpp_add_tests(mut_testsuite_t * root);
}
#include "EmcUTIL_CsxShell_Interface.h"

const char * const CHECKPOINT_FMT    = "CHECKPOINT: correct code execution, this string shall be printed, line %u";
const char * const EXPECTED_FMT      = "Expected assert message, it shall be printed";
const char * const UNEXPECTED_FMT    = "ERROR: unexpected assert message, it shall not be printed";
const char * const DISABLED_CODE_FMT = "ERROR: disabled code execution, this string should not be printed, line %u";

const char * const mut_fail_fmt = "Assert %s";

#include "mut_test_control.h"

#ifdef ALAMOSA_WINDOWS_ENV
#define MUT_PATH_SEP "\\"
#else
#define MUT_PATH_SEP "/"
#endif /* ALAMOSA_WINDOWS_ENV - PATHS - */

extern MUT_API Mut_test_control *mut_control;

char goldenfile_name[MAX_PATH + 1];
FILE *goldenfile = NULL;
mut_logging_t test_log_level;

FILE *commandLineCollection = NULL;

static char *mut_cmdLine = NULL;

const char * program_name;
char program[MAX_PATH + 1]; /* Saved program name */
char path_to_config[MAX_PATH + 1]; /* Path to default configuration file */

const char logging_dir[] = "logdir";
const char logging_filename[] = "%testsuite%.%ext%";

const char logging_dir_alt[] = "logdir" MUT_PATH_SEP "alt";
const char logging_filename_alt[] = "logdirname_config.txt";

char suite_logfile[MAX_PATH + 1];
char suite_logfile_short[MAX_PATH + 1];
const char * current_suite_name;
const char * suite_finish_status;

int tests_passed;
int tests_failed;
int tests_not_executed;

const char * list_tests_failed;
const char * list_tests_not_executed;

const char * requested_testsuite = NULL;
int test_run_cnt;

const char * const suite_status_default =   "testexe";
const char * const suite_status_continue =  NULL;
const char * const suite_status_test =      NULL;
const char * const suite_status_testsuite = "testsuite";
const char * const suite_status_testexe =   "testexe";

//*********************************************************************************************************************

/** \fn static void create_log_name(char * logname, const char * const dir, const char * const suite)
 *  \details constructs log file name
 *  This function is used to generate log file name for control structure.
 *  Please note, that before calling this function you need to set other
 *  structure fields: log_directory, log_type and testsuite.
 */
static void create_log_name(char * logname, const char * const dir, const char * const suite)
{
    if (!strcmp(suite, "logdirname_config"))
    {
        sprintf(logname, "%s%s%s", logging_dir, MUT_PATH_SEP, "logdirname_config.txt");
    }
    else
    {
        _snprintf(logname, MAX_PATH, "%s%s%s.txt", dir, MUT_PATH_SEP, suite);
    }
}

void golden_printf(mut_logging_t level, const char * const fmt, ...)
{
    char buf[16000];
    va_list argptr;

    if (test_log_level >= level)
    {
        va_start(argptr, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, argptr);
        va_end(argptr);

        fprintf(goldenfile, buf);
        fflush(goldenfile);
    }
}

const char * const LOG_STATUS_PASSED          = "Passed";
const char * const LOG_STATUS_FAILED          = "Failed";
const char * const LOG_STATUS_ERROR_EXCEPTION = "ERROR (EXCEPTION_EXECUTE_HANDLER in setup and test)";
const char * const LOG_STATUS_TIMEOUT         = "ERROR (Timeout in setup and test)";
const char * const LOG_STATUS_ERROR_VEH_EXCEPTION = "ERROR (EXCEPTION caught in unhandled exception handler)";

void log_test_begin_with_description(const char * const test, const char* desc)
{
    test_run_cnt++;
    if (test_log_level >= 1)
    {
        golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   TestStarted    (%2u) %s.%s: %s\n", test_run_cnt, current_suite_name, test, desc);
    }

    return;
}

void log_test_begin(const char * const test)
{
    log_test_begin_with_description(test, "@@");

    return;
}

void log_write_to_dump_file()
{
	golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      %s\n", "Writing dump file to: @@");
    return;
}

void log_exception_encounted()
{
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      Exception encountered in test\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      Exception code: 0x$\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      Exception address: 0x$\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      Exception flag: 0x$\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   MutPrintf      Exception NumberParameters: 0x$\n");
}

void log_test_end(const char * const test, const char * const status)
{
    if (test_log_level >= 1)
    {
        golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   TestFinished   (%2u) %s.%s %s\n", test_run_cnt, current_suite_name, test, status);
    }

    return;
}

void log_suite_begin(const char * suite, const char * subdir)
{
    DWORD size;
    char compname[128];
    char username[128];

    size = sizeof(compname) - 1;
    csx_p_native_system_name_get(compname, size, 0);
    
    size = sizeof(username) - 1;
    csx_p_native_user_name_get(username, size, 0);

    current_suite_name = suite;

    if (!requested_testsuite || strcmp(requested_testsuite, current_suite_name))
    {
        return;
    }

    create_log_name(suite_logfile, subdir, suite);

    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Header Iteration      @@\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Header MUTLog         Version 1.0\n");
    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Header TestSuite      %s\n", suite);
    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Header LogFile        %s\n", suite_logfile);
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Header ComputerName   %s\n", compname);
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Header UserName       %s\n", username);
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Header MutVersion     %d.%d\n", mut_version_major, mut_version_minor);
    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Header SuiteStarted   %s\n", suite);

    return;
}

void log_suite_end(void)
{
    if (!requested_testsuite || strcmp(requested_testsuite, current_suite_name))
    {
        return;
    }

    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Result SuiteFinished  Duration: $s\n");

    if (tests_failed) 
    {
        golden_printf(MUT_LOG_SUITE_STATUS, list_tests_failed);
    }

    if (tests_not_executed) 
    {
        golden_printf(MUT_LOG_SUITE_STATUS, list_tests_not_executed);
    }

    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Result SuiteSummary   Status: %s%s Failed: %u Passed: %u NotExecuted: %u\n",
        tests_failed ? "FAILED" : "PASSED",
        tests_not_executed ? "-incomplete" : "",
        tests_failed, tests_passed, tests_not_executed);

    return;
}

void log_test_assert(const char* fmt, const char* id, int usermsg, const char* func, const char* file, unsigned line)
{
    char buf[1024];

    sprintf(buf, fmt, id);

    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Body   AssertFailed   %s%s\n", buf, usermsg ? " User info: Expected assert message, it shall be printed" : "");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   AssertFailed   Function %s\n", func);
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   AssertFailed   Thread 0x$\n");
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   AssertFailed   File %s\n", file);
    golden_printf(MUT_LOG_TEST_STATUS, "##/##/## ##:##:##.###  1 Body   AssertFailed   Line %u\n", line);

    return;
}

/*
 * Commonly used defines
 */

/** \fn void c_throw(void) 
*   \details Throws user defined C exception
*/
void c_throw(void)
{
    csx_rt_proc_raise_exception(CSX_TRUE);

    return;
}

/*********************************************************************/

/** \fn void assert_inner_func(void)
*   \details inner function for call from an outer function
*/
static void assert_inner_func(void) 
{
    MUT_ASSERT_TRUE(1==1)
    CHECKPOINT /*A*/

    return;
}

/** \fn void test_assert_called_func(void)
*   \details Passed assert in a called function does not affect the controo flow
*/
void test_assert_called_func_pass(void)
{
    LOG_TEST_BEGIN
    LOG_TEST_CHECKPOINT(-11 /*assert_inner_func.A*/)
    LOG_TEST_CHECKPOINT(4 /*B*/)
    LOG_TEST_END(LOG_STATUS_PASSED)

    assert_inner_func();
    CHECKPOINT /*B*/

    return;
}

/*********************************************************************/

/** \fn void assert_inner_func_crash(void)
*   \details inner function with assertion failed for call from an outer function
*/
static void assert_inner_func_crash(void)
{
    MUT_FAIL() /*A*/
    DISABLED_CODE

    return;
}

/** \fn void test_assert_called_func_fail(void)
*   \details check assert in a called function
*/
void test_assert_called_func_fail(void)
{
    LOG_TEST_BEGIN
    log_test_assert("Assert %s", "MUT_FAIL", 0, "assert_inner_func_crash", __FILE__, __LINE__ + (-12) /*assert_inner_func_crash.A*/);

    LOG_TEST_END(LOG_STATUS_FAILED)

    assert_inner_func_crash();
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_wrapper_setup(void) 
*   \details 
*/
void test_wrapper_setup(void)
{
    log_test_begin("test_wrapper_order");
    LOG_TEST_CHECKPOINT(5 /*A*/)
    LOG_TEST_CHECKPOINT(14 /*test_wrapper_order.B*/)
    LOG_TEST_CHECKPOINT(23 /*test_wrapper_teardown.C*/)
    log_test_end("test_wrapper_order", LOG_STATUS_PASSED);

    CHECKPOINT /*A*/

    return;
}

/** \fn void test_wrapper_order(void)
*   \details 
*/
void test_wrapper_order(void)
{
    CHECKPOINT /*B*/

    return;
}

/** \fn test_wrapper_teardown(void)
*   \details 
*/
void test_wrapper_teardown(void)
{
    CHECKPOINT /*C*/

    return;
}

/*********************************************************************/

/** \fn void test_wrapper_setup_crash_setup(void)
*   \details Setup function with failure
*/
void test_wrapper_setup_crash_setup(void)
{
    log_test_begin("test_wrapper_setup_crash");
	log_exception_encounted();
	log_write_to_dump_file();
    LOG_TEST_CHECKPOINT(24 /*test_wrapper_setup_crash_teardown.A*/)
    log_test_end("test_wrapper_setup_crash", LOG_STATUS_ERROR_EXCEPTION);

    c_throw();
    DISABLED_CODE

    return;
}

/** \fn void test_wrapper_setup_crash(void)
*   \details Test function shall not be called if setup function is failed
*/
void test_wrapper_setup_crash(void)
{
    DISABLED_CODE

    return;
}

/** \fn test_wrapper_setup_crash_teardown(void)
*   \details Teardown shall be called in any case even if setup or test functions failed
*/
void test_wrapper_setup_crash_teardown(void)
{
    CHECKPOINT /*A*/

    return;
}

/*********************************************************************/

/** \fn void test_wrapper_test_crash(void)
*   \details Test function shall not be called if setup function is failed
*/
void test_wrapper_test_crash(void)
{
    LOG_TEST_BEGIN
	log_exception_encounted();
	log_write_to_dump_file();
    LOG_TEST_CHECKPOINT(14/*test_wrapper_test_crash_teardown.A*/)
    LOG_TEST_END(LOG_STATUS_ERROR_EXCEPTION)

    c_throw();
    DISABLED_CODE

    return;
}

/** \fn test_wrapper_test_crash_teardown(void)
*   \details Teardown shall be called in any case even if setup or test functions failed
*/
void test_wrapper_test_crash_teardown(void)
{
    CHECKPOINT /*A*/

    return;
}

/*********************************************************************/

/** \fn void test_assert_true_pass(void)
*   \details check assert_true predicate
*/
void test_assert_true_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_TRUE(1==1)
    CHECKPOINT

    return;
}

/** \fn void test_assert_true_msg_pass(void)
*   \details check assert_true_msg predicate*
*/
void test_assert_true_msg_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_TRUE_MSG(1==1, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_true_fail_fmt = "%s failure: 0==1";

/** \fn void test_assert_true_fail(void) 
*   \details check assert_true predicate. negative test case
*/
void test_assert_true_fail(void)
{
    LOG_TEST_NEGATIVE(test_assert_true_fail_fmt, "MUT_ASSERT_TRUE", 2)

    MUT_ASSERT_TRUE(0==1);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_true_msg_fail(void)
*   \details check assert_true_msg predicate. Negative test case
*/
void test_assert_true_msg_fail(void)
{
    LOG_TEST_NEGATIVE_MSG(test_assert_true_fail_fmt, "MUT_ASSERT_TRUE_MSG", 2)

    MUT_ASSERT_TRUE_MSG(0==1, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_false_pass(void)
*   \details check assert_false predicate
*/
void test_assert_false_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FALSE(1==0)
    CHECKPOINT

    return;
}

/** \fn void test_assert_false_msg_pass(void)
*   \details check assert_false_msg predicate
*/
void test_assert_false_msg_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FALSE_MSG(1==0, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_false_fail_fmt = "%s failure: 1==1";

/** \fn void test_assert_false_fail(void)
*   \details check assert_false predicate
*/
void test_assert_false_fail(void)
{
    LOG_TEST_NEGATIVE(test_assert_false_fail_fmt, "MUT_ASSERT_FALSE", 2)

    MUT_ASSERT_FALSE(1==1)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_false_msg_fail(void)
*   \details check assert_false_msg predicate
*/
void test_assert_false_msg_fail(void)
{
    LOG_TEST_NEGATIVE_MSG(test_assert_false_fail_fmt, "MUT_ASSERT_FALSE_MSG", 2)

    MUT_ASSERT_FALSE_MSG(1==1, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_fail_fail(void)
*   \details check seert_fail_fail predicate
*/
void test_assert_fail_fail(void)
{
    LOG_TEST_NEGATIVE(mut_fail_fmt, "MUT_FAIL", 2)

    MUT_FAIL()
    DISABLED_CODE

    return;
}

/** \fn void test_assert_fail_msg_fail(void)
*   \details check seert_fail_msg_fail predicate
*/
void test_assert_fail_msg_fail(void)
{
    LOG_TEST_NEGATIVE_MSG(mut_fail_fmt, "MUT_FAIL_MSG", 2)

    MUT_FAIL_MSG(EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_null_pass(void)
*   \details check test_assert_null predicate
*/
void test_assert_null_pass(void)
{
    const char *ptr = NULL;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_NULL(ptr)
    CHECKPOINT

    return;
}

/** \fn void test_assert_null_msg_pass(void)
*   \details check assert_null_msg predicate
*/
void test_assert_null_msg_pass(void)
{
    const char *ptr = NULL;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_NULL_MSG(ptr, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_null_fail_fmt = "%%s failure: pointer is %p";

/** \fn void test_assert_null_fail(void)
*   \details check test_assert_null predicate
*/
void test_assert_null_fail(void)
{
    const void *ptr = (void*)1;
    char test_assert_null_fail_str[256];

    sprintf(test_assert_null_fail_str, test_assert_null_fail_fmt, ptr);

    LOG_TEST_NEGATIVE(test_assert_null_fail_str, "MUT_ASSERT_NULL", 2)

    MUT_ASSERT_NULL(ptr)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_not_null_pass(void)
*   \details check predicated assert_not_null
*/
void test_assert_not_null_pass(void)
{
    const void *ptr = (void*)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_NOT_NULL(ptr)
    CHECKPOINT

    return;
}

/** \fn void test_assert_null_msg_fail(void)
*   \details check assert_null_msg predicate. Negative variant
*/
void test_assert_null_msg_fail(void)
{
    const void *ptr = (void*)1;
    char test_assert_null_fail_str[256];

    sprintf(test_assert_null_fail_str, test_assert_null_fail_fmt, ptr);

    LOG_TEST_NEGATIVE_MSG(test_assert_null_fail_str, "MUT_ASSERT_NULL_MSG", 2)

    MUT_ASSERT_NULL_MSG(ptr, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_not_null_msg_pass(void)
*   \details check predicated assert_not_null_msg
*/
void test_assert_not_null_msg_pass(void)
{
    const void *ptr = (void*)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_NOT_NULL_MSG(ptr, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_not_null_fail_fmt = "%s failure: pointer is NULL";

/** \fn void test_assert_not_null_fail(void)
*   \details check predicated assert_not_null
*/
void test_assert_not_null_fail(void)
{
    const char *ptr = NULL;

    LOG_TEST_NEGATIVE(test_assert_not_null_fail_fmt, "MUT_ASSERT_NOT_NULL", 2)

    MUT_ASSERT_NOT_NULL(ptr)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_not_null_msg_fail(void)
*   \details check predicated assert_not_null_msg. negative test
*/
void test_assert_not_null_msg_fail(void)
{
    const char *ptr = NULL;

    LOG_TEST_NEGATIVE_MSG(test_assert_not_null_fail_fmt, "MUT_ASSERT_NOT_NULL_MSG", 2)

    MUT_ASSERT_NOT_NULL_MSG(ptr, EXPECTED_FMT)
    mut_printf(MUT_LOG_SUITE_STATUS, "This string should not be printed");

    return;
}

/*********************************************************************/

const char * const test_assert_pointer_equal_fail_fmt =
    "%%s failure: %p != %p";

const char * const test_assert_pointer_not_equal_fail_fmt =
    "%%s failure: %p == %p";

/** \fn void test_assert_pointer_equal_pass(void)
*   \details check assert_pointer_equal predicate. positive case
*/
void test_assert_pointer_equal_pass(void)
{
    const void* ptr = (void *)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_POINTER_EQUAL(ptr, (void *)1);
    CHECKPOINT

    return;
}

/** \fn void test_assert_pointer_equal_fail(void)
*   \details check assert_pointer_equal predicate. negative case
*/
void test_assert_pointer_equal_fail(void)
{
    const void* ptr = (void *)1;

    char test_assert_pointer_equal_fail_msg[256];

    _snprintf(test_assert_pointer_equal_fail_msg,
        255,
        test_assert_pointer_equal_fail_fmt,
        ptr,
        (void *)2
    );

    LOG_TEST_NEGATIVE(test_assert_pointer_equal_fail_msg, "MUT_ASSERT_POINTER_EQUAL", 2)

    MUT_ASSERT_POINTER_EQUAL(ptr, (void *)2);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_pointer_equal_msg_pass(void)
*   \details check assert_pointer_equal_msg predicate, positive case
*/
void test_assert_pointer_equal_msg_pass(void)
{
    const void* ptr = (void *)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_POINTER_EQUAL_MSG(ptr, (void *)1, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_pointer_equal_msg_fail(void)
*   \details check assert_pointer_equal_msg predicate. negative case
*/
void test_assert_pointer_equal_msg_fail(void)
{
    const void* ptr = (void *)1;

    char test_assert_pointer_equal_fail_msg[256];

    _snprintf(test_assert_pointer_equal_fail_msg,
        255,
        test_assert_pointer_equal_fail_fmt,
        ptr,
        (void *)2
    );

    LOG_TEST_NEGATIVE_MSG(test_assert_pointer_equal_fail_msg, "MUT_ASSERT_POINTER_EQUAL_MSG", 2)

    MUT_ASSERT_POINTER_EQUAL_MSG(ptr, (void *)2, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_pointer_not_equal_pass(void)
*   \details check assert_pointer_not_equal predicate, positive case
*/
void test_assert_pointer_not_equal_pass(void)
{
    const void* ptr = (void *)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_POINTER_NOT_EQUAL(ptr, (void *)2);
    CHECKPOINT

    return;
}

/** \fn void test_assert_pointer_not_equal_fail(void)
*   \details check assert_pointer_not_equal predicate, negative case
*/
void test_assert_pointer_not_equal_fail(void)
{
    const void* ptr = (void *)1;

    char test_assert_pointer_not_equal_fail_msg[256];

    _snprintf(
        test_assert_pointer_not_equal_fail_msg,
        255,
        test_assert_pointer_not_equal_fail_fmt,
        ptr,
        (void *)1
    );

    LOG_TEST_NEGATIVE(test_assert_pointer_not_equal_fail_msg, "MUT_ASSERT_POINTER_NOT_EQUAL", 2)

    MUT_ASSERT_POINTER_NOT_EQUAL(ptr, (void *)1);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_pointer_not_equal_msg_pass(void)
*   \details check assert_pointer_not_equal_msg predicate, positive case
*/
void test_assert_pointer_not_equal_msg_pass(void)
{
    const void* ptr = (void *)1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(ptr, (void *)2, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_pointer_not_equal_msg_fail(void)
*   \details check assert_pointer_not_equal_msg predicate, negative case
*/
void test_assert_pointer_not_equal_msg_fail(void)
{
    const void* ptr = (void *)1;

    char test_assert_pointer_not_equal_fail_msg[256];

    _snprintf(
        test_assert_pointer_not_equal_fail_msg,
        255,
        test_assert_pointer_not_equal_fail_fmt,
        ptr,
        (void *)1
    );

    LOG_TEST_NEGATIVE_MSG(test_assert_pointer_not_equal_fail_msg, "MUT_ASSERT_POINTER_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(ptr, (void *)1, EXPECTED_FMT);
    DISABLED_CODE

    return;
}


/*********************************************************************/

const char * const test_assert_int_equal_fail_fmt = "%s failure: 5 != 6, 0x5 != 0x6";

/** \fn void test_assert_int_equal_pass(void)
*   \details check assert_int_equal predicate
*/

typedef void fbe_u32_t;
#define MUT_ASSERT_FBE_U32_EQUAL(a, b) MUT_ASSERT_INT_EQUAL(a, b)

void test_assert_int_equal_pass(void)
{
    const unsigned int i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_INT_EQUAL(i, 5);
    CHECKPOINT

    return;
}

/** \fn void test_assert_int_equal_fail(void)
*   \details check assert_int_equal predicate. negative case
*/
void test_assert_int_equal_fail(void)
{
    const int i = 5;

    LOG_TEST_NEGATIVE(test_assert_int_equal_fail_fmt, "MUT_ASSERT_INT_EQUAL", 2)

    MUT_ASSERT_INT_EQUAL(i, 6);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_int_equal_pass(void)
*   \details check assert_int_equal predicate
*/
void test_assert_int_equal_msg_pass(void)
{
    const int i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_INT_EQUAL_MSG(i, 5, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_int_equal_fail(void)
*   \details check assert_int_equal predicate. negative case
*/
void test_assert_int_equal_msg_fail(void)
{
    const int i = 5;

    LOG_TEST_NEGATIVE_MSG(test_assert_int_equal_fail_fmt, "MUT_ASSERT_INT_EQUAL_MSG", 2)

    MUT_ASSERT_INT_EQUAL_MSG(i, 6, EXPECTED_FMT);
    DISABLED_CODE

    return;
}


const char * const test_assert_int_not_equal_fail_fmt =
    "%s failure: 5 == 5, 0x5 == 0x5";

/** \fn void test_assert_int_equal_pass(void)
*   \details check assert_int_equal predicate
*/
void test_assert_int_not_equal_pass(void)
{
    const int i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_INT_NOT_EQUAL(i, 6);
    CHECKPOINT

    return;
}

/** \fn void test_assert_int_equal_fail(void)
*   \details check assert_int_equal predicate. negative case
*/
void test_assert_int_not_equal_fail(void)
{
    const int i = 5;

    LOG_TEST_NEGATIVE(test_assert_int_not_equal_fail_fmt, "MUT_ASSERT_INT_NOT_EQUAL", 2)

    MUT_ASSERT_INT_NOT_EQUAL(i, 5);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_int_equal_pass(void)
*   \details check assert_int_equal predicate
*/
void test_assert_int_not_equal_msg_pass(void)
{
    const int i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_INT_NOT_EQUAL_MSG(i, 6, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_int_equal_fail(void)
*   \details check assert_int_equal predicate. negative case
*/
void test_assert_int_not_equal_msg_fail(void)
{
    const int i = 5;

    LOG_TEST_NEGATIVE_MSG(test_assert_int_not_equal_fail_fmt, "MUT_ASSERT_INT_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_INT_NOT_EQUAL_MSG(i, 5, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/*********************************************************************/

const char * const test_assert_long_equal_fail_fmt =
    "%s failure: 5 != 6, 0x5 != 0x6";

/** \fn void test_assert_long_equal_pass(void)
*   \details check assert_long_equal predicate
*/
void test_assert_long_equal_pass(void)
{
    const long i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_LONG_EQUAL(i, 5);
    CHECKPOINT

    return;
}

/** \fn void test_assert_long_equal_msg_pass(void)
*   \details check assert_long_equal_msg predicate
*/
void test_assert_long_equal_msg_pass(void)
{
    const long i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_LONG_EQUAL_MSG(i, 5, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_long_equal_fail(void)
*   \details check assert_long_equal predicate. negative case
*/
void test_assert_long_equal_fail(void)
{
    const long i = 5;

    LOG_TEST_NEGATIVE(test_assert_long_equal_fail_fmt, "MUT_ASSERT_LONG_EQUAL", 2)

    MUT_ASSERT_LONG_EQUAL(i, 6);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_long_equal_fail(void)
*   \details check assert_long_equal_msg predicate. negative case
*/
void test_assert_long_equal_msg_fail(void)
{
    const long i = 5;

    LOG_TEST_NEGATIVE_MSG(test_assert_long_equal_fail_fmt, "MUT_ASSERT_LONG_EQUAL_MSG", 2)

    MUT_ASSERT_LONG_EQUAL_MSG(i, 6, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

const char * const test_assert_long_not_equal_fail_fmt =
    "%s failure: 5 == 5, 0x5 == 0x5";

/** \fn void test_assert_long_equal_pass(void)
*   \details check assert_long_equal predicate
*/
void test_assert_long_not_equal_pass(void)
{
    const long i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_LONG_NOT_EQUAL(i, 6);
    CHECKPOINT

    return;
}

/** \fn void test_assert_long_equal_msg_pass(void)
*   \details check assert_long_equal_msg predicate
*/
void test_assert_long_not_equal_msg_pass(void)
{
    const long i = 5;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_LONG_NOT_EQUAL_MSG(i, 6, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

/** \fn void test_assert_long_equal_fail(void)
*   \details check assert_long_equal predicate. negative case
*/
void test_assert_long_not_equal_fail(void)
{
    const long i = 5;

    LOG_TEST_NEGATIVE(test_assert_long_not_equal_fail_fmt, "MUT_ASSERT_LONG_NOT_EQUAL", 2)

    MUT_ASSERT_LONG_NOT_EQUAL(i, 5);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_long_equal_fail(void)
*   \details check assert_long_equal_msg predicate. negative case
*/
void test_assert_long_not_equal_msg_fail(void)
{
    const long i = 5;

    LOG_TEST_NEGATIVE_MSG(test_assert_long_not_equal_fail_fmt, "MUT_ASSERT_LONG_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_LONG_NOT_EQUAL_MSG(i, 5, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_assert_integer_equal_pass(void)
*   \details check assert_integer_equal predicate
*/
void test_assert_integer_equal_pass(void)
{
    const unsigned char a = 0xFD;
    const unsigned short b = 0x1;
    const unsigned int c = 0x2;
    const unsigned long d = 0xDEADF00D;
    const unsigned __int64 e = 0x0123456789ABCDEF;
    LOG_TEST_POSITIVE(7)

    MUT_ASSERT_INTEGER_EQUAL(a, 0xFD);
    MUT_ASSERT_INTEGER_EQUAL(b, 0x1);
    MUT_ASSERT_INTEGER_EQUAL(c, 0x2);
    MUT_ASSERT_INTEGER_EQUAL(0xDEADF00D, d);
    MUT_ASSERT_INTEGER_EQUAL(0x0123456789ABCDEF, e);
    CHECKPOINT

    return;
}

/** \fn void test_assert_integer_equal_msg_pass(void)
*   \details check assert_integer_equal_msg predicate
*/
void test_assert_integer_equal_msg_pass(void)
{
    const unsigned char a = 0xFD;
    const unsigned short b = 0x1;
    const unsigned int c = 0x2;
    const unsigned long d = 0xDEADF00D;
    const __int64 e = 0x0123456789ABCDEF;
    LOG_TEST_POSITIVE(7)

    MUT_ASSERT_INTEGER_EQUAL_MSG(a, 0xFD, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_EQUAL_MSG(b, 0x1, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_EQUAL_MSG(c, 0x2, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_EQUAL_MSG(0xDEADF00D, d, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_EQUAL_MSG(0x0123456789ABCDEF, e, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

const char * const test_assert_integer_equal_fail_fmt =
    "%s failure: 2147483648 != 2147483649, 0x80000000 != 0x80000001";
/** \fn void test_assert_integer_equal_fail(void)
*   \details check assert_integer_equal predicate, negative case
*/
void test_assert_integer_equal_fail(void)
{
    const unsigned int i = 0x80000000;

    LOG_TEST_NEGATIVE(test_assert_integer_equal_fail_fmt, "MUT_ASSERT_INTEGER_EQUAL", 2)

    MUT_ASSERT_INTEGER_EQUAL(i, 0x80000001);
    DISABLED_CODE

    return;
}

const char * const test_assert_integer_equal_msg_fail_fmt =
//    "%s failure: 4294967295 != 18446744073709551615, 0xFFFFFFFF != 0xFFFFFFFFFFFFFFFF";
    "%s failure: 4294967295 != -1, 0xFFFFFFFF != 0xFFFFFFFFFFFFFFFF";
/** \fn void test_assert_integer_equal_fail(void)
*   \details check assert_integer_equal_msg predicate. negative case
*/
void test_assert_integer_equal_msg_fail(void)
{
    const unsigned long i = 0xFFFFFFFF;

    LOG_TEST_NEGATIVE_MSG(test_assert_integer_equal_msg_fail_fmt, "MUT_ASSERT_INTEGER_EQUAL_MSG", 2)

    MUT_ASSERT_INTEGER_EQUAL_MSG(i, -1, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_integer_equal_pass(void)
*   \details check assert_integer_equal predicate
*/
void test_assert_integer_not_equal_pass(void)
{
    const unsigned char a = 0xFD;
    const unsigned short b = 0x1;
    const unsigned int c = 0xFFFFFFFE;
    const unsigned long d = 0xDEADF00D;
    const unsigned __int64 e = 0x0123456789ABCDEF;
    LOG_TEST_POSITIVE(7)

    MUT_ASSERT_INTEGER_NOT_EQUAL(a, 0xFE);
    MUT_ASSERT_INTEGER_NOT_EQUAL(b, 0x2);
    MUT_ASSERT_INTEGER_NOT_EQUAL(c, (int)-2);
    MUT_ASSERT_INTEGER_NOT_EQUAL(0x0BADF00D, d);
    MUT_ASSERT_INTEGER_NOT_EQUAL(0xFEDCBA9876543210, e);
    CHECKPOINT

    return;
}

/** \fn void test_assert_integer_equal_msg_pass(void)
*   \details check assert_integer_equal_msg predicate
*/
void test_assert_integer_not_equal_msg_pass(void)
{
    const unsigned char a = 0xFD;
    const unsigned short b = 0x1;
    const unsigned int c = 0xFFFFFFFE;
    const unsigned long d = 0xDEADF00D;
    const unsigned __int64 e = 0x0123456789ABCDEF;
    LOG_TEST_POSITIVE(7)

    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(a, 0xFE, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(b, 0x2, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(c, (int)-2, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(0x0BADF00D, d, UNEXPECTED_FMT);
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(0xFEDCBA9876543210, e, UNEXPECTED_FMT);
    CHECKPOINT

    return;

}

const char * const test_assert_integer_not_equal_fail_fmt =
    "%s failure: 15 == 15, 0xF == 0xF";

/** \fn void test_assert_integer_equal_fail(void)
*   \details check assert_integer_equal predicate. negative case
*/
void test_assert_integer_not_equal_fail(void)
{
    const short i = 15;

    LOG_TEST_NEGATIVE(test_assert_integer_not_equal_fail_fmt, "MUT_ASSERT_INTEGER_NOT_EQUAL", 2)

    MUT_ASSERT_INTEGER_NOT_EQUAL(i, 15);
    DISABLED_CODE

    return;
}

const char * const test_assert_integer_not_equal_msg_fail_fmt =
    "%s failure: 7 == 7, 0x7 == 0x7";

/** \fn void test_assert_integer_equal_fail(void)
*   \details check assert_integer_equal_msg predicate. negative case
*/
void test_assert_integer_not_equal_msg_fail(void)
{
    const char i = 7;

    LOG_TEST_NEGATIVE_MSG(test_assert_integer_not_equal_msg_fail_fmt, "MUT_ASSERT_INTEGER_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(i, 7, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_assert_char_equal_pass(void)
*   \details check assert_char_equal predicate.
*/
void test_assert_char_equal_pass(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'a';

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CHAR_EQUAL(x, y)
    CHECKPOINT

    return;
}

/** \fn void test_assert_char_equal_msg_pass(void)
*   \details check assert_char_equeal_msg predicate.
*/
void test_assert_char_equal_msg_pass(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'a';

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CHAR_EQUAL_MSG(x, y, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_char_equal_fail_fmt =
    "%s failure: Expected 'a' Actual 'b'";

/** \fn void test_assert_char_equal_fail(void)
*   \details check assert_char_equal predicate. negative case
*/
void test_assert_char_equal_fail(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'b';

    LOG_TEST_NEGATIVE(test_assert_char_equal_fail_fmt, "MUT_ASSERT_CHAR_EQUAL", 2)

    MUT_ASSERT_CHAR_EQUAL(x, y)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_char_equal_msg(void)
*   \details check assert_char_equeal_msg predicate. negative case
*/
void test_assert_char_equal_msg_fail(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'b';

    LOG_TEST_NEGATIVE_MSG(test_assert_char_equal_fail_fmt, "MUT_ASSERT_CHAR_EQUAL_MSG", 2)

    MUT_ASSERT_CHAR_EQUAL_MSG(x, y, EXPECTED_FMT)
    DISABLED_CODE

    return;
}


/** \fn void test_assert_char_equal_pass(void)
*   \details check assert_char_equal predicate.
*/
void test_assert_char_not_equal_pass(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'b';

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CHAR_NOT_EQUAL(x, y)
    CHECKPOINT

    return;
}

/** \fn void test_assert_char_equal_msg_pass(void)
*   \details check assert_char_equeal_msg predicate.
*/
void test_assert_char_not_equal_msg_pass(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'b';

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CHAR_NOT_EQUAL_MSG(x, y, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_char_not_equal_fail_fmt =
    "%s failure: Expected 'a' Actual 'a'";

/** \fn void test_assert_char_equal_fail(void)
*   \details check assert_char_equal predicate. negative case
*/
void test_assert_char_not_equal_fail(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'a';

    LOG_TEST_NEGATIVE(test_assert_char_not_equal_fail_fmt, "MUT_ASSERT_CHAR_NOT_EQUAL", 2)

    MUT_ASSERT_CHAR_NOT_EQUAL(x, y)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_char_equal_msg(void)
*   \details check assert_char_equeal_msg predicate. negative case
*/
void test_assert_char_not_equal_msg_fail(void)
{
    const TCHAR x = 'a';
    const TCHAR y = 'a';

    LOG_TEST_NEGATIVE_MSG(test_assert_char_not_equal_fail_fmt, "MUT_ASSERT_CHAR_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_CHAR_NOT_EQUAL_MSG(x, y, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_assert_string_equal_pass(void)
*   \details check assert_string_equal predicate.
*/
void test_assert_string_equal_pass(void)
{
    const char * const x = "abcde";
    const char * const y = "abcde";

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_STRING_EQUAL(x, y)
    CHECKPOINT

    return;
}

/** \fn void test_assert_string_equal_msg(void)
*   \details check assert_string_equal_msg predicate.
*/
void test_assert_string_equal_msg_pass(void)
{
    const char * const x = "abc";
    const char * const y = "abc";

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_STRING_EQUAL_MSG(x, y, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_string_equal_fail_fmt_0 =
    "%s failure: Expected \"abc\" Actual \"ab\"";
const char * const test_assert_string_equal_fail_fmt_1 = 
    "%s failure: Expected \"ab\" Actual \"abc\"";
const char * const test_assert_string_not_equal_fail_fmt = 
    "%s failure: Expected \"abc\" Actual \"abc\"";

/** \fn void test_assert_string_equal_fail(void)
*   \details check assert_string_equal predicate. negative case
*/
void test_assert_string_equal_fail_0(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_NEGATIVE(test_assert_string_equal_fail_fmt_0, "MUT_ASSERT_STRING_EQUAL", 2)

    MUT_ASSERT_STRING_EQUAL(x, y)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_string_equal_fail(void)
*   \details check assert_string_equal predicate. negative case
*/
void test_assert_string_equal_fail_1(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_NEGATIVE(test_assert_string_equal_fail_fmt_1, "MUT_ASSERT_STRING_EQUAL", 2)

    MUT_ASSERT_STRING_EQUAL(y, x)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_string_equal_msg_fail(void)
*   \details check assert_string_equal_msg predicate. negative case
*/
void test_assert_string_equal_msg_fail_0(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_NEGATIVE_MSG(test_assert_string_equal_fail_fmt_0, "MUT_ASSERT_STRING_EQUAL_MSG", 2)

    MUT_ASSERT_STRING_EQUAL_MSG(x, y, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_string_equal_msg_fail(void)
*   \details check assert_string_equal_msg predicate. negative case
*
*
*/
void test_assert_string_equal_msg_fail_1(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_NEGATIVE_MSG(test_assert_string_equal_fail_fmt_1, "MUT_ASSERT_STRING_EQUAL_MSG", 2)

    MUT_ASSERT_STRING_EQUAL_MSG(y, x, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

void test_assert_string_not_equal_pass(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_POSITIVE(4)

    MUT_ASSERT_STRING_NOT_EQUAL(x, y)
    MUT_ASSERT_STRING_NOT_EQUAL(y, x)
    CHECKPOINT

    return;
}

void test_assert_string_not_equal_fail(void)
{
    const char * const x = "abc";
    const char * const y = "abc";

    LOG_TEST_NEGATIVE(test_assert_string_not_equal_fail_fmt, "MUT_ASSERT_STRING_NOT_EQUAL", 2)

    MUT_ASSERT_STRING_NOT_EQUAL(x, y)
    DISABLED_CODE

    return;
}

void test_assert_string_not_equal_msg_pass(void)
{
    const char * const x = "abc";
    const char * const y = "ab";

    LOG_TEST_POSITIVE(4)

    MUT_ASSERT_STRING_NOT_EQUAL(x, y)
    MUT_ASSERT_STRING_NOT_EQUAL(y, x)
    CHECKPOINT

    return;
}

void test_assert_string_not_equal_msg_fail(void)
{
    const char * const x = "abc";
    const char * const y = "abc";

    LOG_TEST_NEGATIVE(test_assert_string_not_equal_fail_fmt, "MUT_ASSERT_STRING_NOT_EQUAL", 2)

    MUT_ASSERT_STRING_NOT_EQUAL(x, y)
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_assert_float_equal_pass(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_equal_pass(void)
{
    const float x = (float)1.23;
    const float y = (float)1.23;
    const float e = (float)0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FLOAT_EQUAL(x, y, e)
    CHECKPOINT

    return;
}

/** \fn void test_assert_float_equal_msg_pass(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_equal_msg_pass(void)
{
    const float x = (float)1.23;
    const float y = (float)1.23;
    const float e = (float)0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FLOAT_EQUAL_MSG(x, y, e, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_float_equal_fail_fmt =
    "%s failure: Expected (1.230000e+000) Actual (1.240000e+000)";

/** \fn void test_assert_float_equal_fail(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_equal_fail(void)
{
    const float x = (float)1.23;
    const float y = (float)1.24;
    const float e = (float)0.001;

    LOG_TEST_NEGATIVE(test_assert_float_equal_fail_fmt, "MUT_ASSERT_FLOAT_EQUAL", 2)

    MUT_ASSERT_FLOAT_EQUAL(x, y, e)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_float_equal_fail(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_equal_msg_fail(void)
{
    const float x = (float)1.23;
    const float y = (float)1.24;
    const float e = (float)0.001;

    LOG_TEST_NEGATIVE_MSG(test_assert_float_equal_fail_fmt, "MUT_ASSERT_FLOAT_EQUAL_MSG", 2)

    MUT_ASSERT_FLOAT_EQUAL_MSG(x, y, e, EXPECTED_FMT)
    DISABLED_CODE

    return;
}


/** \fn void test_assert_float_equal_pass(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_not_equal_pass(void)
{
    const float x = (float)1.23;
    const float y = (float)1.24;
    const float e = (float)0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FLOAT_NOT_EQUAL(x, y, e)
    CHECKPOINT

    return;
}

/** \fn void test_assert_float_equal_msg_pass(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_not_equal_msg_pass(void)
{
    const float x = (float)1.23;
    const float y = (float)1.24;
    const float e = (float)0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_FLOAT_NOT_EQUAL_MSG(x, y, e, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

const char * const test_assert_float_not_equal_fail_fmt =
    "%s failure: Expected (1.230000e+000) Actual (1.230000e+000)";

/** \fn void test_assert_float_equal_fail(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_not_equal_fail(void)
{
    const float x = (float)1.23;
    const float y = (float)1.23;
    const float e = (float)0.001;

    LOG_TEST_NEGATIVE(test_assert_float_not_equal_fail_fmt, "MUT_ASSERT_FLOAT_NOT_EQUAL", 2)

    MUT_ASSERT_FLOAT_NOT_EQUAL(x, y, e)
    DISABLED_CODE

    return;
}

/** \fn void test_assert_float_equal_fail(void)
*   \details check assert_float_equal predicate. negative case
*/
void test_assert_float_not_equal_msg_fail(void)
{
    const float x = (float)1.23;
    const float y = (float)1.23;
    const float e = (float)0.001;

    LOG_TEST_NEGATIVE_MSG(test_assert_float_not_equal_fail_fmt, "MUT_ASSERT_FLOAT_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_FLOAT_NOT_EQUAL_MSG(x, y, e, EXPECTED_FMT)
    DISABLED_CODE

    return;
}

/*********************************************************************/

/** \fn void test_assert_double_equal_pass(void)
*   \details check assert_double_equal predicate
*/
void test_assert_double_equal_pass(void)
{
    const double i = 5.0;
    const double j = 5.0;
    const double e = 0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_DOUBLE_EQUAL(i, j, e);
    CHECKPOINT

    return;
}

/** \fn void test_assert_double_equal_msg_pass(void)
*   \details check assert_double_equal_msg predicate
*/
void test_assert_double_equal_msg_pass(void)
{
    const double i = 5.0;
    const double j = 5.0;
    const double e = 0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_DOUBLE_EQUAL_MSG(i, j, e, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

const char * const test_assert_double_equal_fail_fmt = 
    "%s failure: Expected (5.000000e+000) Actual (5.010000e+000)";

/** \fn void test_assert_double_equal_fail(void)
*   \details check assert_double_equal predicate. negative case
*/
void test_assert_double_equal_fail(void)
{
    const double i = 5.0;
    const double j = 5.01;
    const double e = 0.001;

    LOG_TEST_NEGATIVE(test_assert_double_equal_fail_fmt, "MUT_ASSERT_DOUBLE_EQUAL", 2)

    MUT_ASSERT_DOUBLE_EQUAL(i, j, e);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_double_equal_msg_fail(void)
*   \details check assert_double_equal_msg predicate. negative case
*/
void test_assert_double_equal_msg_fail(void)
{
    const double i = 5.0;
    const double j = 5.01;
    const double e = 0.001;

    LOG_TEST_NEGATIVE_MSG(test_assert_double_equal_fail_fmt, "MUT_ASSERT_DOUBLE_EQUAL_MSG", 2)

    MUT_ASSERT_DOUBLE_EQUAL_MSG(i, j, e, EXPECTED_FMT);
    DISABLED_CODE

    return;
}


/** \fn void test_assert_double_equal_pass(void)
*   \details check assert_double_equal predicate
*/
void test_assert_double_not_equal_pass(void)
{
    const double i = 5.00;
    const double j = 5.01;
    const double e = 0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_DOUBLE_NOT_EQUAL(i, j, e);
    CHECKPOINT

    return;
}

/** \fn void test_assert_double_equal_msg_pass(void)
*   \details check assert_double_equal_msg predicate
*/
void test_assert_double_not_equal_msg_pass(void)
{
    const double i = 5.00;
    const double j = 5.01;
    const double e = 0.001;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG(i, j, e, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

const char * const test_assert_double_not_equal_fail_fmt = 
    "%s failure: Expected (5.000000e+000) Actual (5.000000e+000)";

/** \fn void test_assert_double_equal_fail(void)
*   \details check assert_double_equal predicate. negative case
*/
void test_assert_double_not_equal_fail(void)
{
    const double i = 5.00;
    const double j = 5.00;
    const double e = 0.001;

    LOG_TEST_NEGATIVE(test_assert_double_not_equal_fail_fmt, "MUT_ASSERT_DOUBLE_NOT_EQUAL", 2)

    MUT_ASSERT_DOUBLE_NOT_EQUAL(i, j, e);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_double_equal_msg_fail(void)
*   \details check assert_double_equal_msg predicate. negative case
*/
void test_assert_double_not_equal_msg_fail(void)
{
    const double i = 5.00;
    const double j = 5.00;
    const double e = 0.001;

    LOG_TEST_NEGATIVE_MSG(test_assert_double_not_equal_fail_fmt, "MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG(i, j, e, EXPECTED_FMT);
    DISABLED_CODE

    return;
}

/*********************************************************************/



/** \fn void test_assert_condition_pass(void)
 *  \details    checks MUT_ASSERT_CONDITION predicate, positive case
 */
void test_assert_condition_pass(void)
{
    int variable = 4;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CONDITION( 2*2 == 4, ||, variable > 0)
    CHECKPOINT

    return;
}

/** \fn void test_assert_condition_pass(void)
 *  \details    checks MUT_ASSERT_CONDITION predicate, negative case
 */
void test_assert_condition_fail(void)
{
#define OP1 variable + 5
#define OP2 6
#define OPER >=
#define sOP1 "variable + 5"
#define sOP2 "6"
#define sOPER ">="
    int variable = 0;

    char test_assert_condition_fail_msg[256];

    _snprintf(test_assert_condition_fail_msg,
        255,
        "%%s failure: (%s) %s (%s) where %s = %d and %s = %d",
        sOP1, sOPER, sOP2, sOP1, OP1, sOP2, OP2
    );

    LOG_TEST_NEGATIVE(test_assert_condition_fail_msg, "MUT_ASSERT_CONDITION", 2)

    MUT_ASSERT_CONDITION( variable + 5, >=, 6)
    DISABLED_CODE

    return;
#undef OP1
#undef OP2
#undef OPER
#undef sOP1
#undef sOP2
#undef sOPER
}

/** \fn void test_assert_condition_pass(void)
 *  \details    checks MUT_ASSERT_CONDITION_MSG predicate, positive case
 */
void test_assert_condition_msg_pass(void)
{
    int variable = -1;

    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_CONDITION_MSG( variable , &&, variable <= 0, UNEXPECTED_FMT)
    CHECKPOINT

    return;
}

/** \fn void test_assert_condition_pass(void)
 *  \details    checks MUT_ASSERT_CONDITION_MSG predicate, negative case
 */
void test_assert_condition_msg_fail(void)
{
#define OP1 variable * (5 + variable)
#define OP2 variable - 1
#define OPER & 
#define sOP1 "variable * (5 + variable)"
#define sOP2 "variable - 1"
#define sOPER "&"
    int variable = 1;

    char test_assert_condition_msg_fail_msg[256];

    _snprintf(test_assert_condition_msg_fail_msg,
        255,
        "%%s failure: (%s) %s (%s) where %s = %d and %s = %d User info: %s",
        sOP1, sOPER, sOP2, sOP1, OP1, sOP2, OP2, EXPECTED_FMT
    );

    LOG_TEST_NEGATIVE(test_assert_condition_msg_fail_msg, "MUT_ASSERT_CONDITION_MSG", 2)

    MUT_ASSERT_CONDITION_MSG( variable * (5 + variable), &, variable - 1, EXPECTED_FMT )
    DISABLED_CODE

    return;
#undef OP1
#undef OP2
#undef OPER
#undef sOP1
#undef sOP2
#undef sOPER
}
/*********************************************************************/

/*
 * Commonly used buffer
 */
enum { buflen = 32, };
const char bufa[buflen + 1] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
const char bufb[buflen + 1] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
const char bufz[buflen + 1] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x1F, 0x13, 0x14, 0x15, 0x16, 0x17,
    /*            ^^                             */
    0x18, 0x19, 0x1A, 0x1F, 0x1C, 0x1D, 0x1E, 0x1F
    /*                  ^^                       */
};

/** \fn void test_assert_buffer_equal_pass(void)
*   \details check test_assert_buffer_equal predicate.
*/
void test_assert_buffer_equal_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_BUFFER_EQUAL(bufa, bufb, buflen);
    CHECKPOINT

    return;
}

/** \fn void test_assert_buffer_equal_msg_pass(void)
*   \details check test_assert_buffer_equal_msg predicate.
*/
void test_assert_buffer_equal_msg_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_BUFFER_EQUAL_MSG(bufa, bufa, buflen, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

const char * const test_assert_buffer_equal_fail_fmt = 
    "%s failure: 0x12 != 0x1F (offset 18)";

/** \fn void test_assert_buffer_equal_fail(void)
*   \details check test_assert_buffer_equal predicate. negative case
*/
void test_assert_buffer_equal_fail(void)
{
    LOG_TEST_NEGATIVE(test_assert_buffer_equal_fail_fmt, "MUT_ASSERT_BUFFER_EQUAL", 2)

    MUT_ASSERT_BUFFER_EQUAL(bufa, bufz, buflen);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_buffer_equal_msg_fail(void)
*   \details check test_assert_buffer_equal_msg predicate. negative case
*/
void test_assert_buffer_equal_msg_fail(void)
{
    LOG_TEST_NEGATIVE_MSG(test_assert_buffer_equal_fail_fmt, "MUT_ASSERT_BUFFER_EQUAL_MSG", 2)

    MUT_ASSERT_BUFFER_EQUAL_MSG(bufa, bufz, buflen, EXPECTED_FMT);
    DISABLED_CODE

    return;
}


/** \fn void test_assert_buffer_equal_pass(void)
*   \details check test_assert_buffer_equal predicate.
*/
void test_assert_buffer_not_equal_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_BUFFER_NOT_EQUAL(bufa, bufz, buflen);
    CHECKPOINT

    return;
}

/** \fn void test_assert_buffer_equal_msg_pass(void)
*   \details check test_assert_buffer_equal_msg predicate.
*/
void test_assert_buffer_not_equal_msg_pass(void)
{
    LOG_TEST_POSITIVE(3)

    MUT_ASSERT_BUFFER_NOT_EQUAL_MSG(bufa, bufz, buflen, UNEXPECTED_FMT);
    CHECKPOINT

    return;
}

const char * const test_assert_buffer_not_equal_fail_fmt = 
    "%s failure";

/** \fn void test_assert_buffer_equal_fail(void)
*   \details check test_assert_buffer_equal predicate. negative case
*/
void test_assert_buffer_not_equal_fail(void)
{
    LOG_TEST_NEGATIVE(test_assert_buffer_not_equal_fail_fmt, "MUT_ASSERT_BUFFER_NOT_EQUAL", 2)

    MUT_ASSERT_BUFFER_NOT_EQUAL(bufa, bufa, buflen);
    DISABLED_CODE

    return;
}

/** \fn void test_assert_buffer_equal_msg_fail(void)
*   \details check test_assert_buffer_equal_msg predicate. negative case
*/
void test_assert_buffer_not_equal_msg_fail(void)
{
    LOG_TEST_NEGATIVE_MSG(test_assert_buffer_not_equal_fail_fmt, "MUT_ASSERT_BUFFER_NOT_EQUAL_MSG", 2)

    MUT_ASSERT_BUFFER_NOT_EQUAL_MSG(bufa, bufa, buflen, EXPECTED_FMT);
    DISABLED_CODE

    return;
}


/*********************************************************************/

/** \fn void test_timeout_pass(void)
*   \details check test timeout. positive case
*/
void test_timeout_pass(void)
{
    LOG_TEST_POSITIVE(3/*A*/)

    csx_p_thr_sleep_msecs(500);
    CHECKPOINT /*A*/

    return;
}

/** \fn void test_timeout_fail(void)
*   \details check test timeout. negative case
*/
void test_timeout_fail(void)
{
    LOG_TEST_BEGIN
	log_test_end(__FUNCTION__,LOG_STATUS_TIMEOUT);
    csx_p_thr_sleep_msecs(1500);
    DISABLED_CODE

    return;
}

enum { num_verbosity_levels = 5, };

static struct {
    const char * const level_name;
    mut_logging_t log_level;
}
    verbosity_levels[num_verbosity_levels] = {
        {"suite",  MUT_LOG_SUITE_STATUS},
        {"test",   MUT_LOG_TEST_STATUS},
        {"low",    MUT_LOG_LOW},
        {"medium", MUT_LOG_MEDIUM},
        {"high",   MUT_LOG_HIGH},
/*        {"ktrace", MUT_LOG_KTRACE}*/
    };

static mut_logging_t get_verbosity_level_from_requested_testsuite()
{
    unsigned i;
    for (i = 0; i < num_verbosity_levels; i++)
    {
        if (!strcmp(verbosity_levels[i].level_name, 
                requested_testsuite + strlen(requested_testsuite) - 
                strlen(verbosity_levels[i].level_name)))
            return verbosity_levels[i].log_level;
    }
    
    MUT_FAIL_MSG("Wrong testsuite name for verbosity tests");
    return MUT_LOG_SUITE_STATUS;
}

/** \fn void test_verbosity_level_high(void)
*   \details output verbosity testing with high detalization
*/
void test_verbosity_level(void)
{
    unsigned i;
    const char * const test_output_fmt = "Message verbosity level %s";
    const char * const golden_output_fmt = "##/##/## ##:##:##.### %2u Body   MutPrintf      Message verbosity level %s\n";

    test_log_level = get_verbosity_level_from_requested_testsuite();

    LOG_TEST_BEGIN

    for (i = 0; i < num_verbosity_levels; i++)
    {
        golden_printf(verbosity_levels[i].log_level, golden_output_fmt, verbosity_levels[i].log_level, verbosity_levels[i].level_name);
    }

    LOG_TEST_END(LOG_STATUS_PASSED)

    for (i = 0; i < num_verbosity_levels; i++)
    {
        mut_printf(verbosity_levels[i].log_level, test_output_fmt, verbosity_levels[i].level_name);
    }

    return;
}

enum { test_multithread_loops = 16 };


/** \fn static void test_multithread_thread0(void * context)
*   \details thread0 for test_multithread test
*/
static void test_multithread_thread(void * context)
{
    int loop_cnt;

    for (loop_cnt = 0; loop_cnt < test_multithread_loops; loop_cnt++)
    {
        MUT_FAIL_MSG("EXPECTED THREAD POINT"); /*A*/
    }

    return;
}

/** \fn void test_multithread(void)
*   \details test_multithread for test_multithread test
*/
void test_multithread(void)
{

/*    
    MUT_THREAD_STATUS thread_status;
    mut_thread_t test_thread_handle;
    int loop_cnt;

    LOG_TEST_BEGIN
    for (loop_cnt = 0; loop_cnt <  test_multithread_loops; loop_cnt++)
    {
        log_test_assert("Assert %s User info: EXPECTED THREAD POINT", "MUT_FAIL_MSG", 0, "test_multithread_thread",
                __FILE__, __LINE__ - 19 );//test_multithread_thread.A
    }
    LOG_TEST_END(LOG_STATUS_FAILED)

    mut_control->mut_abort_policy->mut_set_current_abort_policy(MUT_CONTINUE_TEST);

    thread_status = mut_thread_init(&test_thread_handle, 
        test_multithread_thread, NULL);
    MUT_ASSERT_TRUE_MSG(thread_status == MUT_THREAD_SUCCESS, 
        "Failed to init thread!")

    Sleep(500); // Let threads finish
*/
    return;
}

void test_checkpoint_test(void)
{
    LOG_TEST_POSITIVE(2/*A*/)

    CHECKPOINT /*A*/

    return;
}

void test_disabled_test(void)
{
    DISABLED_CODE

    return;
}

unsigned file_exists(const char * pathfile)
{
    FILE *file;
    if ((file = fopen(pathfile, "r")) == NULL)
    {
        printf("SELFTEST: No file found %s\n", pathfile);
        return 0;
    }

    fclose(file);

    return 1;
}

unsigned is_value(char c)
{
    return c >= '0' && c <= '9' || c >= 'A' && c <= 'F' || c == '.' || c >= 'a' && c <= 'f';
}

void selftest_remove_digital_pattern(char *s0, char *s1)
{
    while (*s0 && *s1)
    {
        if (*s0 == '#' && *s1 >= '0' && *s1 <= '9')
        {
            *s1 = '#';
        }

        if (*s0 == '$')
        {
            unsigned cnt = 0;
            *(s1++) = '$';
            while (is_value(*s1))
            {
                s1++;
                cnt++;
            }
            if (cnt > 0)
            {
                strcpy(s1 - cnt, s1);
                s1 -= cnt;
            }

        }
        else
        {
            s1++;
        }

        s0++;
    }

    return;
}

void selftest_remove_rest_line(char *s0, char *s1)
{
    char sign = '@';
    while(*s0 && *(s0 + 1) && *s1)
    {
        if(*s0 == sign && *(s0 + 1) == sign)
        {
            while(*s0 && *s1)
                *s1 ++ = *s0 ++;
            *s1 = *s0;
            break;
        }
        ++ s0;
        ++ s1;
    }

    return;
}

unsigned compare_files(const char * cmp0, const char * cmp1)
{
    FILE *file0;
    FILE *file1;

    enum {
        buflen = 512,
    };


    char buf0[buflen];
    char buf1[buflen];

    unsigned line = 0;
    unsigned res = 0;
    char *status0, *status1;

    if ((file0 = fopen(cmp0, "r")) == NULL)
    {
        printf("SELFTEST: No file found %s\n", cmp0);

        return 0;
    }

    if ((file1 = fopen(cmp1, "r")) == NULL)
    {
        printf("SELFTEST: No file found %s\n", cmp1);
        fclose(file0);
        return 0;
    }

    res = 1;
    while (res)
    {
        status0 = fgets(buf0, buflen, file0);
        status1 = fgets(buf1, buflen, file1);

        if (!status0 && !status1)
        {
            break;
        }

        line++;

        if (status0 && status1)
        {
            if (strchr(buf0, '#'))
            {
                selftest_remove_digital_pattern(buf0, buf1);
            }

            selftest_remove_rest_line(buf0, buf1);

            if (!strcmp(buf0, buf1))
            {
                continue;
            }
            else
            {
                res = 0;
                printf("SELFTEST: Files compare different %s %s, %d {\n%s%s}\n", cmp0, cmp1, line, buf0, buf1);
                break;
            }
        }

        res = 0;

        if (status0)
        {
            printf("SELFTEST: File %s shorter than %s, line %d\n", cmp1, cmp0, line);
        }
        else
        {
            printf("SELFTEST: File %s shorter than %s, line %d\n", cmp0, cmp1, line);
        }
    }

    fclose(file0);
    fclose(file1);

    return res;
}

static void perform(
    const char * const suite,
    const char * const opts,
    const char * const dump,
    const char *       cmp0,
    const char *       cmp1)
{
    char cmd[1024];

    MUT_ASSERT_TRUE_MSG(suite, "Test suite check");

    sprintf(goldenfile_name, "%s%sgoldenfile_%s.log", logging_dir, MUT_PATH_SEP, suite);
    cmp1 = goldenfile_name;

    if (!cmp0)
    {
        create_log_name(suite_logfile, logging_dir, suite);
        cmp0 = suite_logfile;
    }

    sprintf(cmd, "%s -selftest mut_selftest.exe -text -run_testsuites %s -logDir %s -filename %s %s > %s", program, suite, logging_dir, logging_filename, opts, dump);

    system(cmd);

    MUT_ASSERT_TRUE_MSG(compare_files(cmp1, cmp0), "Failed to compare golden and actual output files");

    return;
}

    const char* usage_msg[] = {
       
        "  [-color_output] Enable customized log colors set in code\n"
        "  [-core] Create a core dump when the test fails. Clean up the logs and exit\n"
        "  [-disableconsole] logs are not set to the console\n"
        "  [-filename format] specify the format of log filename\n"
        "  [-formatfile filename] load text output format specification from file\n"
        "  [-help] this message\n"
        "  [-i <n>] specify the number of iterations to run (0 for endless)\n"
        "  [-info] display test information\n"
        "  [-initformatfile filename] creates text output format specification file with default content\n"
        "  [-isolate] execute each test in a separate process\n"
        "  [-list] list all tests\n"
        "  [-logdir dir] specify directory log results are to be written to\n"
        "  [-monitorexit] Trap to debugger on unexpected exit\n"
        "  [-nodebugger] Do not go to the debugger when a test fails\n"
        "  [-nofailontimeout] do not fail tests on timeout\n"
        "  [-pause_test] Enable test pausing point in case code\n"
        "  [-run_tests test1[,test2]...] comma separated list of tests (names and/or numbers) to run\n"
        "  [-run_testsuites suite1[,suite2]...] comma separated list of testsuites to run\n"
        "  [-text] generate textual output\n"
        "  [-timeout timeout] timeout in seconds for single test\n"
        "  [-verbosity suite] display/log suite progress\n"
        "  [-verbosity test] display/log suite and test progress\n"
        "  [-verbosity low] display/log control and minimal test progress\n"
        "  [-verbosity medium] display/log control and medium test progress\n"
        "  [-verbosity high] display/log control and all test progress\n"
        "  [-verbosity ktrace] display/log all messages(Default)\n"
        "  [-xml] generate structured XML output\n"
        "  [N] execute test number N\n"
        "  [N-M] execute tests numbered N through M\n"
        
    };

    
static void test_main_commandline_help(void)
{

    FILE *goldenfile;
    int i;

    char cmd[1024];
    char goldenfile_name[MAX_PATH];
    char actual_output[MAX_PATH];

    sprintf(goldenfile_name, "%s%sgoldenfile_help.log", logging_dir, MUT_PATH_SEP);
    sprintf(actual_output, "%s%stestsuite_help.txt", logging_dir, MUT_PATH_SEP);
        
    sprintf(cmd, "%s -selftest mut_selftest.exe -text -help > %s", program, actual_output);
    system(cmd);

    if ((goldenfile = fopen(goldenfile_name, "w")) == NULL)
    {
        printf( "The help golden file was not opened\n" );
        exit(0);
    }

    fprintf(goldenfile, "Running this program executes all tests\n");
    for (i = 0; i < sizeof(usage_msg) / sizeof(usage_msg[0]); i++)
    {
        fputs(usage_msg[i], goldenfile);
    }

    fclose(goldenfile);

    MUT_ASSERT_TRUE_MSG(compare_files(goldenfile_name, actual_output), "Failed to compare golden and actual output files");

    return;
}

static void test_main_commandline_initformatfile(void)
{
    const char* def_format[] = {
        "\\Header\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% Iteration      %Iteration%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %FormatName:14% Version %Version%\n",
        "\\0:%Date% %Time% %Verbosity:r2% %Section:6% TestSuite      %SuiteName%\n",
        "\\0:%Date% %Time% %Verbosity:r2% %Section:6% LogFile        %LogFile%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% ComputerName   %ComputerName%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% UserName       %UserName%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% MutVersion     %MutVersion%\n",
        "\\0:%Date% %Time% %Verbosity:r2% %Section:6% SuiteStarted   %SuiteName%\n",
        "\\\\\n",
        "\\TestStarted\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% (%TestIndex:r2%) %TestName%: %TestDescription%\n",
        "\\\\\n",
        "\\AssertFailed\n",
        "\\0:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% %Message%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% Function %Func%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% Thread %Thread%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% File %File%\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% Line %Line%\n",
        "\\\\\n",
        "\\MutPrintf\n",
        "\\0:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% %Message%\n",
        "\\\\\n",
        "\\KTrace\n",
        "\\16:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% %Message%\n",
        "\\\\\n",
        "\\TestFinished\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% %Class:14% (%TestIndex:r2%) %TestName% %TestStatus%\n",
        "\\\\\n",
        "\\Result\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% SuiteFinished  Duration: %Duration%s\n",
        "\\1:%TestTableFailed%\\1:%TestTableNotExecuted%\\0:%Date% %Time% %Verbosity:r2% %Section:6% SuiteSummary   Status: %SuiteStatus% Failed: %TestsFailedCount% Passed: %TestsPassedCount% NotExecuted: %TestsNotExecutedCount%\n",
        "\\\\\n",
        "\\TestTableFailed\n",
        "%TestListFailed%\\\\\n",
        "\\TestTableNotExecuted\n",
        "%TestListNotExecuted%\\\\\n",
        "\\TestItemFailed\n",
        "%Date% %Time% %Verbosity:r2% %Section:6% TestFailed     (%TestIndex:r2%) %TestName%\n",
        "\\\\\n",
        "\\TestItemNotExecuted\n",
        "%Date% %Time% %Verbosity:r2% %Section:6% TestNotRunned  (%TestIndex:r2%) %TestName%\n",
        "\\\\\n",
        "\\Summary\n",
        "\\1:%Date% %Time% %Verbosity:r2% %Section:6% SuiteSummary   Suite: %SuiteName%s\n",
        "\\1:%TestTableFailed%\\1:%TestTableNotExecuted%\\0:%Date% %Time% %Verbosity:r2% %Section:6% SuiteSummary   Status: %SuiteStatus% Failed: %TestsFailedCount% Passed: %TestsPassedCount% NotExecuted: %TestsNotExecutedCount%\n",
        "\\\\\n"
    };

    FILE *goldenfile;
    int i;

    char cmd[1024];
    char goldenfile_name[MAX_PATH];
    char actual_output[MAX_PATH];

    sprintf(goldenfile_name, "%s%sgoldenfile_initformatfile.log", logging_dir, MUT_PATH_SEP);
    sprintf(actual_output, "%s%stestsuite_initformatfile.txt", logging_dir, MUT_PATH_SEP);
        
    sprintf(cmd, "%s -selftest mut_selftest.exe -text -initformatfile %s", program, actual_output);
    system(cmd);

    if ((goldenfile = fopen(goldenfile_name, "w")) == NULL)
    {
        printf( "The initformatfile golden file was not opened\n" );
        exit(0);
    }

    for (i = 0; i < sizeof(def_format) / sizeof(def_format[0]); i++)
    {
        fputs(def_format[i], goldenfile);
    }

    fclose(goldenfile);

    MUT_ASSERT_TRUE_MSG(compare_files(goldenfile_name, actual_output), "Failed to compare golden and actual output files");

    return;
}

static void test_main_commandline_list(void)
{
    char cmd[1024];
    char goldenfile_name[MAX_PATH];
    char actual_output[MAX_PATH];

    sprintf(actual_output, "%s%stestsuite_list.txt", logging_dir, MUT_PATH_SEP);
    sprintf(cmd, "%s -selftest mut_selftest.exe -text -list > %s", program, actual_output);
    system(cmd);

    sprintf(goldenfile_name, "%s%sgoldenfile_list.log", logging_dir, MUT_PATH_SEP);

    MUT_ASSERT_TRUE_MSG(compare_files(goldenfile_name, actual_output), "Failed to compare golden and actual output files");

    return;
}

unsigned volatile test_mutex_resource;
mut_mutex_t test_mutex_mutex;

void test_mutex_check(unsigned res, unsigned delay)
{
    /* Enter the critical section */
    mut_mutex_acquire(&test_mutex_mutex);

    test_mutex_resource = res;
    CHECKPOINT /* A */
    csx_p_thr_sleep_msecs(delay);
    MUT_ASSERT_INT_EQUAL(res, test_mutex_resource);

    /* Leave the critical section */
    mut_mutex_release(&test_mutex_mutex);

    return;
}

static void test_mutex_thread_5_15_5(void * context)
{
    int loop_cnt;

    for (loop_cnt = 0; loop_cnt < test_multithread_loops; loop_cnt++)
    {
        test_mutex_check(loop_cnt * 10 + 0, 10);
        test_mutex_check(loop_cnt * 10 + 1, 30);
        test_mutex_check(loop_cnt * 10 + 2, 10);
    }

    return;
}

static void test_mutex_thread_10_5_10(void * context)
{
    int loop_cnt;

    for (loop_cnt = 0; loop_cnt < test_multithread_loops; loop_cnt++)
    {
        test_mutex_check(loop_cnt * 10 + 3, 20);
        test_mutex_check(loop_cnt * 10 + 4, 10);
        test_mutex_check(loop_cnt * 10 + 5, 25);
    }

    return;
}

/** \fn void test_main_mutex_pass(void)
*   \details positive test for mutex
*/
static void test_mutex(void)
{
/*
    MUT_THREAD_STATUS thread_status;
    mut_thread_t test_thread_handle;
    int loop_cnt;

    LOG_TEST_BEGIN
    for (loop_cnt = 0; loop_cnt < 6 * test_multithread_loops; loop_cnt++)
    {
        LOG_TEST_CHECKPOINT(-50);// test_mutex_check.A);
    }
    LOG_TEST_END(LOG_STATUS_PASSED)

    mut_mutex_init(&test_mutex_mutex);

    thread_status = mut_thread_init(&test_thread_handle, 
        test_mutex_thread_5_15_5, NULL);
    MUT_ASSERT_TRUE_MSG(thread_status == MUT_THREAD_SUCCESS, 
        "Failed to init thread!")

    thread_status = mut_thread_init(&test_thread_handle, 
        test_mutex_thread_10_5_10, NULL);
    MUT_ASSERT_TRUE_MSG(thread_status == MUT_THREAD_SUCCESS, 
        "Failed to init thread!")

    mut_thread_wait(&test_thread_handle, INFINITE);
    //Sleep(test_multithread_loops * 175); // Let threads finish 

    mut_mutex_destroy(&test_mutex_mutex);
*/
    return;
}

//static void test_mut_ktrace_api(void)
//{
//    LOG_TEST_BEGIN
//    golden_printf(0, "##/##/## ##:##:##.### 16 Body   KTrace         1 2 3 4\n");
//    Ktrace((ULONG)"%ld %ld %ld %ld", 1L, 2L, 3L, 4L);
//    LOG_TEST_END(LOG_STATUS_PASSED)
//    /*Ktrace()*/
//    return;
//}
//
///* This function will be called instead of mut_ktrace */
//static void test_mut_ktrace_handler(const char * format, ...)
//{
//    MUT_ASSERT_TRUE(1==1)
//    CHECKPOINT 
//    return;
//}
//
///* Checks that new handler is set and restored correctly */
//static void test_mut_ktrace_set_handler(void)
//{
//    LOG_TEST_BEGIN
//    KtraceSetPrintfPtr(&test_mut_ktrace_handler);
//    LOG_TEST_CHECKPOINT(-9) /* test_mut_ktrace_handler */
//    LOG_TEST_CHECKPOINT(3) /* the next one */
//    Ktrace((ULONG)"%ld %ld %ld %ld", 0L, 0L, 0L, 0L);
//    KtraceSetPrintfPtr(&mut_ktrace);
//    CHECKPOINT 
//    LOG_TEST_END(LOG_STATUS_PASSED)
//    return;
//}

static void test_main_control(void)
{
    perform("control", " ", "nul", NULL, goldenfile_name);

    return;
}

static void test_main_asserts(void)
{
    perform("asserts", " ", "nul", NULL, goldenfile_name);

    return;
}

//static void test_main_mut_ktrace_api(void)
//{
//    perform("mut_ktrace_api", "-abortpolicy test ", "nul", NULL, goldenfile_name);
//
//    return;
//}

static void test_main_timeout(void)
{
	//updated the option to "-failontimeout" causing tests to fail on specified timeout
    perform("timeout", "-failontimeout ", "nul", NULL, goldenfile_name);

    return;
}

static void test_main_timeout_config(void)
{
    char opts[MAX_PATH];

	//updated the option to "-failontimeout" causing tests to fail on specified timeout
    strcpy(opts, "-failontimeout");
    perform("timeout_config", opts, "nul", NULL, goldenfile_name);
    return;
}

static void test_main_timeout_config_teardown(void)
{
    csx_p_file_delete(path_to_config);

    return;
}

static void test_main_logdirname_config(void)
{
    char opts[MAX_PATH];
    char actual_log[MAX_PATH];

    sprintf(opts, "-logDir \"%s\" -filename %s ", logging_dir_alt, logging_filename_alt);
    sprintf(actual_log, "%s%s%s", logging_dir_alt, MUT_PATH_SEP, logging_filename_alt);
    perform("logdirname_config", opts, "nul", actual_log, goldenfile_name);

    return;
}

#define SELFTEST_VERBOSITY_TEST(verb) \
    static void test_main_verbosity_##verb(void) \
    { \
    perform("verbosity_"#verb, "-verbosity "#verb, \
        "logdir" MUT_PATH_SEP "dump_verbosity_"#verb".txt", \
        "logdir" MUT_PATH_SEP "dump_verbosity_"#verb".txt", \
        goldenfile_name); \
    return; \
    }

SELFTEST_VERBOSITY_TEST(suite)
SELFTEST_VERBOSITY_TEST(test)
SELFTEST_VERBOSITY_TEST(low)
SELFTEST_VERBOSITY_TEST(medium)
SELFTEST_VERBOSITY_TEST(high)
/*SELFTEST_VERBOSITY_TEST(ktrace)*/

#undef SELFTEST_VERBOSITY_TEST
/*
static void test_main_multithread(void)
{
    perform("multithread", "", "nul", NULL, goldenfile_name);

    return;
}
*/
static void test_main_mutex(void)
{
    perform("mutex", "", "nul", NULL, goldenfile_name);

    return;
}




/***************************************************************************
 * MUT WATCH TEST PATTERN TESTS BEGIN
 ***************************************************************************/
#if 0
/** \fn void test_testpatterns_timewatch_fail(void)
*   \details MUT Time Watch pattern test case
*/
static void test_testpatterns_timewatch_fail(void) 
{
    MUT_TIMEWATCH_DECLARE(sleep_watch)

    LOG_TEST_NEGATIVE(
        "%s failure: the actual time $ is not within the admissible gap (200, +25)",
        "MUT_TIMEWATCH_ASSERT", 10/*A*/)

    /* The following block shall be passed */
    MUT_TIMEWATCH_START(sleep_watch)
    mut_sleep(100);
    MUT_TIMEWATCH_ASSERT(sleep_watch, 100, 25)

    /* The following block shall be failed */
    MUT_TIMEWATCH_START(sleep_watch)
    mut_sleep(100);
    MUT_TIMEWATCH_ASSERT(sleep_watch, 200, 25) /* A */
}

/***************************************************************************
 * MUT WATCH TEST PATTERN TESTS END
 ***************************************************************************/
#endif
/***************************************************************************
 * DATA ITERATOR TEST PATTERN TESTS BEGIN
 ***************************************************************************/
#if 0
typedef struct test_data_s {
    int i;
    char *s;
} test_data_t;

test_data_t test_data[] = {
    { 0, "zero" },
    { 1, "one" }
};

void test_func(test_data_t *d) 
{
    mut_printf(MUT_LOG_SUITE_STATUS, "DATA %d %s", d->i, d->s);
}

/** \fn void test_testpatterns_dataiterator_pass(void)
*   \details MUT Time Watch pattern test case
*/
static void test_testpatterns_dataiterator_pass(void) 
{
    LOG_TEST_BEGIN
        LOG_TEST_MSG("DATA 0 zero")
        LOG_TEST_MSG("DATA 1 one")
    LOG_TEST_END(LOG_STATUS_PASSED)

    MUT_DATA_ITERATOR_FUNC(test_func, test_data)
}
#endif
/***************************************************************************
 * DATA ITERATOR TEST PATTERN TESTS END
 ***************************************************************************/

/***************************************************************************
 * MUT SELTEST C++ LINK BEGIN
 ***************************************************************************/



static void test_main_cplusplus(void)
{
    perform("cplusplus", "", "nul", NULL, goldenfile_name);

    return;
}

/***************************************************************************
 * MUT SELTEST C++ LINK END
 ***************************************************************************/

/***************************************************************************
 * MUT SELTEST MUT_PRINTF XML ESCAPE BEGIN
 ***************************************************************************/


#if 0
static void test_main_xml_special_escape(void) 
{
#define MUT_XML_SPECIAL_ESCAPE(org, exp, name) \
    char xml_##name[MUT_STRING_LEN]; \
    int char_##name##_escape;
#include "mut_xml_special_escape_test.inc"

    MUT_CONTINUE_TEST_ON_FAILURE

#define MUT_XML_SPECIAL_ESCAPE(org, exp, name) \
    mut_convert_to_xml(xml_##name, "Test: Dummy text "org" more dummy text"); \
    char_##name##_escape = (strstr(xml_##name, "Test: Dummy text "exp" more dummy text") != NULL); \
    MUT_ASSERT_TRUE_MSG(char_##name##_escape, "Test xml escape for character '"org"' failed.");
#include "mut_xml_special_escape_test.inc"

    MUT_ABORT_TEST_ON_FAILURE

    return;

}
#endif
/***************************************************************************
 * MUT SELTEST MUT_PRINTF XML ESCAPE END
 ***************************************************************************/

/***************************************************************************
 * MUT TEST DESCRIPTION (INFO) TESTS BEGIN
 ***************************************************************************/

const char *test_testinfo_format_short_desc = "this is short description for test_testinfo_format";
const char *test_testinfo_format_long_desc = "this is long description for test_testinfo_format";

/** \fn void test_testinfo_format(void)
*   \details Test description list (-info) test case.
*/
void test_testinfo_format(void)
{
    char actual_output[MAX_PATH];
    char infofile_name[MAX_PATH];
    char cmd[1024];

    sprintf(actual_output, "%s%stestinfo_format.txt", logging_dir, MUT_PATH_SEP);
    sprintf(cmd, "%s -selftest mut_selftest.exe -text -info > %s", program, actual_output);
    sprintf(infofile_name, "%s%sgoldenfile_testinfo_format.log", logging_dir, MUT_PATH_SEP);

    LOG_TEST_BEGIN

    system(cmd);

    MUT_ASSERT_TRUE_MSG(compare_files(infofile_name, actual_output), "Failed to compare golden and actual output files");
    
    LOG_TEST_END(LOG_STATUS_PASSED)

    return;
}

const char *test_testinfo_start_description_short_desc = "this is short description for test_testinfo_start_description";
const char *test_testinfo_start_description_long_desc = "this is long description for test_testinfo_start_description";

/** \fn void test_testinfo_start_description(void)
*   \details Test description display when test starts test case.
*/
void test_testinfo_start_description(void)
{
    log_test_begin_with_description(__FUNCTION__, test_testinfo_start_description_short_desc);
    
    LOG_TEST_END(LOG_STATUS_PASSED)
  
    return;
}

/** \fn void test_main_testinfo(void)
*   \details Test description test case set.
*/
static void test_main_testinfo(void) 
{
    perform("testinfo", " ", "nul", NULL, goldenfile_name);

    return;
}

/***************************************************************************
 * MUT TEST DESCRIPTION (INFO) TESTS END
 ***************************************************************************/

/***************************************************************************
 * MUT TEST USER OPTION TESTS BEGIN
 ***************************************************************************/

typedef struct {
    const char *option_name;
    int least_num_of_args;
    BOOL show;
    const char *description;
} register_user_option_t;

register_user_option_t register_user_options[] = {
    {"-bat",      1, TRUE, "the batch file you want to process"},
    {"-debugger", 1, TRUE, "the debugger you want to choose"},
    {"-debug",    1, TRUE, "the SP you want to debug"},
    {"-sp_log",   1, TRUE, "the log destination"}
};

const char* user_usage_msg[] = {

        "  [-bat] the batch file you want to process\n"
        "  [-color_output] Enable customized log colors set in code\n"
        "  [-core] Create a core dump when the test fails. Clean up the logs and exit\n"
        "  [-debug] the SP you want to debug\n"
        "  [-debugger] the debugger you want to choose\n"
        "  [-disableconsole] logs are not set to the console\n"
        "  [-filename format] specify the format of log filename\n"
        "  [-formatfile filename] load text output format specification from file\n"
        "  [-help] this message\n"
        "  [-i <n>] specify the number of iterations to run (0 for endless)\n"
        "  [-info] display test information\n"
        "  [-initformatfile filename] creates text output format specification file with default content\n"
        "  [-isolate] execute each test in a separate process\n"
        "  [-list] list all tests\n"
        "  [-logdir dir] specify directory log results are to be written to\n"
        "  [-monitorexit] Trap to debugger on unexpected exit\n"
        "  [-nodebugger] Do not go to the debugger when a test fails\n"
        "  [-nofailontimeout] do not fail tests on timeout\n"
        "  [-pause_test] Enable test pausing point in case code\n"
        "  [-run_tests test1[,test2]...] comma separated list of tests (names and/or numbers) to run\n"
        "  [-run_testsuites suite1[,suite2]...] comma separated list of testsuites to run\n"
        "  [-sp_log] the log destination\n"
        "  [-text] generate textual output\n"
        "  [-timeout timeout] timeout in seconds for single test\n"
        "  [-verbosity suite] display/log suite progress\n"
        "  [-verbosity test] display/log suite and test progress\n"
        "  [-verbosity low] display/log control and minimal test progress\n"
        "  [-verbosity medium] display/log control and medium test progress\n"
        "  [-verbosity high] display/log control and all test progress\n"
        "  [-verbosity ktrace] display/log all messages(Default)\n"
        "  [-xml] generate structured XML output\n"
        "  [N] execute test number N\n"
        "  [N-M] execute tests numbered N through M\n"
    
    };

/** \fn void test_main_register_user_option(void)
*   \details Test register user option test case set.
*/
static void test_main_commandline_register_user_option(void)
{
    FILE *goldenfile;
    int i;

    char cmd[1024];
    char goldenfile_name[MAX_PATH];
    char actual_output[MAX_PATH];

    sprintf(goldenfile_name, "%s%sgoldenfile_register_user_option.log", logging_dir, MUT_PATH_SEP);
    sprintf(actual_output, "%s%stestsuite_register_user_option.txt", logging_dir, MUT_PATH_SEP);

    sprintf(cmd, "%s -selftest mut_selftest.exe -text -register_user_option -help > %s", program, actual_output);
    system(cmd);

    if ((goldenfile = fopen(goldenfile_name, "w")) == NULL)
    {
        printf( "The register user option golden was not opened\n" );
        exit(0);
    }

    fprintf(goldenfile, "Running this program executes all tests\n");

    // changed to a static array, rather than building from the 'register_user_option' array
    // because output is now sorted alphabetically

    for (i = 0; i < sizeof(user_usage_msg) / sizeof(user_usage_msg[0]); ++i)
    {
        fputs(user_usage_msg[i], goldenfile);
    }


    fclose(goldenfile);

    MUT_ASSERT_TRUE_MSG(compare_files(goldenfile_name, actual_output), "Failed to compare golden and actual output files");

    return;
}

/** \fn void test_user_option(void)
*   \details Test user option passing test case.
*/
void test_user_option(void)
{
    char cmd[1024];

    LOG_TEST_BEGIN

    sprintf(cmd, "%s -selftest mut_selftest.exe -user_option -abc -xyz 0 cc -dbg ii oo pp -info", program);
    if (!system(cmd))
    {
        LOG_TEST_END(LOG_STATUS_PASSED)
    }
    else
    {
        MUT_FAIL_MSG("Error passing user option!")
        LOG_TEST_END(LOG_STATUS_FAILED)
    }

    return;
}
/** \fn void test_main_user_option(void)
*   \details Test user option test case set.
*/
static void test_main_user_option(void)
{
    //perform("user_option", "-abortpolicy test ", "nul", NULL, goldenfile_name);
    perform("user_option", " ", "nul", NULL, goldenfile_name);

    return;
}

/***************************************************************************
 * MUT TEST USER OPTION TESTS END
 ***************************************************************************/

/***************************************************************************
 * MUT SELTEST ktrace BEGIN
 ***************************************************************************/
/*
static void test_main_ktraceSuite(void)
{
    perform("ktraceSuite", " ", "nul", NULL, goldenfile_name);
}
//
//    This test verifies that KtraceInit() and KtraceSetPrintfFunc() are setup
//    in mut_init (or somewhere). When KvTrace() is called anywhere, the message  
//    gets logged to mut via the KtraceSetPrintfFunc callback function.
//
void ktraceCallbackTest(void)
{
    char message[] = "This message was logged with KvTrace()\n";

    // Create a golden file
    log_test_begin("ktraceCallbackTest");
    golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.### 16 Body   KTrace         %s", message);
    log_test_end("ktraceCallbackTest", LOG_STATUS_PASSED);

    // Do the test by logging the message
    KvTrace(message);

    return;
}
*/
/***************************************************************************
 * MUT SELTEST ktrace END
 ***************************************************************************/

#define SELFTEST_REGISTER_TEST(suite, test, positive) \
    if (testsuite_##suite##_testcnt == -1) \
    { \
        testsuite_##suite##_testcnt = walkthrough_test_counter; \
    } \
    if (positive == 1) \
    { \
        testsuite_##suite##_tests_to_pass++; \
    } \
    else \
    { \
        char test_info[256]; \
        sprintf(test_info, "##/##/## ##:##:##.###  0 Result %s (%2d) %s\n", \
          positive ? "TestNotRunned " : "TestFailed    ", walkthrough_test_counter, #test); \
        if (positive) \
        { \
            testsuite_##suite##_tests_to_not_execute++; \
            strcat(testsuite_##suite##_list_tests_to_not_execute, test_info); \
        } \
        else \
        { \
            testsuite_##suite##_tests_to_fail++; \
            strcat(testsuite_##suite##_list_tests_to_fail, test_info); \
        } \
    } \
    if (listfile) \
    { \
        if(strcmp(local_suite_name,#suite)) \
        { \
            fprintf(listfile,"%s: \n",#suite); \
        } \
		fprintf(listfile, "(%d) [timeout: @@ sec.] %s:    ", walkthrough_test_counter, #test); \
        fprintf(listfile, "@@\n"); \
        strcpy(local_suite_name,#suite); \
    } \
    if (infofile) \
    { \
        fprintf(infofile, "(%d) %s.%s\n", walkthrough_test_counter, #suite, #test); \
        fprintf(infofile, "  Short description: @@\n"); \
        fprintf(infofile, "  Long  description: @@\n"); \
    } \
    walkthrough_test_counter++;

#define SELFTEST_REGISTER_TEST_WITH_DESCRIPTION(suite, test, positive, short_desc, long_desc) \
    if (testsuite_##suite##_testcnt == -1) \
    { \
        testsuite_##suite##_testcnt = walkthrough_test_counter; \
    } \
    if (positive == 1) \
    { \
        testsuite_##suite##_tests_to_pass++; \
    } \
    else \
    { \
        char test_info[160]; \
        sprintf(test_info, "##/##/## ##:##:##.###  0 Result %s (%2d) %s\n", \
          positive ? "TestNotRunned " : "TestFailed    ", walkthrough_test_counter, #test); \
        if (positive) \
        { \
            testsuite_##suite##_tests_to_not_execute++; \
            strcat(testsuite_##suite##_list_tests_to_not_execute, test_info); \
        } \
        else \
        { \
            testsuite_##suite##_tests_to_fail++; \
            strcat(testsuite_##suite##_list_tests_to_fail, test_info); \
        } \
    } \
    if (listfile) \
    { \
        if(strcmp(local_suite_name,#suite)) \
        { \
            fprintf(listfile,"%s: \n",#suite); \
        } \
        fprintf(listfile, "(%d) [timeout: @@ sec.] %s:    ", walkthrough_test_counter, #test); \
        fprintf(listfile, "@@\n"); \
        strcpy(local_suite_name,#suite); \
    } \
    if (infofile) \
    { \
        fprintf(infofile, "(%d) %s.%s\n", walkthrough_test_counter, #suite, #test); \
        if (strstr(#test, "test_testinfo_")) \
        { \
            fprintf(infofile, "  Short description: %s\n", short_desc); \
            fprintf(infofile, "  Long  description: %s\n", long_desc); \
        } \
        else \
        { \
            fprintf(infofile, "  Short description: @@\n"); \
            fprintf(infofile, "  Long  description: @@\n"); \
        } \
    } \
    walkthrough_test_counter++;

#define SELFTEST_ADD_TEST(suite, test, setup, teardown, positive) \
    SELFTEST_REGISTER_TEST(suite, test, positive) \
    MUT_ADD_TEST(testsuite_##suite, test, setup, teardown)

#define SELFTEST_ADD_TEST_WITH_DESCRIPTION(suite, test, setup, teardown, positive, short_desc, long_desc) \
    SELFTEST_REGISTER_TEST_WITH_DESCRIPTION(suite, test, positive, short_desc, long_desc) \
    MUT_ADD_TEST_WITH_DESCRIPTION(testsuite_##suite, test, setup, teardown, short_desc, long_desc)

/** \fn static const char* get_program_name(const char * namepath)
*   \details saves the program name without path
*/
static const char* get_program_name(const char * namepath)
{
    const char * pos = namepath + strlen(namepath);

    do
    {
        pos--;
        if (pos == namepath)
        {
            return pos;
        }
    }
#ifdef ALAMOSA_WINDOWS_ENV
    while (*pos != '\\');
#else
    while (*pos != '/');
#endif /* ALAMOSA_WINDOWS_ENV - PATHS - */

    return ++pos;
}

int check_user_option_impl(void)
{
    char *value;
    
    if (!mut_option_selected("-abc"))
    {
        mut_printf(MUT_LOG_KTRACE, "-abc option argc wrong nothing returned");
        return 1;
    }
        
    
    value = mut_get_user_option_value("-xyz");
    if (value == NULL)
    {
        mut_printf(MUT_LOG_KTRACE, "-xyz option wrong nothing returned");
        return 1;

    }
    MUT_ASSERT_FALSE_MSG(strcmp(value, "0"), "-xyz arg 0 wrong, expecting 0")
    

    
    value = mut_get_user_option_value("-dbg");
    if (value == NULL || strcmp("ii", value))
    {
        mut_printf(MUT_LOG_KTRACE, "-dbg option wrong nothing returned, expecting ii");
        return 1;
    }

    return 0;
}


/** \fn __cdecl main (int argc , char ** argv)
*   \details caller for the most test suites
*/
int __cdecl main(int argc , char ** argv)
{
    Emcutil_Shell_MainApp theApp; 

	//Arguments::init(argc,argv);

    int walkthrough_test_counter = 0, check_user_option = 0;
    FILE *listfile = NULL, *infofile = NULL;
    char listfile_name[MAX_PATH], infofile_name[MAX_PATH];
    char local_suite_name[MUT_SELFTEST_SHORT_STRING_LEN] = "\0";
    int i;
    const char * const mut_selftest_usage = "mut_selftest usage";
    mut_selftest_option selftest_opt;
        
#define SELFTEST_TESTSUITE(suite, teardown, status) \
    const char testsuite_##suite##_id[] = #suite; \
    mut_testsuite_t * testsuite_##suite; \
    int testsuite_##suite##_testcnt = -1; \
    int testsuite_##suite##_tests_to_pass = 0; \
    int testsuite_##suite##_tests_to_fail = 0; \
    int testsuite_##suite##_tests_to_not_execute = 0; \
    char testsuite_##suite##_list_tests_to_fail[16000] = ""; \
    char testsuite_##suite##_list_tests_to_not_execute[16000] = "";

#include "mut_selftest_testsuites.inc"

    /* Saving the program name */
    csx_p_native_executable_path_get(program, MAX_PATH, 0);

    program_name = get_program_name(argv[0]);
#ifndef ALAMOSA_WINDOWS_ENV
#define MUT_USER_HOME_ENV_VAR "HOME"
#else
#define MUT_USER_HOME_ENV_VAR "USERPROFILE"
#endif /* ALAMOSA_WINDOWS_ENV - PATHS - */
	if (strlen(getenv(MUT_USER_HOME_ENV_VAR) + 8) > MAX_PATH) /* 8 is length of '/.mutrc' */
	{
		printf("ERROR: USERPROFILE path is too long %s\n", getenv("USERPROFILE"));
		exit(1);
	}
	else {
	/* User config file */
		sprintf(path_to_config, "%s%s.mutrc", getenv("USERPROFILE"), MUT_PATH_SEP);
	}

	

   /************************************************************************
    * The code below shall be runned as primary level of MUT_SELFTEST
    * To get selftest.exe properly running, it is required to call 
    * SELFTEST.EXE -selftest 
    ************************************************************************/
    if (argc < 2 || strcmp(argv[1], "-selftest"))
    {
        mut_testsuite_t * testsuite_main;

        if (file_exists(path_to_config))
        {
            printf("ERROR. File %s with default parameters exists.\n"
                   "Solution: This file shall be saved under another name "
                   "before running MUT selftest.", path_to_config);
            exit(1);
        }

        /* Create folder for generating and storing log files */
        system("mkdir logdir");
        system("mkdir logdir" MUT_PATH_SEP "alt");

        /* Standard */
        mut_init(argc, argv);
        testsuite_main = MUT_CREATE_TESTSUITE("mut_selftest")

#define SELFTEST_TESTSUITE(suite, teardown, status) \
    MUT_ADD_TEST(testsuite_main, test_main_##suite, NULL, teardown)
#include "mut_selftest_testsuites.inc"

        /* The following tests requires no testsuite to be run */
        MUT_ADD_TEST(testsuite_main, test_main_commandline_help, NULL, NULL)
        MUT_ADD_TEST(testsuite_main, test_main_commandline_register_user_option, NULL, NULL)
/*        
        MUT_ADD_TEST(testsuite_main, test_main_commandline_list, NULL, NULL)
        MUT_ADD_TEST(testsuite_main, test_main_commandline_initformatfile, NULL, NULL)
        //MUT_ADD_TEST(testsuite_main, test_main_xml_special_escape, NULL, NULL)
*/

        MUT_RUN_TESTSUITE(testsuite_main)

        CSX_EXIT(0);
    }

    argv += 2;
    argc -= 2;

    /* Check whether it is the user option passing test */
    if(argc >= 2 && !strcmp(argv[1], "-user_option"))
    {
        check_user_option = 1;

        MUT_ASSERT_TRUE( mut_register_user_option("-user_option", 0, FALSE, mut_selftest_usage) );
        MUT_ASSERT_TRUE( mut_register_user_option("-abc",         0, FALSE, mut_selftest_usage) );
        MUT_ASSERT_TRUE( mut_register_user_option("-xyz",         2, FALSE, mut_selftest_usage) );
        MUT_ASSERT_TRUE( mut_register_user_option("-dbg",         3, FALSE, mut_selftest_usage) );
    }

    /* Check whether it is the register user option passing test */
    if (argc >= 3 && !strcmp(argv[2], "-register_user_option"))
    {
        MUT_ASSERT_TRUE( mut_register_user_option("-register_user_option", 0, FALSE, mut_selftest_usage) );

        /* register the pre-defined user options same as fbe_test_main.c */
        for (i = 0; i < sizeof(register_user_options) / sizeof(register_user_options[0]); ++i)
        {
            MUT_ASSERT_TRUE( mut_register_user_option(register_user_options[i].option_name, register_user_options[i].least_num_of_args, register_user_options[i].show, register_user_options[i].description) );
        }
    }

    /************************************************************************
     * The code below shall be runned as secondary level MUT_SELFTEST call
     ************************************************************************/

    mut_init(argc, argv);

    /* Expected that sub-testsuites will be called with -run_testsuites <testsuite> as the first arguments mostly
     *
     *  When mut self-test executes, it is re-invoking the MUT program in a separate process
     *  MUT recognizes that it's invokation is the child process when it receives a command line of the form
     *  mut_selftest !sefltest -text -run_testsuites suiteName args
     *   When invoking self-test, MUT sets the first argument to -text, so that a log file will be created
     */
    if (argc >= 4 && !strcmp(argv[2], "-run_testsuites"))
    {
        requested_testsuite = argv[3];
    }

    if (argc >= 3 && !strcmp(argv[2], "-list"))
    {
        sprintf(listfile_name, "%s%sgoldenfile_list.log", logging_dir, MUT_PATH_SEP);

        if ((listfile = fopen(listfile_name, "w")) == NULL)
        {
            printf( "The listfile was not opened\n" );
            exit(0);
        }
    }

    if (argc >= 3 && !strcmp(argv[2], "-info"))
    {
        sprintf(infofile_name, "%s%sgoldenfile_testinfo_format.log", logging_dir, MUT_PATH_SEP);

        if ((infofile = fopen(infofile_name, "w")) == NULL)
        {
            printf( "The infofile was not opened\n" );
            exit(0);
        }
    }

    /* if it is the user option passing test, check the consistence of user options */
    if (check_user_option)
    {
       exit(check_user_option_impl()); /* SAFEBUG - need to exit instead of return because we don't cleanup resources */
    }
#define SELFTEST_TESTSUITE(suite, teardown, status) \
    testsuite_##suite = MUT_CREATE_TESTSUITE(testsuite_##suite##_id)

#include "mut_selftest_testsuites.inc"

    /* Note: With recent changes in MUT, a test failure aborts the test and stops further execution, so all failure cases are commented out  */
    
    SELFTEST_ADD_TEST(control, test_wrapper_order, test_wrapper_setup, test_wrapper_teardown, 1)
    //SELFTEST_ADD_TEST(control, test_wrapper_setup_crash, test_wrapper_setup_crash_setup, test_wrapper_setup_crash_teardown, 0)
    //SELFTEST_ADD_TEST(control, test_wrapper_test_crash, NULL, test_wrapper_test_crash_teardown, 0)

    SELFTEST_ADD_TEST(control, test_assert_called_func_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(control, test_assert_called_func_fail, NULL, NULL, 0)

    // SELFTEST_ADD_TEST(multithread, test_multithread, NULL, NULL, 0) 

    // SELFTEST_ADD_TEST(mutex, test_mutex, NULL, NULL, 1)
/*
    SELFTEST_ADD_TEST(mut_ktrace_api, test_mut_ktrace_api, NULL, NULL, 1)
    SELFTEST_ADD_TEST(mut_ktrace_api, test_mut_ktrace_set_handler, NULL, NULL, 1)
*/
    /************************
     * ASSERTS
     ************************/

    /* Note: With recent changes in MUT, a test failure aborts the test and stops further execution, so all failure cases are commented out  */
    
    SELFTEST_ADD_TEST(asserts, test_assert_true_pass, NULL, NULL,     1)
    SELFTEST_ADD_TEST(asserts, test_assert_true_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_true_fail, NULL, NULL, 0)
    //SELFTEST_ADD_TEST(asserts, test_assert_true_msg_fail, NULL, NULL, 0)


    SELFTEST_ADD_TEST(asserts, test_assert_false_pass, NULL, NULL,     1)
    SELFTEST_ADD_TEST(asserts, test_assert_false_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_false_fail, NULL, NULL, 0)
    //SELFTEST_ADD_TEST(asserts, test_assert_false_msg_fail, NULL, NULL, 0)


    //SELFTEST_ADD_TEST(asserts, test_assert_fail_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_fail_msg_fail, NULL, NULL, 0)


    SELFTEST_ADD_TEST(asserts, test_assert_null_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_null_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_null_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_null_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_not_null_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_not_null_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_not_null_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_not_null_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_pointer_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_pointer_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_pointer_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_pointer_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_pointer_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_pointer_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_pointer_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_pointer_not_equal_msg_fail, NULL, NULL,     0)
    

    SELFTEST_ADD_TEST(asserts, test_assert_int_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_int_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_int_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_int_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_int_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_int_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_int_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_int_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_char_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_char_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_char_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_char_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_char_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_char_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_char_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_char_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_string_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_string_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_equal_fail_0, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_equal_fail_1, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_equal_msg_fail_0, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_equal_msg_fail_1, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_string_not_equal_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_not_equal_fail, NULL, NULL, 0)
    SELFTEST_ADD_TEST(asserts, test_assert_string_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_string_not_equal_msg_fail, NULL, NULL, 0)


    SELFTEST_ADD_TEST(asserts, test_assert_long_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_long_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_long_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_long_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_long_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_long_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_long_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_long_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_integer_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_integer_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_integer_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_integer_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_integer_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_integer_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_integer_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_integer_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_float_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_float_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_float_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_float_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_float_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_float_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_float_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_float_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_double_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_double_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_double_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_double_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_double_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_double_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_double_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_double_not_equal_msg_fail, NULL, NULL,     0)


    SELFTEST_ADD_TEST(asserts, test_assert_buffer_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_buffer_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_buffer_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_buffer_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_buffer_not_equal_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_buffer_not_equal_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_buffer_not_equal_fail, NULL, NULL,     0)
    //SELFTEST_ADD_TEST(asserts, test_assert_buffer_not_equal_msg_fail, NULL, NULL,     0)

    SELFTEST_ADD_TEST(asserts, test_assert_condition_pass, NULL, NULL, 1)
    SELFTEST_ADD_TEST(asserts, test_assert_condition_msg_pass, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(asserts, test_assert_condition_fail, NULL, NULL, 0)
    //SELFTEST_ADD_TEST(asserts, test_assert_condition_msg_fail, NULL, NULL, 0)


    //test_timeout and timeout_config are being added in different way,
	//such that we can specify timeout of 1sec per test. 
	//If specified using SELFTEST_ADD_TEST timeout of 600s was used causing the tests to fail.

/*    
    SELFTEST_REGISTER_TEST(timeout, test_timeout_pass, 1)
	mut_add_class(testsuite_timeout,"test_timeout_pass",new CTest(test_timeout_pass,NULL,NULL,NULL,NULL),1);
    SELFTEST_REGISTER_TEST(timeout, test_timeout_fail, 0)
	mut_add_class(testsuite_timeout,"test_timeout_fail",new CTest(test_timeout_fail,NULL,NULL,NULL,NULL),1);

	SELFTEST_REGISTER_TEST(timeout_config, test_timeout_pass, 1)
	mut_add_class(testsuite_timeout_config,"test_timeout_pass",new CTest(test_timeout_pass,NULL,NULL,NULL,NULL),1);
    SELFTEST_REGISTER_TEST(timeout_config, test_timeout_fail, 0)
	mut_add_class(testsuite_timeout_config,"test_timeout_fail",new CTest(test_timeout_fail,NULL,NULL,NULL,NULL),1);

    SELFTEST_ADD_TEST(logdirname_config, test_assert_true_pass, NULL, NULL, 1);
    SELFTEST_ADD_TEST(logdirname_config, test_assert_true_fail, NULL, NULL, 0);


    SELFTEST_ADD_TEST(verbosity_suite,  test_verbosity_level, NULL, NULL, 1)
    SELFTEST_ADD_TEST(verbosity_test,   test_verbosity_level, NULL, NULL, 1)
    SELFTEST_ADD_TEST(verbosity_low,    test_verbosity_level, NULL, NULL, 1)
    SELFTEST_ADD_TEST(verbosity_medium, test_verbosity_level, NULL, NULL, 1)
    SELFTEST_ADD_TEST(verbosity_high,   test_verbosity_level, NULL, NULL, 1)
    //SELFTEST_ADD_TEST(verbosity_ktrace, test_verbosity_level, NULL, NULL, 1)

    SELFTEST_ADD_TEST_WITH_DESCRIPTION(testinfo, test_testinfo_format, NULL, NULL, 1, test_testinfo_format_short_desc, test_testinfo_format_long_desc)
    SELFTEST_ADD_TEST_WITH_DESCRIPTION(testinfo, test_testinfo_start_description, NULL, NULL, 1, test_testinfo_start_description_short_desc, test_testinfo_start_description_long_desc)

    SELFTEST_ADD_TEST(user_option,  test_user_option, NULL, NULL, 1)

    //SELFTEST_REGISTER_TEST(cplusplus, CExceptionInConstructor)
    SELFTEST_REGISTER_TEST(cplusplus, CCppTest, 0)
    mut_selftest_cpp_add_tests(testsuite_cplusplus);

    //SELFTEST_ADD_TEST(ktraceSuite, ktraceCallbackTest, NULL, NULL, 1)
*/
    if (listfile)
    {
        fclose(listfile);
    }

    if (infofile)
    {
        fclose(infofile);
    }

    if (requested_testsuite)
    {
        if (strlen(logging_dir) + strlen(requested_testsuite) + 15 > MAX_PATH)
        {
            printf("Path too long, line %d\n", __LINE__);
            exit(1);
        }
        sprintf(goldenfile_name, "%s%sgoldenfile_%s.log", logging_dir, MUT_PATH_SEP, requested_testsuite);

        if ((goldenfile = fopen(goldenfile_name, "w")) == NULL)
        {
            printf( "The goldenfile %s was not opened\n", goldenfile_name);
            exit(0);
        }
    }

#define SELFTEST_TESTSUITE(suite, teardown, level) \
    tests_failed = testsuite_##suite##_tests_to_fail; \
    tests_passed = testsuite_##suite##_tests_to_pass; \
    tests_not_executed = testsuite_##suite##_tests_to_not_execute; \
    list_tests_failed = testsuite_##suite##_list_tests_to_fail; \
    list_tests_not_executed = testsuite_##suite##_list_tests_to_not_execute; \
    suite_finish_status = NULL; \
    test_log_level = level; \
    test_run_cnt = testsuite_##suite##_testcnt - 1; \
    log_suite_begin(#suite, logging_dir); \
    MUT_RUN_TESTSUITE(testsuite_##suite) \
    log_suite_end();

#include "mut_selftest_testsuites.inc"

    if (goldenfile)
    {
        fclose(goldenfile);
    }

    exit(0);
}
