/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_selftest_testsuites.inc
 ***************************************************************************
 *
 * DESCRIPTION: MUT Selftest testsuites definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/07/2007   Igor Bobrovnikov (bobroi) initial version
 *
 **************************************************************************/

SELFTEST_TESTSUITE(control,     NULL,    MUT_LOG_TEST_STATUS)

/*
SELFTEST_TESTSUITE(multithread, NULL,    MUT_LOG_TEST_STATUS)
SELFTEST_TESTSUITE(mutex,       NULL,    MUT_LOG_TEST_STATUS)
*/

/*  Ktrace is still not implemented correctly
SELFTEST_TESTSUITE(mut_ktrace_api, NULL, MUT_LOG_TEST_STATUS)
*/

SELFTEST_TESTSUITE(asserts, NULL,               MUT_LOG_TEST_STATUS)

/*
SELFTEST_TESTSUITE(timeout, NULL,               MUT_LOG_TEST_STATUS)
SELFTEST_TESTSUITE(timeout_config, test_main_timeout_config_teardown, MUT_LOG_TEST_STATUS)
SELFTEST_TESTSUITE(logdirname_config, test_main_timeout_config_teardown, MUT_LOG_TEST_STATUS)

SELFTEST_TESTSUITE(verbosity_suite,    NULL, MUT_LOG_SUITE_STATUS)
SELFTEST_TESTSUITE(verbosity_test,     NULL, MUT_LOG_TEST_STATUS)
SELFTEST_TESTSUITE(verbosity_low,      NULL, MUT_LOG_LOW)
SELFTEST_TESTSUITE(verbosity_medium,   NULL, MUT_LOG_MEDIUM)
SELFTEST_TESTSUITE(verbosity_high,     NULL, MUT_LOG_HIGH)
//SELFTEST_TESTSUITE(verbosity_ktrace, NULL, MUT_LOG_KTRACE)

SELFTEST_TESTSUITE(testinfo, NULL,      MUT_LOG_TEST_STATUS)
SELFTEST_TESTSUITE(user_option, NULL,      MUT_LOG_TEST_STATUS)

SELFTEST_TESTSUITE(cplusplus, NULL,         MUT_LOG_TEST_STATUS)

//SELFTEST_TESTSUITE(ktraceSuite, NULL, MUT_LOG_TEST_STATUS)
*/

#undef SELFTEST_TESTSUITE
