#include "fbe_test_package_config.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "simulation/cache_to_mcr_transport_server_interface.h"
#include "fbe/fbe_api_transport.h"

fbe_status_t fbe_sp_sim_config_init(fbe_u32_t sp_mode, fbe_u16_t sp_port_base, fbe_u16_t cmi_port_base, fbe_u16_t disk_sever_port_base)
{
    /*start the packet server*/
    fbe_api_common_init_sim();
    if(sp_port_base > 0)
    {
        fbe_api_sim_transport_set_server_mode_port(sp_port_base);
    }
    if(cmi_port_base > 0)
    {
        fbe_terminator_api_set_cmi_port_base(cmi_port_base);
    }
    if(disk_sever_port_base > 0)
    {
        fbe_terminator_api_set_simulated_drive_server_port(disk_sever_port_base);
    }

#if 0
    /*start the server that will accept IO from cache*/
    cache_to_mcr_transport_server_init(((sp_mode == FBE_SIM_SP_A) ? CACHE_TO_MCR_SP_A : CACHE_TO_MCR_SP_B));
#endif
    fbe_api_sim_transport_init_server(sp_mode);
    fbe_api_sim_transport_control_init_server_control();
    fbe_terminator_api_set_sp_id((sp_mode == FBE_SIM_SP_A) ? TERMINATOR_SP_A :TERMINATOR_SP_B);
    return FBE_STATUS_OK;
}

void fbe_sp_sim_config_destroy(void)
{    
#if 0
	cache_to_mcr_transport_server_destroy(0);
#endif
    fbe_api_transport_destroy_server();
	fbe_api_common_destroy_sim();
	fbe_terminator_api_destroy();
}
