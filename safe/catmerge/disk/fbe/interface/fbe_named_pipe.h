#error

#ifndef FBE_NAMED_PIPE_H
#define FBE_NAMED_PIPE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"

/* We are not allow pipes in kernel */
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#error No pipes in kernel
#endif



#endif /* FBE_NAMED_PIPE */