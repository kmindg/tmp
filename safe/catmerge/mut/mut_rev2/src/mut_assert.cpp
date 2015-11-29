#include <stdio.h>
//#include <math.h>
#include <string.h>

#include "mut_test_control.h"
#include <fstream>

using namespace std;

extern Mut_test_control *mut_control;

char result[MUT_LOG_MAX_SIZE];


/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_false(const char* file, unsigned line, const char* func,
    bool condition, const char * s_condition, const char * msg)
{
    if (!condition)
    {
        sprintf(result,"MUT_ASSERT_FALSE_MSG failure: %s User info: %s", 
		file, line, func, s_condition, msg);
		mut_control->mut_log->mut_log_assert(result);
    }

 return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_true(const char* file, unsigned line, const char* func,
    bool condition, const char * s_condition, const char * msg)
{
    if (!condition)
    {
		sprintf(result,"MUT_ASSERT_TRUE_MSG failure: %s User info: %s", 
		file, line, func, s_condition, msg);
		mut_control->mut_log->mut_log_assert(result);
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_fail(const char* file, unsigned line, const char* func, const char* msg)
{
    sprintf(result, "Assert MUT_FAIL_MSG User info: %s", file, line, func, msg);
	mut_control->mut_log->mut_log_assert(result);
}


/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_null(const char* file, unsigned line, const char* func,
    const void* ptr,const char* msg)
{
	if (ptr != NULL)
	{
	 	sprintf(result,"%s-%d MUT_ASSERT_NULL failure: pointer is %p, User info: %s", file, line, ptr, msg);
		mut_control->mut_log->mut_log_assert(result);
	}
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_null(const char* file, unsigned line, const char* func,
    const void* ptr, const char *msg)
{
    if (ptr == NULL)
    {
    	sprintf(result,"%s-%f MUT_ASSERT_NOT_NULL_MSG failure: pointer is NULL User info: %s",file, line, msg);
		mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    unsigned int expected, unsigned int actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: %u != %u, 0x%X != 0x%X User info: %s",
            expected, actual, expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
	int expected, int actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: %d != %d, 0x%X != 0x%X User info: %s",
            expected, actual, expected, actual, msg);
		mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    float expected, float actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    char expected, char actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: Expected '%c' Actual '%c' User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    double expected, double actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    const char* expected, const char* actual, const char* msg)
{
    if (strcmp(expected,actual) != 0)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: Expected \"%s\" , Actual \"%s\" User info: %s", 
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    const void* expected, const void* actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: %p != %p, User info: %s", 
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_equal(const char* file, unsigned line, const char* func,
    unsigned __int64 expected, unsigned __int64 actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_EQUAL_MSG failure: %llu != %llu, 0x%llX != 0x%llX User info: %s",
            (unsigned long long)expected, (unsigned long long)actual,
	    (unsigned long long)expected, (unsigned long long)actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    unsigned int expected, unsigned int actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: %u != %u, 0x%X != 0x%X User info: %s",
            expected, actual, expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
	int expected, int actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: %d != %d, 0x%X != 0x%X User info: %s",
            expected, actual, expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    float expected, float actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    char expected, char actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: Expected '%c' Actual '%c' User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    double expected, double actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    const char* expected, const char* actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: Expected \"%s\" , Actual \"%s\" User info: %s", 
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    const void* expected, const void* actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: %p != %p, User info: %s", 
			expected, actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}

/*doxygen documentation for this function can be found in mut_assert.h*/
MUT_API void mut_assert_not_equal(const char* file, unsigned line, const char* func,
    unsigned __int64 expected, unsigned __int64 actual, const char* msg)
{
    if (expected != actual)
    {
        sprintf(result,file, line, func,
            "MUT_ASSERT_NOT_EQUAL_MSG failure: %llu != %llu, 0x%llX != 0x%llX User info: %s",
            (unsigned long long)expected, (unsigned long long)actual,
	    (unsigned long long)expected, (unsigned long long)actual, msg);
        mut_control->mut_log->mut_log_assert(result);
    }
}
