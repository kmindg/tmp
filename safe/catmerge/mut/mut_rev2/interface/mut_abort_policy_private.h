/*
 * mut_abort_policy_private.h
 *
 *  Created on: Sep 29, 2011
 *      Author: londhn
 */

#ifndef MUT_ABORT_POLICY_PRIVATE_H_
#define MUT_ABORT_POLICY_PRIVATE_H_


#ifdef __cplusplus
extern "C" {
#endif

/** \fn void mut_abort_init(void)
 *  \details
 *  This function initializes abort_control structure
 */
void mut_abort_init(void);

/** \fn int mut_abort_test(void)
 *  \details
 *  Returns true if test should be aborted after assert failure
 */
int mut_abort_test(void);

/** \fn int mut_abort_executable_on_failure(void)
 *  \details
 *  Returns true if exe should be terminated on assert failure
 */
int mut_abort_executable_on_failure(void);

/** \fn int mut_abort_suite_on_failure(void)
 *  \details
 *  Returns true if testsuite should be terminated on assert failure
 */
int mut_abort_suite_on_failure(void);

/** \fn int mut_debug_executable_on_failure(void)
 *  \details
 *  Returns true if VisualStudio debugger shall be invoked on assert failure
 */
int mut_debug_executable_on_failure(void);

/** \fn void print_abortpolicy_options(void)
 *  \details
 *  This function prints abort policy specific help
 */
void print_abortpolicy_options(void);

/** \fn void set_abortpolicy(char **argv)
 *  \param argv - command line
 *  \details
 *  Parses argument of -abortpolicy option and sets appropriate abort policy
 */
void set_option_abortpolicy(char **argv);

#ifdef __cplusplus
}
#endif

#endif /* MUT_ABORT_POLICY_PRIVATE_H_ */
