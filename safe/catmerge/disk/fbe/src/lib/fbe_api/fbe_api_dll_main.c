#include "windows.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe/fbe_api_user_server.h"
#include "fbe/fbe_api_discovery_interface.h"

#include "sep_dll.h"
#include "EmcUTIL_CsxShell_Interface.h"

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"

#if defined(SHARED_USERBOOTSTRAP)
#define USERBOOTSTRAP_ROUTINE_LINKAGE EMCPAL_IMPORT
#define USERBOOTSTRAP_SYMBOL_LINKAGE EMCPAL_IMPORT extern
#else
#define USERBOOTSTRAP_ROUTINE_LINKAGE
#define USERBOOTSTRAP_SYMBOL_LINKAGE extern
#endif

/*
 * Imported routines from a shared library
 * Or static linkage wthin this module
 */
USERBOOTSTRAP_ROUTINE_LINKAGE csx_nsint_t initPALandCSX_impl(csx_void_t);
USERBOOTSTRAP_ROUTINE_LINKAGE csx_nsint_t deinitPALandCSX_impl(csx_void_t);

static fbe_u32_t connect_retry_times = 10;


/* @!TODO: Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}


csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_startup(void)
{
    return CSX_TRUE;
}

csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_shutdown(void)
{
    return CSX_TRUE;
}


CSX_MOD_EXPORT fbe_status_t __cdecl  fbe_api_dll_init(char * str)
{
	fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: Entry with (%s)\n", __FUNCTION__,str);

	initPALandCSX_impl();

    return FBE_STATUS_OK;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  fbe_api_dll_destroy(void)
{
	deinitPALandCSX_impl();
    return FBE_STATUS_OK;
}


CSX_MOD_EXPORT fbe_status_t __cdecl  fbe_api_dll_init_sim(char * str)
{
	fbe_status_t status;

    status = fbe_api_common_init_sim();

    /*set function entries for commands that would go to the physical package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    /*set function entries for commands that would go to the sep package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);
    /*set function entries for commands that would go to the neit package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);
    
    /*need to initialize the client to connect to the server*/
    fbe_api_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

    status = fbe_api_transport_set_unregister_on_connect(FBE_TRUE);

    status = fbe_api_transport_init_client("10.245.189.34", FBE_TRANSPORT_SP_B, FBE_FALSE, connect_retry_times);

    /* need to do some additional setup on each SP */
    
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);

	/* Set control entry for drivers on SP server side.*/
    status = fbe_api_user_server_set_driver_entries();


	return FBE_STATUS_OK;
}

CSX_MOD_EXPORT fbe_u32_t __cdecl  fbe_api_dll_get_object_lifecycle_state(fbe_u32_t object_id, fbe_u32_t package_id)
{
	fbe_lifecycle_state_t lifecycle_state = 0;
	fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
	return lifecycle_state;
}
