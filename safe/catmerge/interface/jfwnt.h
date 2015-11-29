#if !defined(JFWNT_H)
#define JFWNT_H

/*
 * void ntErrorMsg (const char *const messagePA)
 *
 ( DESCRIPTION:
 *
 * PARAMETERS:
 *
 * GLOBALS:
 *
 * CALLS:
 *
 * RETURNS:
 *
 * ERRORS:
 *
 * HISTORY:
 */
#ifdef ALAMOSA_WINDOWS_ENV
void __cdecl ntErrorMsg (const char *const messagePA);
#endif /* ALAMOSA_WINDOWS_ENV - NTHACK */

#endif /* !defined(JFWNT_H) */
