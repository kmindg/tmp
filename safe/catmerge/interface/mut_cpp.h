/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_cpp.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT/C++ extension
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2007   Igor Bobrovnikov (bobroi) Initial version
 *    10/2010       Reworked for true C++ implementation JD
 *
 **************************************************************************/

#ifndef MUT_CPP_H
#define MUT_CPP_H

#if ! defined(MUT_REV2)

/* This in an extension based on MUT Framework in C */
#include "mut.h"
/* Ctest and CAbstractTest classes */
#include "cabstracttest.h"

/* Mut_testsuite class */
#include "mut_testsuite.h"

//Teardown timeout for MUT_tests 300sec.
#define TEARDOWN_TIMEOUT 300

//Default timeout for MUT_tests 600sec.
#define DEFAULT_TIMEOUT 600

#define MUT_ADD_CLASS_DEFAULT_TIMEOUT_IN_SECONDS ((unsigned long) 0xFFFFFFFF)

//If timeout per test is not specified its defaults to -1 (infinite)
void MUT_API mut_add_class(mut_testsuite_t *suite, const char *test_id, CAbstractTest *instance, unsigned long timeOutInSeconds = MUT_ADD_CLASS_DEFAULT_TIMEOUT_IN_SECONDS, const char* sd = NULL, const char* ld = NULL);
void MUT_API mut_add_class(Mut_testsuite *suite, const char *test_id, CAbstractTest *instance, unsigned long timeOutInSeconds = MUT_ADD_CLASS_DEFAULT_TIMEOUT_IN_SECONDS, const char* sd = NULL, const char* ld = NULL);

#define MUT_ADD_TEST_CLASS(testsuite, class_id) \
    mut_add_class(testsuite, #class_id, (CAbstractTest*)(new class_id()));

#define MUT_ADD_TEST_CLASS_TIMEOUT(testsuite, class_id, timeOutInSeconds) \
	mut_add_class(testsuite, #class_id, (CAbstractTest*)(new class_id()), (unsigned long)timeOutInSeconds);

#define MUT_ADD_TEST_CLASS_WITH_DESCRIPTION(testsuite, class_id, sd, ld) \
    mut_add_class(testsuite, #class_id, (CAbstractTest*)(new class_id()), MUT_ADD_CLASS_DEFAULT_TIMEOUT_IN_SECONDS, sd, ld);

#define MUT_ADD_TEST_CLASS_TIMEOUT_WITH_DESCRIPTION(testsuite, class_id, timeOutInSeconds, sd, ld) \
    mut_add_class(testsuite, #class_id, (CAbstractTest*)(new class_id()), (unsigned long)timeOutInSeconds, sd, ld);

#endif

#endif /* MUT_CPP_H */
