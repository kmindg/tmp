/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_selftest_tools.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Self Test tools interface
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/29/2007   Igor Bobrovnikov  Initial version
 *
 **************************************************************************/

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


#ifndef MUT_SELFTEST_TOOLS_H
#define MUT_SELFTEST_TOOLS_H

#define MUT_SELFTEST_SHORT_STRING_LEN 128
extern const char * const CHECKPOINT_FMT;
extern const char * const EXPECTED_FMT;
extern const char * const UNEXPECTED_FMT;
extern const char * const DISABLED_CODE_FMT;
extern const char * const LOG_STATUS_PASSED;
extern const char * const LOG_STATUS_FAILED;
extern const char * const LOG_STATUS_ERROR_EXCEPTION;
extern const char * const LOG_STATUS_TIMEOUT;

extern const char * const mut_fail_fmt;

void golden_printf(mut_logging_t level, const char * const fmt, ...) __attribute__((format(__printf_func__,2,3)));
/*void golden_append(const char * const fmt, ...);*/
void log_test_begin(const char* const test);
void log_test_end(const char* const test, const char* const status);
void log_suite_begin(const char * suite, const char * subdir);
void log_suite_end(void);
void log_test_assert(const char* fmt, const char* id, int usermsg, const char* func, const char* file, unsigned line);

/*
 * Commonly used defines
 */
#define DISABLED_CODE mut_printf(MUT_LOG_SUITE_STATUS, DISABLED_CODE_FMT, __LINE__);
#define CHECKPOINT mut_printf(MUT_LOG_SUITE_STATUS, CHECKPOINT_FMT, __LINE__);

#define LOG_TEST_BEGIN log_test_begin(__FUNCTION__);
#define LOG_TEST_END(passed) log_test_end(__FUNCTION__, passed);

#define LOG_TEST_MSG(msg) golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Body   MutPrintf      %s\n", msg);
#define LOG_TEST_CHECKPOINT(lineofs) golden_printf(MUT_LOG_SUITE_STATUS, "##/##/## ##:##:##.###  0 Body   MutPrintf      CHECKPOINT: correct code execution, this string shall be printed, line %u\n", __LINE__ + lineofs);

#define LOG_TEST_ASSERT(fmt, id, ofs) log_test_assert(fmt, id, 0, __FUNCTION__,  __FILE__, __LINE__ + (ofs));
#define LOG_TEST_ASSERT_MSG(fmt, id, ofs) log_test_assert(fmt, id, 1, __FUNCTION__, __FILE__, __LINE__ + (ofs));

#define LOG_TEST_POSITIVE(ofs) \
    LOG_TEST_BEGIN \
    LOG_TEST_CHECKPOINT(ofs) \
    LOG_TEST_END(LOG_STATUS_PASSED)

#define LOG_TEST_NEGATIVE(msg, id, ofs) \
    LOG_TEST_BEGIN \
    LOG_TEST_ASSERT(msg, id, ofs) \
    LOG_TEST_END(LOG_STATUS_FAILED)

#define LOG_TEST_NEGATIVE_MSG(msg, id, ofs) \
    LOG_TEST_BEGIN \
    LOG_TEST_ASSERT_MSG(msg, id, ofs) \
    LOG_TEST_END(LOG_STATUS_FAILED)

#endif /* MUT_SELFTEST_TOOLS_H */
