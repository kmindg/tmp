/*
 * copyright 2001
 */
#if !defined(CDECL_H)
#define CDECL_H 1
/*
 * cdecl.h
 *
 * DESCRIPTION:  This file defines the preprocessor symbol CDECL_ as __cdecl
 *   if _MSC_VER is defined and as an empty string if _MSC_VER is not defined.
 *   Thus the CDECL_ symbol can be used in declaration of functions that are
 *   valid under the VC++ compiler and other compilers.
 *
 * HISTORY:
 *   12-Jan-2001CE jfw -- initial version
 *
 */
#if defined(_MSC_VER)
#define CDECL_  __cdecl
#else
#define CDECL_
#endif
#endif /* !defined(CDECL_H) */
