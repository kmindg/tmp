#if !defined(FLARE_SYS_TYPES_H)
#define FLARE__STAR__H

#if defined(UMODE_ENV) || defined(SIMMODE_ENV) || (defined (K10_ENV) && defined (K10_WIN_SIM))
#define FLARE_SYS_TYPES_H 0x80000001	/* from common & system dirs */

// #include <sys/types.h>

#else
#define FLARE_SYS_TYPES_H 0x00000001	/* from common dir */
//typedef unsigned int size_t;

#endif

#endif /* !defined(FLARE_SYS_TYPES_H) */
