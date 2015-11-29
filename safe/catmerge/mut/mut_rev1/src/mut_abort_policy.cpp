/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_abort_policy.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework abort policies
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *   11/20/2007 Kirill Timofeev  initial version
 *   01/18/2007 Igor Bobrovnikov  trap into debugger fixed
 *   09/02/2010 Updated for C++ implementation
 **************************************************************************/
#include "mut_abort_policy.h"
#include "mut_abort_policy_private.h"

#include "mut_private.h"
#include "mut_test_control.h"

#include "mut_log.h" /* For i/o critical sections */
#include "mut_options.h"
#include <stdio.h>
#include <windows.h>

extern Mut_test_control* mut_control;

MUT_API mut_abort_control_t::mut_abort_control_t(): debugtrapOnFailure(false), abortTestOnFailure(false), abortSuiteOnFailure(false),
    abortExecutableOnFailure(false), prompt(false), default_abort_policy(MUT_DEBUG_TRAP) {}


MUT_API Mut_abort_policy::Mut_abort_policy(){
    csx_ci_module_disable_resource_accounting(CSX_MY_MODULE_CONTEXT()); 
    mut_mutex_init(&abort_lock);    /* SAFEBUG - need to destroy */
    csx_ci_module_enable_resource_accounting(CSX_MY_MODULE_CONTEXT());
}

/** \fn void mut_set_current_abort_policy_control(const mut_abort_policy_t policy)
 *  \param policy - abort policy to set
 *  \details
 *  sets current abort policy flags abortXxxFailure to the given value
 *  does not touch the prompt flag
 */
MUT_API void Mut_abort_policy::mut_set_current_abort_policy_control(const mut_abort_policy_t policy)
{
    abort_control.debugtrapOnFailure =       policy == MUT_DEBUG_TRAP;
    abort_control.abortTestOnFailure =       policy == MUT_ABORT_TEST;
    abort_control.abortSuiteOnFailure =      policy == MUT_ABORT_TESTSUITE;
    abort_control.abortExecutableOnFailure = policy == MUT_ABORT_TESTEXE;

    return;
}

MUT_API void mut_set_current_abort_policy(const mut_abort_policy_t policy) // wrapper for C code and macro calls.
{
    mut_control->mut_abort_policy->mut_set_current_abort_policy(policy);
}


/* doxygen documentation for this function can be found in mut.h */
MUT_API void Mut_abort_policy::mut_set_current_abort_policy(const mut_abort_policy_t policy)
{
    mut_mutex_acquire(&abort_lock);
    mut_set_current_abort_policy_control(policy);

    abort_control.prompt = policy == MUT_ASK_USER;
    mut_mutex_release(&abort_lock);

    return;
}

MUT_API void mut_set_default_abort_policy(void){
    mut_control->mut_abort_policy->mut_set_default_abort_policy();
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API void Mut_abort_policy::mut_set_default_abort_policy(void)
{
    mut_set_current_abort_policy(abort_control.default_abort_policy);

    return;
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API void Mut_abort_policy::mut_abort_init()
{
    abort_control.default_abort_policy = abortpolicy.get();
    mut_set_default_abort_policy();

    return;
}

/** \fn static void mut_ask_user_policy(void)
 *  \details
 *  This function asks user what to do in case abort policy is
 *  MUT_ASK_USER
 */
MUT_API void Mut_abort_policy::mut_ask_user_policy(void)
{
    int answer;

    if (!abort_control.prompt)
    {
        return;
    }

    do
    {
        /* Prompt the user until he get the correct answer */
        printf("Continue(c), Trap into debugger(d), "
               "Abort test(t), Abort testsuite(s), Abort executable(e)?\n");
        answer = getchar();
        switch (answer)
        {
            case 'd': case 'D':  /* User choose to trap into debugger */
                mut_set_current_abort_policy_control(MUT_DEBUG_TRAP);
                return;

           case 'c': case 'C': /* User choose continue test execution */
                mut_set_current_abort_policy_control(MUT_CONTINUE_TEST);
                return;

            case 't': case 'T': /* User choose terminate current test */
                mut_set_current_abort_policy_control(MUT_ABORT_TEST);
                return;

            case 's': case 'S': /* User choose terminate current test suite */
                mut_set_current_abort_policy_control(MUT_ABORT_TESTSUITE);
                return;

            case 'e': case 'E':  /* User choose terminate test executable */
                mut_set_current_abort_policy_control(MUT_ABORT_TESTEXE);
                return;
        }
    } while (1); /* Loop forever */

    return;
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API bool Mut_abort_policy::mut_abort_suite_on_failure(void)
{
    return abort_control.abortSuiteOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API bool Mut_abort_policy::mut_abort_executable_on_failure(void)
{
    return abort_control.abortExecutableOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API bool Mut_abort_policy::mut_debug_executable_on_failure(void)
{
    return abort_control.debugtrapOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
MUT_API bool Mut_abort_policy::mut_abort_test(void)
{
    /* Avoid other threads may interrupt this prompt */
    mut_mutex_acquire(&abort_lock);
    mut_ask_user_policy();
    mut_mutex_release(&abort_lock);

    return abort_control.abortTestOnFailure  ||
           abort_control.abortSuiteOnFailure ||
           abort_control.abortExecutableOnFailure;
}
