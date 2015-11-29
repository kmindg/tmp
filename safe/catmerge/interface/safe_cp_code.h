// general OS header for 'control path' (user space) code
#ifdef ALAMOSA_WINDOWS_ENV
#include "csx_ext.h"
#include <windows.h>
#else
#include "csx_ext.h"
#include <safe_win_u_stoff.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
