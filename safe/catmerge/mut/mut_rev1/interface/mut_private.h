/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_private.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework internal data structures and API
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/21/2007 Kirill Timofeev initial version
 *    09/03/2010 Updated for C++ implementation
 **************************************************************************/

#ifndef MUT_PRIVATE_H
#define MUT_PRIVATE_H

#include "mut.h"
#include "mut_cpp.h"
#include <vector>
using namespace std;
#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 */
extern "C" {
#endif

#include "mut_sdk.h"
//#include <windef.h> // provides MAX_PATH among other things to the mut framework

//#define INFINITE            0xFFFFFFFF  // Infinite timeout (needs to come from system file,but don't want to include windows.h,
// have to find appropriate alternative

#define NO_DESC_MSG "<no description>"

/* Reserved length for MUT strings */

#define    MUT_STRING_LEN  8192
#define 	MUT_SHORT_STRING_LEN 128


/** \enum mut_execution_e
 *  \details
 *  2 modes of execution. If MUT_TEST_EXECUTE mode is set real test run is
 *  performed. If MUT_TEST_LIST mode is set (via -list command line option)
 *  test names and numbers are printed, tests are not run.
 */
typedef enum mut_execution_e {
    MUT_TEST_EXECUTE = 0,
    MUT_TEST_LIST    = 1,
    MUT_TEST_INFO = 2
} mut_execution_t;

/** \fn void mut_process_test_failure(void)
 *  \details This function contains common functionality that is used in case assert fails.
 */
void mut_process_test_failure(void);

/** \fn void CleanupAndExitNow()
 *  \details
 *  This function is called to cleanup, close the log, and exit
 */
void CleanupAndExitNow();

/** \def MUT_UNREFERENCED_PARAMETER(p)
 *  \param p - the parameter that is not referenced in a function
 *  \details This macro supress the waring message for a specific unreferenced parameter
 */
#define MUT_UNREFERENCED_PARAMETER(p) (void) (p);

/** \def MUT_INTERNAL_ERROR(msg)
 *  \param msg - additional user information, like in printf 
 *  \details This macro shall be used in case of MUT internal error that
 *  \         requires to stop the MUT execution. It will be exit with code 1
 */
#define MUT_INTERNAL_ERROR(msg) \
    printf("[ MUT internal error, function %s\n", __FUNCTION__); \
    printf msg; \
    printf("]\n"); \
    exit(1);

#define MUT_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof(array[0]))


/** \fn int mut_get_exception_filter(void)
    \details return the exception handling filter depend current abort strategy
*/
csx_status_e mut_get_exception_filter(void);


#ifdef __cplusplus
};
#endif

#endif /* MUT_PRIVATE_H */
