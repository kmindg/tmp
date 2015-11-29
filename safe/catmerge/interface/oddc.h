#ifndef __oddc_h
#define __oddc_h

//
//  On disk data compatability helpers
//

#include <stddef.h>

#ifndef ALAMOSA_WINDOWS_ENV
#include "safe_fix_null.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

//===========

static __inline int
oddc_strncmp_n_u(
    const char *s1,
    const NTWCHAR * s2,
    size_t n)
{
    for (; n > 0; s1++, s2++, --n) {
        if (*s1 != ((char) *s2)) {
            return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
        } else if (*s1 == 0) {
            return 0;
        }
    }
    return 0;
}

static __inline int
oddc_strncmp_u_n(
    const NTWCHAR * s1,
    const char *s2,
    size_t n)
{
    for (; n > 0; s1++, s2++, --n) {
        if (((char) *s1) != *s2) {
            return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
        } else if (*s1 == 0) {
            return 0;
        }
    }
    return 0;
}

static __inline int
oddc_strncmp_u_u(
    const NTWCHAR * s1,
    const NTWCHAR * s2,
    size_t n)
{
    for (; n > 0; s1++, s2++, --n) {
        if (*s1 != *s2) {
            return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
        } else if (*s1 == 0) {
            return 0;
        }
    }
    return 0;
}

//===========

static __inline int
oddc_strcmp_n_u(
    const char *s1,
    const NTWCHAR * s2)
{
    for (; *s1 == ((char) *s2); s1++, s2++) {
        if (*s1 == 0) {
            return 0;
        }
    }
    return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
}

static __inline int
oddc_strcmp_u_n(
    const NTWCHAR * s1,
    const char *s2)
{
    for (; ((char) *s1) == *s2; s1++, s2++) {
        if (*s1 == 0) {
            return 0;
        }
    }
    return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
}

static __inline int
oddc_strcmp_u_u(
    const NTWCHAR * s1,
    const NTWCHAR * s2)
{
    for (; *s1 == *s2; s1++, s2++) {
        if (*s1 == 0) {
            return 0;
        }
    }
    return ((*(unsigned char *) s1 < *(unsigned char *) s2) ? -1 : +1);
}

//===========

static __inline char *
oddc_strncpy_n_u(
    char *s1,
    const NTWCHAR * s2,
    size_t n)
{
    char *s = s1;
    while (n > 0 && *s2 != 0) {
        *s++ = (char) (*s2++);
        --n;
    }
    while (n > 0) {
        *s++ = 0;
        --n;
    }
    return s1;
}

static __inline NTWCHAR *
oddc_strncpy_u_n(
    NTWCHAR * s1,
    const char *s2,
    size_t n)
{
    NTWCHAR *s = s1;
    while (n > 0 && *s2 != 0) {
        *s++ = (NTWCHAR) (unsigned char) (*s2++);
        --n;
    }
    while (n > 0) {
        *s++ = 0;
        --n;
    }
    return s1;
}

static __inline NTWCHAR *
oddc_strncpy_u_u(
    NTWCHAR * s1,
    const NTWCHAR * s2,
    size_t n)
{
    NTWCHAR *s = s1;
    while (n > 0 && *s2 != 0) {
        *s++ = *s2++;
        --n;
    }
    while (n > 0) {
        *s++ = 0;
        --n;
    }
    return s1;
}

//===========

static __inline char *
oddc_strcpy_n_u(
    char *s1,
    const NTWCHAR * s2)
{
    char *s = s1;
    while ((*s++ = (char) (*s2++)) != 0) {
    }
    return s1;
}

static __inline NTWCHAR *
oddc_strcpy_u_n(
    NTWCHAR * s1,
    const char *s2)
{
    NTWCHAR *s = s1;
    while ((*s++ = (NTWCHAR) (unsigned char) (*s2++)) != 0) {
    }
    return s1;
}

static __inline NTWCHAR *
oddc_strcpy_u_u(
    NTWCHAR * s1,
    const NTWCHAR * s2)
{
    NTWCHAR *s = s1;
    while ((*s++ = *s2++) != 0) {
    }
    return s1;
}

//===========

static __inline size_t
oddc_strnlen_u(
    const NTWCHAR * s1,
    size_t n)
{
    size_t len = 0;
    for (; *s1 && (len < n); s1++, len++) {
    }
    return len;
}

static __inline size_t
oddc_strlen_u(
    const NTWCHAR * s1)
{
    size_t len = 0;
    for (; *s1; s1++, len++) {
    }
    return len;
}

//===========

static __inline void
oddc_memcpy_n_u(
    char *dst,
    const NTWCHAR * src,
    size_t n)
{
    while (n > 0) {
        *dst++ = (char) (*src++);
        n--;
    }
}

static __inline void
oddc_memcpy_u_n(
    NTWCHAR * dst,
    const char *src,
    size_t n)
{
    while (n > 0) {
        *dst++ = (NTWCHAR) (unsigned char) (*src++);
        n--;
    }
}

static __inline void
oddc_memcpy_u_u(
    NTWCHAR * dst,
    const NTWCHAR * src,
    size_t n)
{
    while (n > 0) {
        *dst++ = *src++;
        n--;
    }
}

//===========

// this name is bad - really is the opposite of memcmp()
static __inline int
oddc_memequal_n_u(
    const char *s1,
    const NTWCHAR * s2,
    size_t n)
{
    unsigned char u1, u2;
    for (; n--; s1++, s2++) {
        u1 = *(unsigned char *) s1;
        u2 = *(unsigned char *) s2;
        if (u1 != u2) {
            return 0;
        }
    }
    return 1;
}

// this name is bad - really is the opposite of memcmp()
static __inline int
oddc_memequal_u_n(
    const NTWCHAR * s1,
    const char *s2,
    size_t n)
{
    unsigned char u1, u2;
    for (; n--; s1++, s2++) {
        u1 = *(unsigned char *) s1;
        u2 = *(unsigned char *) s2;
        if (u1 != u2) {
            return 0;
        }
    }
    return 1;
}

// this name is bad - really is the opposite of memcmp()
static __inline int
oddc_memequal_u_u(
    const NTWCHAR * s1,
    const NTWCHAR * s2,
    size_t n)
{
    unsigned char u1, u2;
    for (; n--; s1++, s2++) {
        u1 = *(unsigned char *) s1;
        u2 = *(unsigned char *) s2;
        if (u1 != u2) {
            return 0;
        }
    }
    return 1;
}

static __inline size_t
oddc_memcount_equal_n_u(
    const char *src1,
    const NTWCHAR * src2,
    size_t length)
{
    size_t count = 0;
    const unsigned char *_src1 = (const unsigned char *) (src1);
    const unsigned short *_src2 = (const unsigned short *) (src2);
    while (count < length) {
        if (((unsigned char) *_src2) != *_src1) {
            break;
        }
        _src1++;
        _src2++;
        count++;
    }
    return count;
}

static __inline size_t
oddc_memcount_equal_u_n(
    const NTWCHAR * src1,
    const char *src2,
    size_t length)
{
    size_t count = 0;
    const unsigned short *_src1 = (const unsigned short *) (src1);
    const unsigned char *_src2 = (const unsigned char *) (src2);
    while (count < length) {
        if (((unsigned char) *_src1) != *_src2) {
            break;
        }
        _src1++;
        _src2++;
        count++;
    }
    return count;
}

//===========

static __inline char *
oddc_convert_u_to_n(
    const NTWCHAR * p_u_string,
    char *p_n_tmp_buffer,
    size_t sz_u_string,
    size_t sz_n_tmp_buffer)
{
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV) && !defined(I_AM_DEBUG_EXTENSION)
    CSX_ASSERT_H_RDC(sz_n_tmp_buffer >= (sz_u_string/sizeof(NTWCHAR)));
#endif
    CSX_UNREFERENCED_PARAMETER(sz_n_tmp_buffer);
    CSX_UNREFERENCED_PARAMETER(sz_u_string);
    oddc_strcpy_n_u(p_n_tmp_buffer, p_u_string);
    return p_n_tmp_buffer;
}

#define oddc_u_to_n(p_u_string, p_n_tmp_buffer) oddc_convert_u_to_n(p_u_string, p_n_tmp_buffer, sizeof(p_u_string), sizeof(p_n_tmp_buffer))

//===========

#endif                                     /* __oddc_h */
