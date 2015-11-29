#include <windows.h>
#include "fbe_terminator_miniport_interface.h"
#include  "fbe/fbe_library_interface.h"

fbe_status_t 
fbe_cpd_shim_init(void)
{
    return FBE_STATUS_OK;
}

/* Testability hooks */
DWORD WINAPI fbe_cpd_shim_remote_ktrace(LPVOID lpParameter)
{
    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s lpParameter %p\n", __FUNCTION__, lpParameter);
    CSX_P_THR_EXIT();
    return 0;
}
