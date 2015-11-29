/*
 * copyright
 */
/*
 * bool.h
 *
 * DESCRIPTION: This file supplies a typdef for bool which defines
 *   it as an enum with the values of true and false.  The typedef is
 *   generated only if __cplusplus is not defined, since C++ has bool
 *   as an elementary type.  NOTE that the following attributes of C++'s
 *   bool are not emulated in C:
 *
 *        (bool) as a cast does not force the value to be either true or false
 *         boolval++ does not simply set boolval to true.  It will set it to
 *         false if the previous value was 0xFFFFFFFF.
 *
 * HISTORY:
 *   30May2000 JFW: formally added to the source tree.
 */
#if !defined(BOOL_H)
#define BOOL_H
#if !defined(__cplusplus)
typedef enum bool_enum
{
   false,
   true
} bool;
#endif /* !defined(__cplusplus) */
#endif /* !defined(BOOL_H) */ /* end of file bool.h */
