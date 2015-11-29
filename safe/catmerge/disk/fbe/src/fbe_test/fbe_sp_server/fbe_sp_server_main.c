#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe_sp_server.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_package.h"
#include "fbe_trace.h"
#include "fbe/fbe_trace_interface.h"
#include <signal.h>
#include "spid_user_interface.h"
#include "fbe/fbe_emcutil_shell_include.h"

static void fbe_sp_server_destroy(int exitCode);
static void CSX_GX_RT_DEFCC fbe_sp_server_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static fbe_transport_server_mode_t server_mode = FBE_TRANSPORT_SERVER_MODE_USER; // user by default

int __cdecl main(int argc, char **argv)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_transport_connection_target_t target_sp;
    fbe_u32_t pid;
    char title[50] = { 0 };
    SPID SpID;

#include "fbe/fbe_emcutil_shell_maincode.h"

    pid = csx_p_get_process_id();

    /*
     * This is supposed to allow a user to type ctrl-c to exit the server, when it is running within a cmd window
     */
    csx_rt_proc_cleanup_handler_register(fbe_sp_server_csx_cleanup_handler, fbe_sp_server_csx_cleanup_handler, NULL);

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);


    spidGetSpid(&SpID);

    if(SpID.engine == 1) {
        target_sp = FBE_TRANSPORT_SP_B;
    }
    else {
        target_sp = FBE_TRANSPORT_SP_A;
    }


    sprintf(title, "FBE SP Server %s(%s mode), Process ID:%d", (target_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB",
                (server_mode == FBE_TRANSPORT_SERVER_MODE_SIM) ? "SIM" : "USER", pid);
    (void) csx_uap_native_console_title_set(title);

    status = fbe_sp_server_config_init(server_mode, target_sp);
    if (status != FBE_STATUS_OK)
    {
        exit(-1);
    }

    EmcutilSleep(INFINITE);

    csx_rt_proc_cleanup_handler_deregister(fbe_sp_server_csx_cleanup_handler);

    fbe_sp_server_destroy(0);
}

fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}
static void fbe_sp_server_destroy(int exitCode)
{
    fbe_sp_server_config_destroy(server_mode);
    fbe_trace_destroy();
    fflush(stdout);
    exit(exitCode);
}

static void CSX_GX_RT_DEFCC
fbe_sp_server_csx_cleanup_handler(
    csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
        fbe_sp_server_destroy(0);
    }
}

