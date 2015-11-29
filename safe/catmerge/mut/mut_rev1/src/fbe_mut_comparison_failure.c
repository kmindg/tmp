#include "fbe_mut_comparison_failure.h"

/* XXX - temporary hacks */
void fbe_mut_integral_comparison_failure(int expected, int actual, int line, const char* file)
{
    mut_printf(MUT_LOG_TEST_STATUS, "\ntest failure:\n\t%s line: %d \n\texpected: %d\n\treceived: %d\n", file, line, expected, actual);       
}

void fbe_mut_ptr_comparison_failure(void* expected, void* actual, int line, const char* file)
{
    mut_printf(MUT_LOG_TEST_STATUS, "\ntest failure:\n\t%s line: %d \n\texpected: %d\n\treceived: %d\n", file, line, expected, actual);       
}

void fbe_mut_string_comparison_failure(const char* expected, const char* actual, int line, const char* file) 
{
    mut_printf(MUT_LOG_TEST_STATUS, "\ntest failure:\n\t%s line: %d \n\texpected: %s\n\treceived: %s \n", file, line, expected, actual);    
}

void fbe_mut_binary_data_comparison_failure(const char* expected, const char* actual, int size, int line, const char* file) 
{
    int i;
    mut_printf(MUT_LOG_TEST_STATUS, "\ntest failure:\n\t%s line: %d\n\texpected:  ", file, line);
    for(i = 0 ; i < size; i++) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s", expected[i]);
    }
    
    mut_printf(MUT_LOG_TEST_STATUS,"\t received: ");
    for(i = 0 ; i < size; i++) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s", actual[i]);
    }
}

void fbe_mut_log_failure(const char* message, int line, const char*file)
{
    mut_printf(MUT_LOG_TEST_STATUS, "\ntest tailure:\n\t%s line: %d: failure: %s\n", file, line, message);          
}
