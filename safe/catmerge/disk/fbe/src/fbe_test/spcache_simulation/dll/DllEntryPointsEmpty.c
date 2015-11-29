
/* new style DLL entry points (dummy variant) */

#include "EmcUTIL_CsxShell_Interface.h"

csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_startup(void)
{
    return CSX_TRUE;
}

csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_shutdown(void)
{
    return CSX_TRUE;
}
