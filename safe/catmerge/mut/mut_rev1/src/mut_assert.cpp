/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_assert.cp
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework asserts
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/07/2007   Igor Bobrovnikov  Initial version
 *    01/10/2008   Igor Bobrovnikov (bobroi) 
 *                 epsilon argument for float and double compare added
 *    29/08/2008   Alexander Alanov (alanoa)
 *                 Implemented mut_assert_pointer_* functions (required for  
 *                 respective MUT_ASSERT_POINTER_* macros)
 *
 **************************************************************************/

/************************
 * INCLUDE FILES
 ************************/

#include "mut_private.h"
#include "mut_log_private.h"
#include "mut_test_control.h"


#include <stdarg.h>
#include <stdio.h>
#include <float.h>
#include <time.h>
#include <windows.h>
#include <math.h>

extern Mut_test_control *mut_control;

/** \fn int compare_mem_buffer(const char * const left,
 *  const char * const right, const int size_of_left, int
 *  * const first_difference)
 *  \param left - the first buffer for comparison
 *  \param right -  the second buffer for comparison
 *  \param size_of_left - size of left buffer
 *  \param first_difference - the offset of the firat difference
 *  \return boolean flag true in case buffers is differ 
 *  \details compare two buffers and return offset of the first difference.
 *  User is responsible for passing approprited size of buffer
 *  otherwise memory protection exception can be occured    
 */
int compare_mem_buffer(const char * const left,
        const char * const right, const int size_of_left,
        int * const first_difference)
{
    int offset = 0;
    while ( offset < size_of_left)
    {
        if (*(left+offset) != *(right + offset) )
        {
            *first_difference = offset;
            break;
        }
        offset++;
    }
    return offset == size_of_left ? FALSE : TRUE;
}

/** \fn float mut_fabsf(float float_number)
 *  \param float_number - value we want to get absolute value of
 *  \return absolute value of the argument
 *  \details
 *  Used in asserts comparing float numbers.
 */
float mut_fabsf(float float_number)
{
    return float_number < 0 ? -float_number : float_number;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_false(const char* file, unsigned line, const char* func,
    int condition, const char * s_condition)
{
    if (!condition)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_FALSE failure: %s",
            s_condition);
    }

 return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_false_msg(const char* file, unsigned line, const char* func,
    int condition, const char * s_condition, const char * msg)
{
    if (!condition)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_FALSE_MSG failure: %s User info: %s",
            s_condition, msg);
    }

 return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_true(const char* file, unsigned line, const char* func,
    int condition, const char * s_condition)
{
    if (!condition)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_TRUE failure: %s", 
            s_condition);
    }

 return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_true_msg(const char* file, unsigned line, const char* func,
    int condition, const char * s_condition, const char * msg)
{
    if (!condition)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_TRUE_MSG failure: %s User info: %s", 
            s_condition, msg);
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_fail(const char* file, unsigned line, const char* func)
{
    mut_control->mut_log->mut_log_assert(file, line, func, "Assert MUT_FAIL");
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_fail_msg(const char* file, unsigned line, const char* func,
                  const char* msg)
{
    mut_control->mut_log->mut_log_assert(file, line, func, 
        "Assert MUT_FAIL_MSG User info: %s", 
        msg);
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_null(const char* file, unsigned line, const char* func,
    const void* ptr)
{
    if (ptr != NULL)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_NULL failure: pointer is %p", 
            ptr);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_null_msg(const char* file, unsigned line, const char* func,
    const void* ptr, const char* msg)
{
    if (ptr != NULL)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_NULL_MSG failure: pointer is %p User info: %s", 
            ptr, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_not_null(const char* file, unsigned line, const char* func,
    const void* ptr)
{
    if (ptr == NULL)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_NOT_NULL failure: pointer is NULL");
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_not_null_msg(const char* file, unsigned line, const char* func,
    const void* ptr, const char* msg)
{
    if (ptr == NULL)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_NOT_NULL_MSG failure: pointer is NULL User info: %s", 
            msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_pointer_equal(const char* file, unsigned line, const char* func,
    const void* expected, const void* actual)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_POINTER_EQUAL failure: %p != %p", 
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_pointer_equal_msg(const char* file, unsigned line, const char* func,
    const void* expected, const void* actual, const char* msg)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_POINTER_EQUAL_MSG failure: %p != %p User info: %s", 
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_pointer_not_equal(const char* file, unsigned line, const char* func,
    const void* expected, const void* actual)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_POINTER_NOT_EQUAL failure: %p == %p",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_pointer_not_equal_msg(
    const char* file, unsigned line, const char* func,
    const void* expected, const void* actual, const char* msg)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_POINTER_NOT_EQUAL_MSG failure: %p == %p User info: %s",
            expected, actual, msg);
    }
}



/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_int_equal(const char* file, unsigned line, const char* func,
    int expected, int actual)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_INT_EQUAL failure: %d != %d, 0x%X != 0x%X", 
            expected, actual, expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_int_equal_msg(const char* file, unsigned line, const char* func,
    int expected, int actual, const char* msg)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_INT_EQUAL_MSG failure: %d != %d, 0x%X != 0x%X User info: %s", 
            expected, actual, expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_int_not_equal(const char* file, unsigned line, const char* func,
    int expected, int actual)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_INT_NOT_EQUAL failure: %d == %d, 0x%X == 0x%X",
            expected, actual, expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_int_not_equal_msg(
    const char* file, unsigned line, const char* func,
    int expected, int actual, const char* msg)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_INT_NOT_EQUAL_MSG failure: %d == %d, 0x%X == 0x%X User info: %s",
            expected, actual, expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_long_equal(const char* file, unsigned line, const char* func,
    long expected, long actual)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_LONG_EQUAL failure: %d != %d, 0x%X != 0x%X",
            (int)expected, (int)actual, (unsigned int)expected, (unsigned int)actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_long_equal_msg(
    const char* file, unsigned line, const char* func,
    long expected, long actual, const char* msg)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_LONG_EQUAL_MSG failure: %d != %d, 0x%X != 0x%X User info: %s",
            (int)expected, (int)actual, (unsigned int)expected, (unsigned int)actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_long_not_equal(
    const char* file, unsigned line, const char* func,
    long expected, long actual)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_LONG_NOT_EQUAL failure: %d == %d, 0x%X == 0x%X",
            (int)expected, (int)actual, (unsigned int)expected, (unsigned int)actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_long_not_equal_msg(
    const char* file, unsigned line, const char* func,
    long expected, long actual, const char* msg)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_LONG_NOT_EQUAL_MSG failure: %d == %d, 0x%X == 0x%X User info: %s",
            (int)expected, (int)actual, (unsigned int)expected, (unsigned int)actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_buffer_equal(const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, unsigned length)
{
    int offset;
    if (compare_mem_buffer(expected, actual, length, &offset))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_BUFFER_EQUAL failure: 0x%X != 0x%X (offset %d)", 
            expected[offset], actual[offset], offset);
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_buffer_equal_msg(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, unsigned length,
    const char * msg)
{
    int offset;
    if (compare_mem_buffer(expected, actual, length, &offset))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_BUFFER_EQUAL_MSG failure: 0x%X != 0x%X (offset %d) User info: %s",
            expected[offset], actual[offset], offset, msg);
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_buffer_not_equal(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, unsigned length)
{
    int offset;
    if (!compare_mem_buffer(expected, actual, length, &offset))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_BUFFER_NOT_EQUAL failure");
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_buffer_not_equal_msg(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, unsigned length,
    const char * msg)
{
    int offset;
    if (!compare_mem_buffer(expected, actual, length, &offset))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_BUFFER_NOT_EQUAL_MSG failure User info: %s", 
            msg);
    }

    return;
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_string_equal(const char* file, unsigned line, const char* func,
    const char * expected, const char * actual)
{
    if (strcmp(expected, actual))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, "MUT_ASSERT_STRING_EQUAL failure: Expected \"%s\" Actual \"%s\"",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_string_equal_msg(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, const char * msg)
{
    if (strcmp(expected, actual))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, "MUT_ASSERT_STRING_EQUAL_MSG failure: Expected \"%s\" Actual \"%s\" User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_string_not_equal(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual)
{
    if (!strcmp(expected, actual))
    {
        mut_control->mut_log->mut_log_assert(file, line, func, 
            "MUT_ASSERT_STRING_NOT_EQUAL failure: Expected \"%s\" Actual \"%s\"",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_string_not_equal_msg(
    const char* file, unsigned line, const char* func,
    const char * expected, const char * actual, const char * msg)
{
    if (!strcmp(expected, actual))
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_STRING_NOT_EQUAL_MSG failure: Expected \"%s\" Actual \"%s\" User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_char_equal(const char* file, unsigned line, const char* func,
    char expected, char actual)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CHAR_EQUAL failure: Expected '%c' Actual '%c'",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_char_equal_msg(
    const char* file, unsigned line, const char* func,
    char expected, char actual, const char * msg)
{
    if (expected != actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CHAR_EQUAL_MSG failure: "
            "Expected '%c' Actual '%c' User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_char_not_equal(
    const char* file, unsigned line, const char* func,
    char expected, char actual)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CHAR_NOT_EQUAL failure: Expected '%c' Actual '%c'",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_char_not_equal_msg(
    const char* file, unsigned line, const char* func,
    char expected, char actual, const char * msg)
{
    if (expected == actual)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CHAR_NOT_EQUAL_MSG failure: "
            "Expected '%c' Actual '%c' User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_float_equal(const char* file, unsigned line, const char* func,
    float expected, float actual, float epsilon)
{
    if (mut_fabsf(expected - actual) > epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_FLOAT_EQUAL failure: Expected (%e) Actual (%e)",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_float_equal_msg(
    const char* file, unsigned line, const char* func,
    float expected, float actual, float epsilon, const char * msg)
{
    if (mut_fabsf(expected - actual) > epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_FLOAT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_float_not_equal(
    const char* file, unsigned line, const char* func,
    float expected, float actual, float epsilon)
{
    if (mut_fabsf(expected - actual) < epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_FLOAT_NOT_EQUAL failure: Expected (%e) Actual (%e)",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_float_not_equal_msg(
    const char* file, unsigned line, const char* func,
    float expected, float actual, float epsilon, const char * msg)
{
    if (mut_fabsf(expected - actual) < epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_FLOAT_NOT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_double_equal(
    const char* file, unsigned line, const char* func,
    double expected, double actual, double epsilon)
{
    if (fabs(expected - actual) > epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_DOUBLE_EQUAL failure: Expected (%e) Actual (%e)",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_double_equal_msg(
    const char* file, unsigned line, const char* func,
    double expected, double actual, double epsilon, const char * msg)
{
    if (fabs(expected - actual) > epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_DOUBLE_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_double_not_equal(
    const char* file, unsigned line, const char* func,
    double expected, double actual, double epsilon)
{
    if (fabs(expected - actual) < epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_DOUBLE_NOT_EQUAL failure: Expected (%e) Actual (%e)",
            expected, actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_double_not_equal_msg(
    const char* file, unsigned line, const char* func,
    double expected, double actual, double epsilon, const char * msg)
{
    if (fabs(expected - actual) < epsilon)
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG failure: Expected (%e) Actual (%e) User info: %s",
            expected, actual, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_condition(const char * file, unsigned line, const char * func,
                          int condition, int op1_value, int op2_value, 
                          const char * op1, const char * op2, const char * oper)
{
    if( !condition )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CONDITION failure: (%s) %s (%s) where %s = %d and %s = %d",
            op1, oper, op2, op1, op1_value, op2, op2_value);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_condition_msg(const char * file, unsigned line, const char * func,
                          int condition, int op1_value, int op2_value, const char * op1,
                          const char * op2, const char * oper, const char * msg)
{
    if( !condition )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_CONDITION_MSG failure: (%s) %s (%s) where %s = %d and %s = %d User info: %s",
            op1, oper, op2, op1, op1_value, op2, op2_value, msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_uint64_equal(const char * file, unsigned line, const char * func,
                              unsigned __int64 m_expected, unsigned __int64 m_actual)
{
    if( m_expected != m_actual )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_INTEGER_EQUAL failure: %lld != %lld, 0x%llX != 0x%llX", 
            (long long)m_expected, (long long)m_actual, (unsigned long long)m_expected, (unsigned long long)m_actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_uint64_equal_msg(const char * file, unsigned line, const char * func,
                              unsigned __int64 m_expected, unsigned __int64 m_actual,
                              const char * m_msg)
{
    if( m_expected != m_actual )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_INTEGER_EQUAL_MSG failure: %lld != %lld, 0x%llX != 0x%llX User info: %s", 
            (long long)m_expected, (long long)m_actual, (unsigned long long)m_expected, (unsigned long long)m_actual, m_msg);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_uint64_not_equal(const char * file, unsigned line, const char * func,
                              unsigned __int64 m_expected, unsigned __int64 m_actual)
{
    if( m_expected == m_actual )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_INTEGER_NOT_EQUAL failure: %lld == %lld, 0x%llX == 0x%llX", 
            (long long)m_expected, (long long)m_actual, (unsigned long long)m_expected, (unsigned long long)m_actual);
    }
}

/* doxygen documentation for this function can be found in mut_assert.h */
MUT_API void mut_assert_uint64_not_equal_msg(const char * file, unsigned line, const char * func,
                              unsigned __int64 m_expected, unsigned __int64 m_actual,
                              const char * m_msg)
{
    if( m_expected == m_actual )
    {
        mut_control->mut_log->mut_log_assert(file, line, func,
            "MUT_ASSERT_INTEGER_NOT_EQUAL_MSG failure: %lld == %lld, 0x%llX == 0x%llX User info: %s", 
            (long long)m_expected, (long long)m_actual, (unsigned long long)m_expected, (unsigned long long)m_actual, m_msg);
    }
}
