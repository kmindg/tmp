// general OS header for 'data path' (kernel space on Windows, CSX equivalent on Linux) code
#ifdef ALAMOSA_WINDOWS_ENV
#include "csx_ext.h"
#include "k10ntddk.h"
#else
#include "csx_ext.h"
#include <safe_win_k_stoff.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
