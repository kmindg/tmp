#ifndef MUT_ABORT_POLICY_H
#define MUT_ABORT_POLICY_H
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
    MUT_API mut_abort_control_t();
};

// implementation in mut_abort_policy.c
class Mut_abort_policy {
    
    MUT_API void mut_set_current_abort_policy_control(const mut_abort_policy_t policy);
    MUT_API void mut_ask_user_policy();

    mut_abortpolicy_option abortpolicy; // option
    mut_abort_control_t abort_control;
    mut_mutex_t abort_lock;

public:
    MUT_API void mut_abort_init();
    MUT_API bool mut_abort_suite_on_failure(void);
    MUT_API bool mut_abort_executable_on_failure(void);
    MUT_API bool mut_debug_executable_on_failure(void);
    MUT_API bool mut_abort_test(void);
    MUT_API void mut_set_current_abort_policy(const mut_abort_policy_t policy);
    MUT_API void  mut_set_default_abort_policy(void);
    MUT_API Mut_abort_policy();
};
#endif
