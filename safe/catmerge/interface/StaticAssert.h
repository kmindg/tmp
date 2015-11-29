/************************************************************************
 *
 *  Copyright (c) 2011 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 *
 ************************************************************************/

#ifndef __StaticAssert_h__
#define __StaticAssert_h__


//  STATIC_ASSERT
//
//  Makes a static assertion, that is, an assertion that is checked at
//  compilation time and which results in a compilation error if false.
//
//  The `condition` is a constant boolean expression (that is, a
//  constant integer expression where zero means false and nonzero means
//  true).  The `description` must have the form of a valid suffix of a
//  program identifier and must be unique among uses of STATIC_ASSERT.
//
//  Checking the condition involves attempting to define an identifier
//  whose name is the concatenation of "__VIOLATED_ASSERTION_THAT__" and
//  the given description.  Hence, phrase the description as the
//  statement of the property that must hold.
//
//  Unfortunately, the compilation error that results on assertion
//  violation is misleading.  The error results from an attempt to
//  define an array with negative size.  However, the name of the array
//  indicates the assertion that was violated.

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif
#ifndef STATIC_ASSERT_DEFINED
#define STATIC_ASSERT_DEFINED
#if ((defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(__cplusplus)
#define STATIC_ASSERT(description, condition) \
    _Static_assert(condition, #description);
#else
#define STATIC_ASSERT(description, condition) \
    typedef char __VIOLATED_ASSERTION_THAT__##description[1-2*!(condition)];
#endif
#endif


#endif // __StaticAssert_h__
