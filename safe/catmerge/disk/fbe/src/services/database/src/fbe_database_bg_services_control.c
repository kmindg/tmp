#include "fbe_database_common.h"
#include "fbe_database_private.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_database_cmi_interface.h"

extern fbe_database_service_t fbe_database_service;

fbe_status_t database_stop_all_background_services(fbe_packet_t *packet)
{
    fbe_status_t		status = FBE_STATUS_OK;
    fbe_base_config_control_system_bg_service_t system_bg_service;
    fbe_database_control_update_peer_system_bg_service_t  db_system_bg_service;
    fbe_database_control_update_system_bg_service_t*  update_system_bg_service_p = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_system_bg_service_p, sizeof(*update_system_bg_service_p));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    system_bg_service.enable = FBE_FALSE;
    system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
    system_bg_service.issued_by_ndu = FBE_FALSE;
    system_bg_service.issued_by_scheduler = FBE_FALSE;

    /* update the control system background service */
    status = fbe_database_send_control_packet_to_class(&system_bg_service, 
                                                       sizeof(fbe_base_config_control_system_bg_service_t),
                                                       FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                       "%s: local send failed; status %d\n", __FUNCTION__, status);      
    }

    /* need to update the peer as well */
    if (update_system_bg_service_p->update_bgs_on_both_sp == FBE_TRUE) {
        
        if (is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            db_system_bg_service.system_bg_service.enable = FBE_FALSE;
            db_system_bg_service.system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 &system_bg_service,
                                                                 FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG);
        }
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

fbe_status_t database_restart_all_background_services(fbe_packet_t *packet)
{
    fbe_status_t		status = FBE_STATUS_OK;
    fbe_base_config_control_system_bg_service_t system_bg_service;

    fbe_database_control_update_peer_system_bg_service_t  db_system_bg_service;

    system_bg_service.enable = FBE_TRUE;
    system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
    system_bg_service.issued_by_ndu = FBE_FALSE;
    system_bg_service.issued_by_scheduler = FBE_FALSE;

    /* update the control system background service */
    status = fbe_database_send_control_packet_to_class(&system_bg_service, 
                                                       sizeof(fbe_base_config_control_system_bg_service_t),
                                                       FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);
    if ( status != FBE_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                       "%s local send failed; status %d\n", __FUNCTION__, status);      
    }
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        db_system_bg_service.system_bg_service.enable = FBE_TRUE;
        db_system_bg_service.system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             &system_bg_service,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG);
    }
    fbe_database_complete_packet(packet, status);
    return status;
}
