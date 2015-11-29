/*
 * mut_abort_policy.h
 *
 *  Created on: Sep 29, 2011
 *      Author: londhn
 */

#ifndef MUT_ABORT_POLICY_H_
#define MUT_ABORT_POLICY_H_

#include "mut.h"
#include "mut_options.h"
#include "mut_sdk.h"

/* This structure holds abort policies related fields */
class mut_abort_control_t {     /* 3 boolean flags. If set ... */
public:
    bool volatile debugtrapOnFailure;       /**< executable should be aborted on failure */
    bool volatile abortTestOnFailure;       /**< test should be aborted on failure */
    bool volatile abortSuiteOnFailure;      /**< suite should be aborted on failure */
    bool volatile abortExecutableOnFailure; /**< executable should be aborted on failure */

    bool volatile prompt;                   /**< user should be prompted what to do */

    mut_abort_policy_t default_abort_policy; /**< Default abort policy. Can be
                                   * used for restoring current abort policy. */
};

// implementation in mut_abort_policy.c
class Mut_abort_policy {

    void mut_set_current_abort_policy_control(const mut_abort_policy_t policy);
    void mut_ask_user_policy();

    mut_abort_control_t abort_control;
    mut_abortpolicy_option abortpolicy;

public:
    void mut_abort_init();
    bool mut_abort_suite_on_failure(void);
    bool mut_abort_executable_on_failure(void);
    bool mut_debug_executable_on_failure(void);
    bool mut_abort_test(void);
    void mut_set_current_abort_policy(const mut_abort_policy_t policy);
    void  mut_set_default_abort_policy(void);
    Mut_abort_policy();
};

#endif /* MUT_ABORT_POLICY_H_ */
