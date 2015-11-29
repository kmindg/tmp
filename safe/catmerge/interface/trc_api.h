/* copyright */
/* trc_api.h 
 *
 * DESCRIPTION: This file should be included by any file containing
 *   TRC* statements.  It includes all the header files needed by
 *   such a source file.  Its main purpose is to mask any future
 *   changes to the include requirements.
 */
#if !defined(TRC_API_H)
#define TRC_API_H

#if defined(__cplusplus)
extern "C"
{
#endif

#if !defined(TRC_ENVIRON_H)
#include "trc_environ.h"
#endif

#if !defined(TRC_ENVIRON1_H)
#include "trc_environ1.h"
#endif

#if !defined(TRC_ENVIRON2_H)
#include "trc_environ2.h"
#endif

#if !defined(TRC_UTIL_H)
#include "trc_util.h"
#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(TRC_API_H) */
