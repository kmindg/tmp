#ifndef FBE_MUT_COMPARISON_FAILURE_H_
#define FBE_MUT_COMPARISON_FAILURE_H_

#include "fbe_mut_include.h"

void fbe_mut_integral_comparison_failure(int expected, int actual, int line, const char* file);
void fbe_mut_ptr_comparison_failure(void* expected, void* actual, int line, const char* file);
void fbe_mut_string_comparison_failure(const char* expected, const char* actual, int line, const char* file);
void fbe_mut_binary_data_comparison_failure(const char* expected, const char* actual, int size, int line, const char* file);
void fbe_mut_log_failure(const char* message, int line, const char*file);

#endif /*FBE_MUT_COMPARISON_FAILURE_H_*/
