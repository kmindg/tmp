#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_transport_packet_interface.h"

fbe_status_t fbe_sp_server_config_init(fbe_u32_t server_mode, fbe_u32_t sp_mode)
{
    fbe_status_t    status  = FBE_STATUS_GENERIC_FAILURE;

    /*start the packet server*/
    if(server_mode == FBE_TRANSPORT_SERVER_MODE_SIM)
    {
        fbe_api_common_init_sim();
        fbe_terminator_api_set_sp_id((sp_mode == FBE_TRANSPORT_SP_A) ? TERMINATOR_SP_A :TERMINATOR_SP_B);
    }
    else if(server_mode == FBE_TRANSPORT_SERVER_MODE_USER)
    {
        fbe_api_common_init_user(FBE_TRUE);
    }
    else
    {
        return status;
    }
    fbe_api_transport_init_server(sp_mode);
    fbe_api_transport_control_init_server_control();
    return FBE_STATUS_OK;
}

void fbe_sp_server_config_destroy(fbe_u32_t server_mode)
{    

    fbe_api_transport_destroy_server();
    if(server_mode == FBE_TRANSPORT_SERVER_MODE_SIM)
    {
        fbe_api_common_destroy_sim();
        fbe_terminator_api_destroy();
    }
    else if(server_mode == FBE_TRANSPORT_SERVER_MODE_USER)
    {
        fbe_api_common_destroy_user();
    }
}
