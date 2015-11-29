/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_assert.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework asserts
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/08/2007   Kirill Timofeev  asserts moved to separate header file
 *    12/07/2007   Igor Bobrovnikov  asserts implementation moved to C file
 *    01/10/2008   Igor Bobrovnikov (bobroi) 
 *                 epsilon argument for float and double compare added
 *    08/29/2008   Alexander Alanov (alanoa)
 *                 Added MUT_ASSERT_POINTER_* macros
 *    10/29/2008   Alexander Alanov (alanoa)
 *                 Added MUT_ASSERT_INTEGER_* macros
 *
 **************************************************************************/

#ifndef MUT_ASSERT_H
#define MUT_ASSERT_H

#if ! defined(MUT_REV2)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * MUT ASSERTS BEGIN
 **************************************************************************/

/** \def MUT_ASSERT_FALSE(m_condition)
 *  \param m_condition - expression like x == 0
 *
 *  \details This predicate fails in case condition true
 */
#define MUT_ASSERT_FALSE(m_condition) \
    mut_assert_false(__FILE__, __LINE__, __FUNCTION__, (m_condition) == 0, #m_condition);

/** \def MUT_ASSERT_FALSE_MSG(CONDITION)
 *  \param m_condition - expression like x == 0
 *  \param m_msg - error message type of m_msg is const char *
 *
 *  \details This predicate fails in case condition true
 */
#define MUT_ASSERT_FALSE_MSG(m_condition, m_msg) \
    mut_assert_false_msg(__FILE__, __LINE__, __FUNCTION__, (m_condition) == 0, #m_condition, (m_msg));

/** \def MUT_ASSERT_TRUE(CONDITION)
 *  \param m_condition - expression like x == 0
 *
 *  \details This predicate fails in case condition false
 */
#define MUT_ASSERT_TRUE(m_condition) \
    mut_assert_true(__FILE__, __LINE__, __FUNCTION__, (m_condition) != 0, #m_condition);

/** \def MUT_ASSERT_TRUE_MSG(CONDITION)
 *  \param m_condition - expression like x == 0
 *  \param m_msg       - user message type of m_msg is const char *
 *
 *  \details This predicate fails in case condition false
 */
#define MUT_ASSERT_TRUE_MSG(m_condition, m_msg) \
    mut_assert_true_msg(__FILE__, __LINE__, __FUNCTION__, (m_condition) != 0, #m_condition, (m_msg));

/** \def MUT_FAIL()
 *
 * \details This predicate always fails
 */
#define MUT_FAIL() \
    mut_fail(__FILE__, __LINE__, __FUNCTION__);

/** \def MUT_FAIL_MSG(m_msg)
 *  \param m_msg - error message type of m_msg is const char *
 *
 *  \details This predicate always fails and send msg
 */
#define MUT_FAIL_MSG(m_msg) \
    mut_fail_msg(__FILE__, __LINE__, __FUNCTION__, (m_msg));

/** \def MUT_ASSERT_NULL(m_ptr)
 *  \param m_ptr type of m_ptr is a pointer
 *
 *  \details predicate  m_ptr. it should be NULL
 */
#define MUT_ASSERT_NULL(m_ptr) \
    mut_assert_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr));

/** \def MUT_ASSERT_NULL_MSG(m_ptr, m_msg)
 *  \param m_ptr - check this ptr for NULL. type of m_ptr is a pointer
 *  \param m_msg - message for output. type of m_msg is const char *
 *
 *  \details predicate checks m_ptr. it should be NULL
 */
#define MUT_ASSERT_NULL_MSG(m_ptr, m_msg) \
    mut_assert_null_msg(__FILE__, __LINE__, __FUNCTION__, (m_ptr), (m_msg));
    
/** \def MUT_ASSERT_NOT_NULL(m_ptr)
 *  \param m_ptr - pointer for checking. type of m_ptr is a pointer
 *
 *  \details predicate checks m_ptr. it should be NULL
 */
#define MUT_ASSERT_NOT_NULL(m_ptr) \
    mut_assert_not_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr));

/** \def MUT_ASSERT_NOT_NULL_MSG(m_ptr, m_msg)
 *  \param m_ptr - check this ptr for NULL. type of m_ptr is a pointer
 *  \param m_msg - message for output. type of m_msg is const char *
 *
 *  \details predicate checks m_ptr. it should be NULL
 */
#define MUT_ASSERT_NOT_NULL_MSG(m_ptr, m_msg) \
    mut_assert_not_null_msg(__FILE__, __LINE__, __FUNCTION__, (m_ptr), (m_msg));

/** \def MUT_ASSERT_POINTER_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is void*
 *  \param m_actual   - actual value. type of m_actual is void*
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_POINTER_EQUAL(m_expected, m_actual) \
    mut_assert_pointer_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_POINTER_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is void*
 *  \param m_actual   - actual value. type of m_actual is void*
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_POINTER_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_pointer_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_POINTER_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is void*
 *  \param m_actual   - actual value. type of m_actual is void*
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_POINTER_NOT_EQUAL(m_expected, m_actual)  \
    mut_assert_pointer_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_POINTER_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_msg - user message. type of m_msg is const char *
 *  \param m_expected - expected value. type of m_expected is void*
 *  \param m_actual   - actual value. type of m_actual is void*
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_POINTER_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_pointer_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_INT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_INT_EQUAL(m_expected, m_actual) \
    mut_assert_int_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_UINT64_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_UINT64_EQUAL(m_expected, m_actual) \
    mut_assert_uint64_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_INT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_INT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_int_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_UINT64_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_UINT64_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_uint64_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_INT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_INT_NOT_EQUAL(m_expected, m_actual)  \
    mut_assert_int_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_UINT64_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_UINT64_NOT_EQUAL(m_expected, m_actual)  \
    mut_assert_uint64_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_INT_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_msg - user message. type of m_msg is const char *
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_INT_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_int_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_LONG_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_LONG_EQUAL(m_expected, m_actual)  \
    mut_assert_long_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_LONG_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_LONG_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_long_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_LONG_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_LONG_NOT_EQUAL(m_expected, m_actual) \
    mut_assert_long_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_LONG_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_LONG_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_long_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_STRING_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_STRING_EQUAL(m_expected, m_actual)  \
    mut_assert_string_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_STRING_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_STRING_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_string_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_STRING_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_STRING_NOT_EQUAL(m_expected, m_actual) \
    mut_assert_string_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_STRING_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_STRING_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_string_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_CHAR_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_CHAR_EQUAL(m_expected, m_actual) \
    mut_assert_char_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_CHAR_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_CHAR_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_char_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_CHAR_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_CHAR_NOT_EQUAL(m_expected, m_actual) \
    mut_assert_char_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual));

/** \def MUT_ASSERT_CHAR_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_CHAR_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_char_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg));

/** \def MUT_ASSERT_BUFFER_EQUAL(m_expected, m_actual, m_extent)
 *  \param m_expected - expected value. type of m_expected is a pointer
 *  \param m_actual   - actual value. type of m_actual is a pointer
 *  \param m_extent   - size of buffer. type of m_extent is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_BUFFER_EQUAL(m_expected, m_actual, m_extent) \
    mut_assert_buffer_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_extent));

/** \def MUT_ASSERT_BUFFER_EQUAL_MSG(m_expected, m_actual, m_extent, m_msg)
 *  \param m_expected - expected value. type of m_expected is a pointer
 *  \param m_actual   - actual value. type of m_actual is a pointer
 *  \param m_extent   - size of buffer. type of m_extent is int
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_BUFFER_EQUAL_MSG(m_expected, m_actual, m_extent, m_msg) \
        mut_assert_buffer_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_extent), (m_msg));

/** \def MUT_ASSERT_BUFFER_NOT_EQUAL(m_expected, m_actual, m_extent)
 *  \param m_expected - expected value. type of m_expected is a pointer
 *  \param m_actual   - actual value. type of m_actual is a pointer
 *  \param m_extent   - size of buffer. type of m_extent is int
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_BUFFER_NOT_EQUAL(m_expected, m_actual, m_extent)  \
    mut_assert_buffer_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_extent));

/** \def MUT_ASSERT_BUFFER_NOT_EQUAL_MSG(m_expected, m_actual, m_extent, m_msg)
 *  \param m_expected - expected value. type of m_expected is a pointer
 *  \param m_actual   - actual value. type of m_actual is a pointer
 *  \param m_extent   - size of buffer. type of m_extent is int
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_BUFFER_NOT_EQUAL_MSG(m_expected, m_actual, m_extent, m_msg) \
    mut_assert_buffer_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_extent), (m_msg));

/** \def MUT_ASSERT_FLOAT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is float
 *  \param m_actual   - actual value. type of m_actual is float
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_FLOAT_EQUAL(m_expected, m_actual, m_epsilon) \
    mut_assert_float_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon));

/** \def MUT_ASSERT_FLOAT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is float
 *  \param m_actual   - actual value. type of m_actual is float
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_FLOAT_EQUAL_MSG(m_expected, m_actual, m_epsilon, m_msg) \
    mut_assert_float_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon), (m_msg));

/** \def MUT_ASSERT_FLOAT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is float
 *  \param m_actual   - actual value. type of m_actual is float
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_FLOAT_NOT_EQUAL(m_expected, m_actual, m_epsilon) \
    mut_assert_float_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon));

/** \def MUT_ASSERT_FLOAT_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is float
 *  \param m_actual   - actual value. type of m_actual is float
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_FLOAT_NOT_EQUAL_MSG(m_expected, m_actual, m_epsilon, m_msg) \
    mut_assert_float_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon), (m_msg));

/** \def MUT_ASSERT_DOUBLE_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_DOUBLE_EQUAL(m_expected, m_actual, m_epsilon) \
mut_assert_double_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon));

/** \def MUT_ASSERT_DOUBLE_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_DOUBLE_EQUAL_MSG(m_expected, m_actual, m_epsilon, m_msg) \
    mut_assert_double_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon), (m_msg));

/** \def MUT_ASSERT_DOUBLE_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_DOUBLE_NOT_EQUAL(m_expected, m_actual, m_epsilon) \
    mut_assert_double_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon));

/** \def MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *  \param m_msg - user message. type of m_msg is const char *
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
#define MUT_ASSERT_DOUBLE_NOT_EQUAL_MSG(m_expected, m_actual, m_epsilon, m_msg) \
    mut_assert_double_not_equal_msg(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_epsilon), (m_msg));

/** \def MUT_ASSERT_CONDITION(m_op1, m_operator, m_op2)
 *  \param  m_op1 - first operand
 *  \param  m_op2 - second operand
 *  \param  m_operator - binary operator
 *
 *  \details predicate fails in case the condition m_op1 m_operator m_op2 is false
 */
#define MUT_ASSERT_CONDITION(m_op1, m_operator, m_op2) \
    mut_assert_condition(__FILE__, __LINE__, __FUNCTION__, ((m_op1) m_operator (m_op2)), (int)(m_op1), (int)(m_op2), #m_op1, #m_op2, #m_operator);

/** \def MUT_ASSERT_CONDITION(m_op1, m_operator, m_op2)
 *  \param  m_op1 - first operand
 *  \param  m_op2 - second operand
 *  \param  m_operator - binary operator
 *  \param  m_msg - user message (null-terminated string)
 *
 *  \details predicate fails in case the condition m_op1 m_operator m_op2 is false
 */
#define MUT_ASSERT_CONDITION_MSG(m_op1, m_operator, m_op2, m_msg) \
    mut_assert_condition_msg(__FILE__, __LINE__, __FUNCTION__, ((m_op1) m_operator (m_op2)),  (int)(m_op1), (int)(m_op2), #m_op1, #m_op2, #m_operator, m_msg);

/** \def MUT_ASSERT_INTEGER_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value, any unsigned integral type is accepted
 *  \param m_actual - expected value, any unsigned integral type is accepted
 *
 *  \details predicate fails, if m_expected != m_actual. Note that before comparison,
 *  \details m_expected and m_actual are cast to unsigned __int64.
 */
#define MUT_ASSERT_INTEGER_EQUAL(m_expected, m_actual)\
    mut_assert_uint64_equal(__FILE__, __LINE__, __FUNCTION__,\
    (unsigned __int64)(m_expected), (unsigned __int64)(m_actual));

/** \def MUT_ASSERT_INTEGER_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value, any unsigned integral type is accepted
 *  \param m_actual - expected value, any unsigned integral type is accepted
 *
 *  \details predicate fails, if m_expected == m_actual. Note that before comparison,
 *  \details  m_expected and m_actual are cast to unsigned __int64.
 */
#define MUT_ASSERT_INTEGER_NOT_EQUAL(m_expected, m_actual)\
    mut_assert_uint64_not_equal(__FILE__, __LINE__, __FUNCTION__,\
    (unsigned __int64)(m_expected), (unsigned __int64)(m_actual));

/** \def MUT_ASSERT_INTEGER_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value, any unsigned integral type is accepted
 *  \param m_actual - expected value, any unsigned integral type is accepted
 *  \param m_msg - user-specified message, zero-terminated string
 *
 *  \details predicate fails, if m_expected != m_actual. Before comparison,
 *  \details m_expected and m_actual are cast to unsigned __int64. In case
 *  \details of failure, user-specified message would be printed.
 */
#define MUT_ASSERT_INTEGER_EQUAL_MSG(m_expected, m_actual, m_msg)\
    mut_assert_uint64_equal_msg(__FILE__, __LINE__, __FUNCTION__,\
    (unsigned __int64)(m_expected), (unsigned __int64)(m_actual), (m_msg));

/** \def MUT_ASSERT_INTEGER_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value, any unsigned integral type is accepted
 *  \param m_actual - expected value, any unsigned integral type is accepted
 *  \param m_msg - user-specified message, zero-terminated string
 *
 *  \details predicate fails, if m_expected == m_actual. Before comparison,
 *  \details m_expected and m_actual are cast to unsigned __int64. In case
 *  \details of failure, user-specified message would be printed.
 */
#define MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(m_expected, m_actual, m_msg)\
    mut_assert_uint64_not_equal_msg(__FILE__, __LINE__, __FUNCTION__,\
    (unsigned __int64)(m_expected), (unsigned __int64)(m_actual), (m_msg));


/***************************************************************************
 * MUT ASSERTS END
 **************************************************************************/

/***************************************************************************
 * MUT ASSERTS IMPLEMENTATION FUNCTIONS BEGIN
 **************************************************************************/

/** \fn void mut_assert_false(void)
 *  \param m_condition - expression like x == 0
 *  \details This predicate fails in case condition true
 */
MUT_API void MUT_CC mut_assert_false(const char* file, unsigned line, const char* func, int condition, const char * s_condition);

/** \fn void mut_assert_false_msg(void)
 *  \param m_condition - expression like x == 0
 *  \param m_msg - user error message
 *  \details This predicate fails in case condition true
 */
MUT_API void MUT_CC mut_assert_false_msg(const char* file, unsigned line, const char* func, int condition, const char * s_condition, const char * msg);

/** \fn void mut_assert_true(void)
 *  \param m_condition - expression like x == 0
 *  \details This predicate fails in case condition false
 */
MUT_API void MUT_CC mut_assert_true(const char* file, unsigned line, const char* func, int condition, const char * s_condition);

/** \fn void mut_assert_true_msg(void)
 *  \param m_condition - expression like x == 0
 *  \param m_msg       - user message
 *  \details This predicate fails in case condition false
 */
MUT_API void MUT_CC mut_assert_true_msg(const char* file, unsigned line, const char* func, int condition, const char * s_condition, const char * msg);

/** \fn void mut_fail(void)
 *  \details This predicate always fails
 */
MUT_API void MUT_CC mut_fail(const char* file, unsigned line, const char* func);

/** \fn void mut_fail_msg(void)
 *  \param m_msg - user error message
 *  \details This predicate always fails
 */
MUT_API void MUT_CC mut_fail_msg(const char* file, unsigned line, const char* func, const char * msg);

/** \fn void mut_assert_null(void)
 *  \param ptr - pointer to be checked
 *  \details predicate fails is ptr is not null
 */
MUT_API void MUT_CC mut_assert_null(const char* file, unsigned line, const char* func, const void* ptr);

/** \fn void mut_assert_null_msg(void)
 *  \param ptr - pointer to be checked
 *  \param msg - user error message
 *  \details predicate fails is ptr is not null
 */
MUT_API void MUT_CC mut_assert_null_msg(const char* file, unsigned line, const char* func, const void* ptr, const char* msg);

/** \fn void mut_assert_not_null(void)
 *  \param ptr - pointer to be checked
 *  \details predicate fails is ptr is null
 */
MUT_API void MUT_CC mut_assert_not_null(const char* file, unsigned line, const char* func, const void* ptr);

/** \fn void mut_assert_not_null_msg(void)
 *  \param ptr - pointer to be checked
 *  \param msg - user error message
 *  \details predicate fails is ptr is null 
 */
MUT_API void MUT_CC mut_assert_not_null_msg(const char* file, unsigned line, const char* func, const void* ptr, const char* msg);

/** \fn void mut_assert_pointer_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_pointer_equal(const char* file, unsigned line, const char* func, const void* expected, const void* actual);

/** \fn void mut_assert_pointer_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user assert message
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_pointer_equal_msg(const char* file, unsigned line, const char* func, const void* expected, const void* actual, const char* msg);

/** \fn void mut_assert_pointer_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_pointer_not_equal(const char* file, unsigned line, const char* func, const void* expected, const void* actual);

/** \fn void mut_assert_pointer_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user assert message
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_pointer_not_equal_msg(const char* file, unsigned line, const char* func, const void* expected, const void* actual, const char* msg);

/** \fn void mut_assert_int_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_int_equal(const char* file, unsigned line, const char* func, int expected, int actual);

/** \fn void mut_assert_int_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user assert message
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_int_equal_msg(const char* file, unsigned line, const char* func, int expected, int actual, const char* msg);

/** \fn void mut_assert_int_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_int_not_equal(const char* file, unsigned line, const char* func, int expected, int actual);

/** \fn void mut_assert_int_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user assert message
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_int_not_equal_msg(const char* file, unsigned line, const char* func, int expected, int actual, const char* msg);

/** \fn void mut_assert_long_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_long_equal(const char* file, unsigned line, const char* func, long expected, long actual);

/** \fn void mut_assert_long_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user assert message
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_long_equal_msg(const char* file, unsigned line, const char* func, long expected, long actual, const char* msg);

/** \fn void mut_assert_long_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
  *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_long_not_equal(const char* file, unsigned line, const char* func, long expected, long actual);

/** \fn void mut_assert_long_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_long_not_equal_msg(const char* file, unsigned line, const char* func, long expected, long actual, const char* msg);

/** \fn void mut_assert_buffer_equal(void)
 *  \param expected - expected byte buffer
 *  \param actual - actual byte buffer
 *  \param extent - length of buffer
 *  \details predicate fails in case actual and expected byte buffers are not equal
 */
MUT_API void MUT_CC mut_assert_buffer_equal(const char* file, unsigned line, const char* func, const char * expected, const char * actual, unsigned length);

/** \fn void mut_assert_buffer_equal_msg(void)
 *  \param expected - expected byte buffer
 *  \param actual - actual byte buffer
 *  \param extent - length of buffer
 *  \param msg - user assert message
 *  \details predicate fails in case actual and expected byte buffers are not equal
 */
MUT_API void MUT_CC mut_assert_buffer_equal_msg(const char* file, unsigned line, const char* func, const char * expected, const char * actual, unsigned length, const char * msg);

/** \fn void mut_assert_buffer_not_equal(void)
*  \param expected - expected byte buffer
 *  \param actual - actual byte buffer
 *  \param extent - length of buffer
 *  \details predicate fails in case actual and expected byte buffers are equal
 */
MUT_API void MUT_CC mut_assert_buffer_not_equal(const char* file, unsigned line, const char* func, const char * expected, const char * actual, unsigned length);

/** \fn void mut_assert_buffer_not_equal_msg(void)
*  \param expected - expected byte buffer
 *  \param actual - actual byte buffer
 *  \param extent - length of buffer
 *  \param msg - user assert message
 *  \details predicate fails in case actual and expected byte buffers are equal
 */
MUT_API void MUT_CC mut_assert_buffer_not_equal_msg(const char* file, unsigned line, const char* func, const char * expected, const char * actual, unsigned length, const char * msg);

/** \fn void mut_assert_string_equal(void)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \details predicate  fails in case actual and expected strings are not equal
 */
MUT_API void MUT_CC mut_assert_string_equal(const char* file, unsigned line, const char* func, const char * expected, const char * actual);

/** \fn void mut_assert_string_equal_msg(void)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \param m_msg - user message
 *  \details predicate  fails in case actual and expected strings are not equal
 */
MUT_API void MUT_CC mut_assert_string_equal_msg(const char* file, unsigned line, const char* func, const char * expected, const char * actual, const char * msg);

/** \fn void mut_assert_string_not_equal(void)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \details predicate  fails in case actual and expected strings are equal
 */
MUT_API void MUT_CC mut_assert_string_not_equal(const char* file, unsigned line, const char* func, const char * expected, const char * actual);

/** \fn void mut_assert_string_not_equal_msg(void)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \param m_msg - user message
 *  \details predicate  fails in case actual and expected strings are equal
 */
MUT_API void MUT_CC mut_assert_string_not_equal_msg(const char* file, unsigned line, const char* func, const char * expected, const char * actual, const char * msg);

/** \fn void mut_assert_char_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details assert failed in case actual and expected chars are not equal
 */
MUT_API void MUT_CC mut_assert_char_equal(const char* file, unsigned line, const char* func, char expected, char actual);

/** \fn void mut_assert_char_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details assert failed in case actual and expected chars are not equal
 */
MUT_API void MUT_CC mut_assert_char_equal_msg(const char* file, unsigned line, const char* func, char expected, char actual, const char * msg);

/** \fn void mut_assert_char_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details assert failed in case actual and expected chars are equal
 */
MUT_API void MUT_CC mut_assert_char_not_equal(const char* file, unsigned line, const char* func, char expected, char actual);

/** \fn void mut_assert_char_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details assert failed in case actual and expected chars are equal
 */
MUT_API void MUT_CC mut_assert_char_not_equal_msg(const char* file, unsigned line, const char* func, char expected, char actual, const char * msg);

/** \fn void mut_assert_float_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_float_equal(const char* file, unsigned line, const char* func, float expected, float actual, float delta);

/** \fn void mut_assert_float_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_float_equal_msg(const char* file, unsigned line, const char* func, float expected, float actual, float delta, const char * msg);

/** \fn void mut_assert_float_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_float_not_equal(const char* file, unsigned line, const char* func, float expected, float delta, float actual);

/** \fn void mut_assert_float_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_float_not_equal_msg(const char* file, unsigned line, const char* func, float expected, float actual, float delta, const char * msg);

/** \fn void mut_assert_double_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_double_equal(const char* file, unsigned line, const char* func, double expected, double actual, double delta);

/** \fn void mut_assert_double_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details predicate  fails in case actual and expected values are not equal
 */
MUT_API void MUT_CC mut_assert_double_equal_msg(const char* file, unsigned line, const char* func, double expected, double actual, double delta, const char * msg);

/** \fn void mut_assert_double_not_equal(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_double_not_equal(const char* file, unsigned line, const char* func, double expected, double actual, double delta);

/** \fn void mut_assert_double_not_equal_msg(void)
 *  \param expected - expected value
 *  \param actual - actual value
 *  \param msg - user message
 *  \details predicate  fails in case actual and expected values are equal
 */
MUT_API void MUT_CC mut_assert_double_not_equal_msg(const char* file, unsigned line, const char* func, double expected, double actual, double delta, const char * msg);

/** \fn void mut_assert_condition(void)
 *  \param  op1 - first operand
 *  \param  op2 - second operand
 *  \param  oper - binary operator
 *  \param  condition - value of condition (op1) oper (op2)
 *  \param  op1_value - value of first operand
 *  \param  op2_value - value of second operand
 *  \details predicate fails in case the condition m_op1 m_operator m_op2 is false
 */
MUT_API void MUT_CC mut_assert_condition(const char * file, unsigned line, const char * func, 
                                  int condition, int op1_value, int op2_value, 
                                  const char * op1, const char * op2, const char * oper);
/** \fn void mut_assert_condition_msg(void)
 *  \param  op1 - first operand
 *  \param  op2 - second operand
 *  \param  oper - binary operator
 *  \param  msg - user specified message
 *  \param  condition - value of condition (op1) oper (op2)
 *  \param  op1_value - value of first operand
 *  \param  op2_value - value of second operand
 *  \details predicate fails in case the condition m_op1 m_operator m_op2 is false
 */
MUT_API void MUT_CC mut_assert_condition_msg(const char * file, unsigned line, const char * func,
                                      int condition, int op1_value, int op2_value, const char * op1, 
                                      const char * op2, const char * oper, const char * msg);

/** \fn void mut_assert_uint64_equal(m_expected, m_actual)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *
 *  \details predicate fails, if m_expected != m_actual
 */
MUT_API void MUT_CC mut_assert_uint64_equal(const char * file, unsigned line, const char * func,
                                     unsigned __int64 m_expected, unsigned __int64 m_actual);

/** \fn void mut_assert_uint64_equal(m_expected, m_actual)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *
 *  \details predicate fails, if m_expected == m_actual
 */
MUT_API void MUT_CC mut_assert_uint64_not_equal(const char * file, unsigned line, const char * func,
                                         unsigned __int64 m_expected, unsigned __int64 m_actual);

/** \fn void mut_assert_uint64_equal(m_expected, m_actual)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \param m_msg - user-specified message
 *
 *  \details predicate fails, if m_expected != m_actual
 */
MUT_API void MUT_CC mut_assert_uint64_equal_msg(const char * file, unsigned line, const char * func,
                                         unsigned __int64 m_expected, unsigned __int64 m_actual,
                                         const char * m_msg);

/** \fn void mut_assert_uint64_equal(m_expected, m_actual)
 *  \param m_expected - expected value
 *  \param m_actual - actual value
 *  \param m_msg - user-specified message
 *
 *  \details predicate fails, if m_expected == m_actual
 */
MUT_API void MUT_CC mut_assert_uint64_not_equal_msg(const char * file, unsigned line, const char * func,
                                             unsigned __int64 m_expected, unsigned __int64 m_actual,
                                             const char * m_msg);

/***************************************************************************
 * MUT ASSERTS IMPLEMENTATION FUNCTIONS END
 **************************************************************************/
#ifdef __cplusplus
}
#endif

#else

#define MUT_ASSERT_FALSE(m_condition) \
    mut_assert_false(__FILE__, __LINE__, __FUNCTION__, (m_condition) == 0, #m_condition);

#define MUT_ASSERT_FALSE_MSG(m_condition, m_msg) \
    mut_assert_false(__FILE__, __LINE__, __FUNCTION__, (m_condition) == 0, #m_condition, (m_msg));

#define MUT_ASSERT_TRUE(m_condition) \
    mut_assert_true(__FILE__, __LINE__, __FUNCTION__, (m_condition) != 0, #m_condition);

#define MUT_ASSERT_TRUE_MSG(m_condition, m_msg) \
    mut_assert_true(__FILE__, __LINE__, __FUNCTION__, (m_condition) != 0, #m_condition, (m_msg));

#define MUT_FAIL() \
    mut_fail(__FILE__, __LINE__, __FUNCTION__);

#define MUT_FAIL_MSG(m_msg) \
    mut_fail(__FILE__, __LINE__, __FUNCTION__, (m_msg));
	
#define MUT_ASSERT_NULL(m_ptr) \
	mut_assert_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr))

#define MUT_ASSERT_NULL_MSG(m_ptr, m_msg) \
	mut_assert_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr), (m_msg))

#define MUT_ASSERT_NOT_NULL(m_ptr) \
	mut_assert_not_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr))

#define MUT_ASSERT_NOT_NULL_MSG(m_ptr, m_msg) \
	mut_assert_not_null(__FILE__, __LINE__, __FUNCTION__, (m_ptr), (m_msg))

#define MUT_ASSERT_EQUAL(m_expected, m_actual) \
    mut_assert_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual))

#define MUT_ASSERT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg))
	
#define MUT_ASSERT_NOT_EQUAL(m_expected, m_actual) \
    mut_assert_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual))

#define MUT_ASSERT_NOT_EQUAL_MSG(m_expected, m_actual, m_msg) \
    mut_assert_not_equal(__FILE__, __LINE__, __FUNCTION__, (m_expected), (m_actual), (m_msg))

/** \fn void mut_assert_false_msg(void)
 *  \param m_condition - expression like x == 0
 *  \param m_msg - user error message
 *  \details This predicate fails in case condition true
 */
MUT_API void MUT_CC mut_assert_false(const char* file, unsigned line, const char* func, bool condition, const char * s_condition, const char * msg = "");
	
/** \fn void mut_assert_true_msg(void)
 *  \param m_condition - expression like x == 0
 *  \param m_msg       - user message
 *  \details This predicate fails in case condition false
 */
MUT_API void MUT_CC mut_assert_true(const char* file, unsigned line, const char* func, bool condition, const char * s_condition, const char * msg = "");
	
/** \fn void mut_fail(void)
 *  \details This predicate always fails
 */
MUT_API void MUT_CC mut_fail(const char* file, unsigned line, const char* func, const char* msg = "");
	
/** \def MUT_ASSERT_NULL(m_ptr)
 *  \param m_ptr type of m_ptr is a pointer
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  m_ptr. it should be NULL
 */
MUT_API void MUT_CC mut_assert_null(const char *file, unsigned line, const char *func, const void* ptr, const char* msg = "");

MUT_API void MUT_CC mut_assert_not_null(const char *file, unsigned line, const char *func, const void* ptr, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const void *
 *  \param m_actual   - actual value. type of m_actual is const void *
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, const void* expected, const void* actual, const char* msg = "");


/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, int expected, int actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is unsigned int
 *  \param m_actual   - actual value. type of m_actual is unsigned int
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, unsigned int expected, unsigned int actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, char expected, char actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, long expected, long actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, double expected, double actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, const char* expected, const char* actual, const char* msg = "");

/** \def MUT_ASSERT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is uint64
 *  \param m_actual   - actual value. type of m_actual is uint64
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_equal(const char *file, unsigned line, const char *func, unsigned __int64 expected, unsigned __int64 actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const void *
 *  \param m_actual   - actual value. type of m_actual is const void *
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, const void* expected, const void* actual, const char* msg = "");


/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is int
 *  \param m_actual   - actual value. type of m_actual is int
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, int expected, int actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is unsigned int
 *  \param m_actual   - actual value. type of m_actual is unsigned int
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, unsigned int expected, unsigned int actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is char
 *  \param m_actual   - actual value. type of m_actual is char
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, char expected, char actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is long
 *  \param m_actual   - actual value. type of m_actual is long
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, long expected, long actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is double
 *  \param m_actual   - actual value. type of m_actual is double
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, double expected, double actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is const char *
 *  \param m_actual   - actual value. type of m_actual is const char *
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, const char* expected, const char* actual, const char* msg = "");

/** \def MUT_ASSERT_NOT_EQUAL(m_expected, m_actual)
 *  \param m_expected - expected value. type of m_expected is uint64
 *  \param m_actual   - actual value. type of m_actual is uint64
 *  \param m_msg - message for output. type of m_msg is const char *
 *  			   default value of message is null
 *
 *  \details predicate  fails in case actual and expected value is not equal
 */
MUT_API void MUT_CC mut_assert_not_equal(const char *file, unsigned line, const char *func, unsigned __int64 expected, unsigned __int64 actual, const char* msg = "");


#endif

#endif /* MUT_ASSERT_H */
