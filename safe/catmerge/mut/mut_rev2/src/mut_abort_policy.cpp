/*
 * mut_abort_policy.cpp
 *
 *  Created on: Sep 29, 2011
 *      Author: londhn
 */

#include "mut_abort_policy.h"
#include "mut_abort_policy_private.h"

#include "mut_private.h"
#include "mut_test_control.h"

#include "mut_log.h" /* For i/o critical sections */
#include "mut_options.h"
#include <stdio.h>

extern Mut_test_control* mut_control;


/** \fn void mut_set_current_abort_policy_control(const mut_abort_policy_t policy)
 *  \param policy - abort policy to set
 *  \details
 *  sets current abort policy flags abortXxxFailure to the given value
 *  does not touch the prompt flag
 */
void Mut_abort_policy::mut_set_current_abort_policy_control(const mut_abort_policy_t policy)
{
    abort_control.debugtrapOnFailure =       policy == MUT_DEBUG_TRAP;
    abort_control.abortTestOnFailure =       policy == MUT_ABORT_TEST;
    abort_control.abortSuiteOnFailure =      policy == MUT_ABORT_TESTSUITE;
    abort_control.abortExecutableOnFailure = policy == MUT_ABORT_TESTEXE;

    return;
}

void   mut_set_current_abort_policy(const mut_abort_policy_t policy) // wrapper for C code and macro calls.
{
    mut_control->mut_abort_policy->mut_set_current_abort_policy(policy);
}


/* doxygen documentation for this function can be found in mut.h */
void Mut_abort_policy::mut_set_current_abort_policy(const mut_abort_policy_t policy)
{
//    mut_mutex_acquire(&abort_lock);
    mut_set_current_abort_policy_control(policy);

    abort_control.prompt = policy == MUT_ASK_USER;
//    mut_mutex_release(&abort_lock);

    return;
}

void   mut_set_default_abort_policy(void){
    mut_control->mut_abort_policy->mut_set_default_abort_policy();
}

/* doxygen documentation for this function can be found in mut.h */
void Mut_abort_policy::mut_set_default_abort_policy(void)
{
    mut_set_current_abort_policy(abort_control.default_abort_policy);

    return;
}

/* doxygen documentation for this function can be found in mut.h */
void Mut_abort_policy::mut_abort_init()
{
    mut_set_default_abort_policy();

    return;
}

/** \fn static void mut_ask_user_policy(void)
 *  \details
 *  This function asks user what to do in case abort policy is
 *  MUT_ASK_USER
 */
void Mut_abort_policy::mut_ask_user_policy(void)
{
    char answer;

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
bool Mut_abort_policy::mut_abort_suite_on_failure(void)
{
    return abort_control.abortSuiteOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
bool Mut_abort_policy::mut_abort_executable_on_failure(void)
{
    return abort_control.abortExecutableOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
bool Mut_abort_policy::mut_debug_executable_on_failure(void)
{
    return abort_control.debugtrapOnFailure;
}

/* doxygen documentation for this function can be found in mut.h */
bool Mut_abort_policy::mut_abort_test(void)
{
    /* Avoid other threads may interrupt this prompt */
//    mut_mutex_acquire(&abort_lock);
    mut_ask_user_policy();
//    mut_mutex_release(&abort_lock);

    return abort_control.abortTestOnFailure  ||
           abort_control.abortSuiteOnFailure ||
           abort_control.abortExecutableOnFailure;
}



