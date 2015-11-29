/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework main header
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin   initial version
 *    10/17/2007   Alexander Gorlov  and Kirill Timofeev, global rework
 *    10/25/2007   Kirill Timofeev   added support for multiple testsuites
 *    02/29/2008   Igor Bobrovnikov  new log format
 * 
 *    09/02/2010   Tweaked for C++ rewrite, all C interfaces should have remained unchanged.
 *
 *				   Nachiket Londhe   Linux Rewrite 
 **************************************************************************/

#ifndef MUT_H
#define MUT_H

#include "EmcPAL.h"
#include "EmcUTIL.h"
#include "spid_enum_types.h"

#if ! defined(MUT_REV2)

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 */
extern "C" {
#endif

/** \def MUT_API 
 *  \details This macro specify the standard C calling convention. Required for
 *   function those may communicate between MUT C core and C++
 *   extension modules.
 *   As of sept 2011, it's use is deprecated to being defined as
 *   nothing. On C4LX it will be nothing.  It can be removed
 *   completely at a later time.
 *   --------------------------------------------------
 *   As of Sept. 2012, MUT_API is being used as a dllexport/dllimport MACRO
 *   
 */

#if defined(MUT_DLL_EXPORT)
#define MUT_API CSX_MOD_EXPORT
#else
#define MUT_API CSX_MOD_IMPORT
#endif
#define MUT_CC __stdcall
#define MUT_CCV __stdcall

#include "mut_assert.h"
#include "generic_types.h"
#include "csx_ext.h"	// needed for __attribute__

/***************************************************************************
 * LOGGING RELATED STUFF START
 **************************************************************************/

enum {
    mut_version_major = 1, //changing to V1.0 for c++ implementation
    mut_version_minor = 0,
};

typedef enum mut_logging_e {
    MUT_LOG_NO_CHANGE = -1,
    MUT_LOG_SUITE_STATUS = 0,
    MUT_LOG_TEST_STATUS = 1,
    MUT_LOG_LOW = 2,
    MUT_LOG_MEDIUM = 4,
    MUT_LOG_HIGH = 8,
    MUT_LOG_KTRACE = 16
} mut_logging_t;

typedef enum mut_color_e {
    MUT_COLOR_BLACK,
    MUT_COLOR_BLUE,
    MUT_COLOR_GREEN,
    MUT_COLOR_CYAN,
    MUT_COLOR_RED,
    MUT_COLOR_MAGENTA,
    MUT_COLOR_BROWN,
    MUT_COLOR_LIGHTGREY,
    MUT_COLOR_DARKGREY,
    MUT_COLOR_LIGHTBLUE,
    MUT_COLOR_LIGHTGREEN,
    MUT_COLOR_LIGHTCYAN,
    MUT_COLOR_LIGHTRED,
    MUT_COLOR_LIGHTMAGENTA,
    MUT_COLOR_YELLOW,
    MUT_COLOR_WHITE
} mut_color_t;

/** \fn void mut_ktrace(const char *format, ...);
 *  \details
 *  this function logs the formatted string like printf as ktrace message
 *  \param format - format string like in printf
 *  \param ... - optional orguments like in printf
 */
MUT_API void MUT_CCV mut_ktrace(const char *format, ...) __attribute__((format(__printf_func__,1,2)));

/** \fn void mut_printf(mut_logging_t level, const char *format, ...)
 *  \details
 *  this function logs the formatted string like printf if it satisfies the
 *  current logging level
 *  \param level - logging level
 *  \param format - format string like in printf
 *  \param ... - optional orguments like in printf
 */
MUT_API void MUT_CCV mut_printf(const mut_logging_t level, const char *format, ...)__attribute__((format(__printf_func__,2,3)));

/** \fn void mut_log_assert(const char* file, unsigned line, const char* func, const char *fmt, ...);
 *  \param file - source filename for logging assert information
 *  \param line - source line number for logging assert information
 *  \param func - function name for logging assert information
 *  \param fmt - formatted message like in printf
 *  \details
 *  This function logs the assert information and 
 *  implement the necessary flow control corresponding with selected abort policy  
 */
MUT_API void MUT_CCV mut_log_assert(const char* file, unsigned line, const char* func, const char *fmt, ...)__attribute__((format(__printf_func__,4,5)));

/** \fn int mut_has_any_assert_failed()
    this function checks if assert failed since test started
    \return non-zero value if any assert failed 
 */
MUT_API int MUT_CC mut_has_any_assert_failed(void);

/** \fn char *mut_get_logdir()
 *  \param none
 *  \details
 *  Get the log directory
 */
MUT_API char * MUT_CC mut_get_logdir(void);

MUT_API char * MUT_CC mut_get_log_file_name(void);

/***************************************************************************
 * LOGGING RELATED STUFF END
 **************************************************************************/

/***************************************************************************
 * Vetoable Notification Definition Start
 **************************************************************************/

/** \enum mut_event_notification_e
 *  \details
 *  This enum holds constants, defining all notification messages
 */
typedef enum mut_notification_e { 
    MUT_SUITE_START = 0,
    MUT_SUITE_END,
    MUT_TEST_START,
    MUT_TEST_END
} mut_notification_t;

typedef unsigned ( *mut_notfication_callback) (mut_notification_t type, unsigned testIndex);

MUT_API void MUT_CC mut_register_event_notification_callback(mut_notfication_callback callback);

/***************************************************************************
 * Vetoable Notification Definition End
 **************************************************************************/


/***************************************************************************
 * ABORT POLICIES RELATED STUFF START
 **************************************************************************/

/** \enum mut_abort_policy_e
 *  \details
 *  This enum holds constants, defining different abort policies 
 */
typedef enum mut_abort_policy_e { /**< in case of failure */
    MUT_DEBUG_TRAP = 0,           /**< trap into debugger and continue test*/
    MUT_CONTINUE_TEST,            /**< test would continue */
    MUT_ABORT_TEST,               /**< test would be terminated, testsuite would continue */
    MUT_ABORT_TESTSUITE,          /**< test and current testsuite would be terminated, next testsuite would continue */
    MUT_ABORT_TESTEXE,            /**< exe file would exit */
    MUT_ASK_USER,                 /**< user would be asked if test should be continued or not, testsuite would continue */
} mut_abort_policy_t;

/** \fn void mut_set_current_abort_policy(const mut_abort_policy_t policy)
 *  \param policy - abort policy to set
 *  \details
 *  sets current abort policy to the given value
 */
MUT_API void MUT_CC mut_set_current_abort_policy(const mut_abort_policy_t policy);

/** \fn void mut_set_default_abort_policy(void)
 *  \details
 *  resets current abort policy to default
 */
MUT_API void MUT_CC mut_set_default_abort_policy(void);

/* macros below are supposed to be used as unified shortcuts for setting abort
 * policies
 */
#define MUT_CONTINUE_TEST_ON_FAILURE \
    mut_set_current_abort_policy(MUT_CONTINUE_TEST);

#define MUT_ABORT_TEST_ON_FAILURE \
    mut_set_current_abort_policy(MUT_ABORT_TEST);

#define MUT_ABORT_TESTSUITE_ON_FAILURE \
    mut_set_current_abort_policy(MUT_ABORT_TESTSUITE);

#define MUT_ABORT_TESTEXE_ON_FAILURE \
    mut_set_current_abort_policy(MUT_ABORT_TESTEXE);

#define MUT_ASK_USER_ON_FAILURE \
    mut_set_current_abort_policy(MUT_ASK_USER);

#define MUT_SET_DEFAULT_ABORT_POLICY \
    mut_set_default_abort_policy();

/***************************************************************************
 * ABORT POLICIES RELATED STUFF END
 **************************************************************************/

/***************************************************************************
 * MUT FUNCTIONS AND MACROS START
 **************************************************************************/

/* MUT uses void(void) functions. This is declaration for function pointers. */
typedef void (MUT_CC *mut_function_t)(void); /* No return value */
typedef void* (MUT_CC *mut_function_create_t)(void); /* Abstract pointer as return value */

/* Abstract pointer to the MUT testsuite */
typedef void * mut_testsuite_t;

/** \fn void mut_init(int argc, char **argv)
 *  \param argc - number of arguments to parse. This is the same parameter
 *  as in main(argc, argv)
 *  \param argv - array of arguments to parse. This is the same parameter
 *  as in main(argc, argv)
 *  \details
 *  This function should be invoked in the very beginning of main() function.
 *  It sets default values in the mut_test_control structure and then calls
 *  mut_parse_arguments() to parse command line parameters.
 */
MUT_API void MUT_CC mut_init(int argc, char **argv);

MUT_API void MUT_CC init_arguments(int argc, char **argv);

MUT_API void MUT_CC reset_arguments(int argc, char **argv);

/** \fn void set_global_timeout(unsigned long timeout)
 *  \param timeout - timeout in seconds 
 *  \details
 *  This functions will set the global timeout for the executable. If its not
 *  specified by the user default value of 600sec. will be used.
 */
MUT_API void MUT_CC set_global_timeout(
#ifdef __cplusplus
								unsigned long timeout = 600);
#else
								unsigned long timeout);
#endif

/** \fn void set_monitor_exit(void)
 *  \details
 *  Called to configure mut to trap to the debugger when test code unexpectedly exits
 *  The MUT control flow is a function main that invokes mut_init(), mut_run_testsuite() & exit()
 *  When set, any other calls to exit() will result in a trap to the debugger
 *  
 */
MUT_API void MUT_CC set_monitor_exit(char **argv);

/** \fn bool mut_isTimeoutSet()
 *  \details
 *  This function will check if the timeout option is provided on the command line.
 *  If it then that value will be used if not set_global_timeout will be called.
 *  
 */
MUT_API BOOL MUT_CC mut_isTimeoutSet(void);

/** \fn char * mut_get_program_name()
 *  \details
 *  This function returns the name of the executeable containg this mut test process.
 *  This function can not be used until after mut_init has processed the user command line options
 */
MUT_API char * MUT_CC mut_get_program_name(void);

/** \fn char * mut_get_cmdLine()
 *  \details
 *  This function returns a string containing the originally specified command line,
 *  excluding the program name.
 *  This function can not be used until after mut_init has processed the user command line options
 */
MUT_API char * MUT_CC mut_get_cmdLine(void);


/** \fn char * mut_get_exePath()
 *  \details
 *  This function returns a string containing the full path used to access the executable..
 *  This function can not be used until after mut_init has processed the user command line options
 */
MUT_API char * MUT_CC mut_get_exePath(void);

/** \fn void mut_isolate_tests()
 *  \details
 *  Passing TRUE to this function causes each test to run in its own process
 */
MUT_API void MUT_CC mut_isolate_tests(BOOL b);

/* This function creates new testsuite with given name. Is used in
 * MUT_CREATE_TESTSUITE macro below.
 */
MUT_API mut_testsuite_t * MUT_CC mut_create_testsuite(const char *name);
MUT_API mut_testsuite_t * MUT_CC mut_create_testsuite_advanced(const char *name, mut_function_t setup, mut_function_t tear_down);

/* This function adds a C++ based test, passed through MUT_ADD_CLASS in mut_cpp.h
 */

//void MUT_APImut_add_class(mut_testsuite_t *suite, const char *test_id, void *instance);

/* This function adds given test to the appropriate testsuite with short and
 * long description. Is used in MUT_ADD_TEST_WITH_DESCRIPTION macro below for adding C tests.
 */

MUT_API void MUT_CC mut_add_test_with_description(mut_testsuite_t *suite, \
    const char*    test_id, \
    mut_function_t test, \
    mut_function_t setup, \
    mut_function_t teardown, \
    const char* short_desc, \
    const char* long_desc);


/* This function starts execution of the given testsuite. Is used in
 * MUT_RUN_TESTSUITE macro below.
 */
MUT_API int MUT_CC mut_run_testsuite(mut_testsuite_t *suite);

/*
Returns overall execution status for program
*/
MUT_API int MUT_CC get_executionStatus(void);

/* Unified macros for adding tests and running testsuites */

/** \def MUT_CREATE_TESTSUITE(name)
 *  \param name - symbolic name of testsuite
 *  \return pointer to testsuite structure
 *  \details This macro creates testsuite with given name.
 *  Typical usage: mut_testsuite_t = MUT_CREATE_TESTSUITE("testsuite")
 */
#define MUT_CREATE_TESTSUITE(name) mut_create_testsuite(name);

/** \def MUT_CREATE_TESTSUITE_ADVANCED(name, suite_startup,
 *       suite_teardown)
 *  \param name - symbolic name of testsuite
 *  \param StartUp - function to run before the suite starts
 *         (provide NULL if not used)
 *  \param TearDown - function to run after the suite exits
 *         (provide NULL if not used)
 *  \return pointer to testsuite structure
 *  \details This macro creates testsuite with given name, a
 *  startup and or teardown. Typical usage: mut_testsuite_t =
 *  MUT_CREATE_TESTSUITE("testsuite", my_suite_startup,
 *  my_suite_teardown)
 *  \note this functionality is new and still under test
 */

#define MUT_CREATE_TESTSUITE_ADVANCED(name, StartUp, TearDown) mut_create_testsuite_advanced(name, StartUp, TearDown);

/** \def MUT_ADD_TEST(testsuite, test, startup, teardown)
 *  \param testsuite - pointer to testsuite
 *  \param test - pointer to the test function
 *  \param startup - pointer to the test startup function. NULL if there
 *  is no startup function.
 *  \param teardown - pointer to the test teardown function. NULL if there
 *  is no teardown function.
 *  \details This macro adds specified test to the appropriate testsuite
 *  Typical usage: MUT_ADD_TEST(testsuite, test001, setup, tear_down)
 *  Test, startup and teardown functions shall have the mut_function_t type
 */
#define MUT_ADD_TEST(testsuite, test, startup, teardown) \
    mut_add_test_with_description(testsuite, #test, (mut_function_t)test, (mut_function_t)startup, (mut_function_t)teardown, NULL, NULL);


/** \def MUT_ADD_TEST_WITH_DESCRIPTION(testsuite, test, startup, teardown, shrot_desc, long_desc)
 *  \param testsuite - pointer to testsuite
 *  \param test - pointer to the test function
 *  \param startup - pointer to the test startup function. NULL if there
 *  is no startup function.
 *  \param teardown - pointer to the test teardown function. NULL if there
 *  is no teardown function.
 *  \param short_desc - pointer to the short description string.
 *  NULL if there is no short description.
 *  \param long_desc - pointer to the long description string. NULL if there
 *  is no long description.
 *  \details This macro adds specified test to the appropriate testsuite with short and long description
 *  Typical usage:
 *  MUT_ADD_TEST_WITH_DESCRIPTION(testsuite,test,startup,
 *  teardown, short_desc, long_desc) Test, startup and teardown
 *  functions shall have the mut_function_t type
 */
#define MUT_ADD_TEST_WITH_DESCRIPTION(testsuite, test, startup, teardown, short_desc, long_desc) \
    mut_add_test_with_description(testsuite, #test, (mut_function_t)test,  (mut_function_t)startup, (mut_function_t)teardown, short_desc, long_desc);


/** \def MUT_RUN_TESTSUITE(testsuite)
 *  \param testsuite - pointer to testsuite
 *  \details This macro runs specified testsuite
 *  Typical usage: MUT_RUN_TESTSUITE(testsuite)
 */
#define MUT_RUN_TESTSUITE(testsuite) \
    mut_run_testsuite(testsuite);

/* This function enables/disables vectored exception handling
*/
MUT_API void MUT_CC mut_enable_framework_exception_handling(BOOL state);

/***************************************************************************
 * MUT FUNCTIONS AND MACROS END
 **************************************************************************/

/***************************************************************************
 * MUT USER OPTION FUNCTIONS AND MACROS START
 **************************************************************************/
/** \fn int mut_register_user_option(const char *option_name, int num_of_args, bool store, bool show, const char *description)
*  \param option_name       - name of the to be registered user option
*  \param least_num_of_args - number of the arguments of the user option
*  \param show              - if we need to show the user option when type "-help"
*  \param description       - description of the user option
*  \return TRUE always
*  \details
*  Register the specified user option in MUT.
*/
MUT_API BOOL MUT_CC mut_register_user_option(const char *option_name, int least_num_of_args, BOOL show, const char *description);

/** \fn char * mut_get_user_option_value(const char *option_name)
 *  \param option_name - name of the user option
 *  \return value of options if the user option exists in the command line, NULL if not
 *  \details
 *  Give the arguments of the specified user option in the tests command line.
 */
MUT_API char * MUT_CC mut_get_user_option_value(const char *option_name);

/** \fn BOOL* mut_option_selected(const char *option_name)
 *  \param option_name - name of the user option
 *  \return value of options if the user option exists in the command line, NULL if not
 *  \details
 *  Give the arguments of the specified user option in the tests command line.
 */
MUT_API BOOL MUT_CC mut_option_selected(const char *option_name);

/** \fn char * mut_get_current_suite_name(void)
 *  \return name of the current suite
 *  \details
 *  Returns the name of the suite that is currently in context 
 */
MUT_API const char * MUT_CC mut_get_current_suite_name(void);

MUT_API BOOL MUT_CC mut_get_user_option(const char *option_name, int *argc, char ***argv);
/***************************************************************************
 * MUT USER OPTION FUNCTIONS AND MACROS END
 **************************************************************************/

typedef void (MUT_CCV *mut_printf_func_t)(const char *, ...);
MUT_API mut_printf_func_t MUT_CC mut_get_user_trace_printf_func(void);

MUT_API csx_ic_id_t MUT_CC mut_get_master_ic_id (csx_void_t);

MUT_API int MUT_CC mut_get_specified_cache_size(csx_void_t);

MUT_API char* MUT_CC mut_get_specified_platform(csx_void_t);

/*!**************************************************************
 * mut_pause()
 ****************************************************************
 * @brief
 *  pend the test in order to observe the output of the test at some timepoint.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/11/2011 - Created. zphu
 *
 ****************************************************************/
MUT_API void MUT_CC mut_pause(void);


/*!**************************************************************
 * mut_set_color()
 ****************************************************************
 * @brief
 *  change the output color of the font on the screen.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/11/2011 - Created. zphu
 *
 ****************************************************************/
MUT_API void MUT_CC mut_set_color(mut_color_t color);

#ifdef __cplusplus
};
#endif

#else

#ifndef NULL
#define NULL 0
#endif

#define MUT_LOG_MAX_SIZE 1024

/***************************************************************************
 * LOGGING RELATED STUFF START
 **************************************************************************/

typedef enum mut_logging_e {
    MUT_LOG_NO_CHANGE = -1,
    MUT_LOG_SUITE_STATUS = 0,
    MUT_LOG_TEST_STATUS = 1,
    MUT_LOG_LOW = 2,
    MUT_LOG_MEDIUM = 4,
    MUT_LOG_HIGH = 8,
    MUT_LOG_KTRACE = 16
} mut_logging_t;

/** \fn void mut_printf(const char *format, ...)
 *  \details
 *  this function logs the formatted string like printf.
 *  \param format - format string like in printf
 *  \param ... - optional orguments like in printf
 */
MUT_API void   MUT_CCV mut_printf(const char *format, ...)__attribute__((format(__printf_func__,1,2)));

/** \fn void mut_log_assert(const char* file, unsigned line, const char* func, const char *fmt, ...);
 *  \param file - source filename for logging assert information
 *  \param line - source line number for logging assert information
 *  \param func - function name for logging assert information
 *  \param fmt - formatted message like in printf
 *  \details
 *  This function logs the assert information and
 *  implement the necessary flow control corresponding with selected abort policy
 */
MUT_API void   MUT_CCV mut_log_assert(const char* file, unsigned line, const char* func, const char *fmt, ...)__attribute__((format(__printf_func__,3,4)));

/***************************************************************************
 * LOGGING RELATED STUFF END
 **************************************************************************/

/***************************************************************************
 * MUT FUNCTIONS AND MACROS START
 **************************************************************************/

/** \fn void mut_init(int argc, char **argv)
 *  \param argc - number of arguments to parse. This is the same parameter
 *  as in main(argc, argv)
 *  \param argv - array of arguments to parse. This is the same parameter
 *  as in main(argc, argv)
 *  \details
 *  This function should be invoked in the very beginning of main() function.
 *  It sets default values in the mut_test_control structure and then calls
 *  mut_parse_arguments() to parse command line parameters.
 */
MUT_API void MUT_CC mut_init(int argc, char **argv);

MUT_API void MUT_CC set_global_timeout(
#ifdef __cplusplus
								unsigned long timeout = 600);
#else
								unsigned long timeout);
#endif

#endif /* MUT_REV2 Check */

#endif /* MUT_H */
