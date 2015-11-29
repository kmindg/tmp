#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sas_physical_drive_private.h"

fbe_status_t 
fbe_sas_physical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry.\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, packet);

    return status;
}

fbe_status_t fbe_sas_physical_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_physical_drive));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sas_physical_drive_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sas_physical_drive_attach_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_attach_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_open_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_open_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_protocol_edge_detached_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_protocol_edge_detached_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_protocol_edge_closed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_protocol_edge_closed_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_vpd_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_vpd_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_identity_invalid_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_identity_invalid_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_test_unit_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_test_unit_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_not_spinning_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_not_spinning_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_get_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_get_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_release_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_release_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_get_mode_pages_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_get_mode_pages_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_set_mode_pages_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_set_mode_pages_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_get_mode_page_attributes_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_get_mode_page_attributes_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_read_capacity_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_read_capacity_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_detach_ssp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_detach_ssp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sas_physical_drive_reset_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_reset_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_protocol_edges_not_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t sas_physical_drive_protocol_edges_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_check_drive_busy_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sas_physical_drive_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_physical_drive_fw_download_cond_peer_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_physical_drive_abort_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t           sas_physical_drive_fw_download(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_packet_t * u_packet);
static fbe_status_t           sas_physical_drive_fw_download_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_enter_power_save(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_bool_t passive);
static fbe_status_t sas_physical_drive_enter_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sas_physical_drive_power_save_active_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_physical_drive_power_save_passive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_physical_drive_exit_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_physical_drive_disk_collect_flush_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_disk_collect_flush_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_physical_drive_health_check_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static void sas_physical_drive_update_dieh_health_check_stats(fbe_sas_physical_drive_t * sas_physical_drive);
static fbe_status_t sas_physical_drive_send_health_check(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * monitor_packet);
static fbe_status_t sas_physical_drive_send_health_check_completion(fbe_packet_t * usurper_packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sas_physical_drive_initiate_health_check_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t           sas_physical_drive_initiate_health_check_no_pvd_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sas_physical_drive_health_check_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t sas_physical_drive_dump_disk_log_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_dump_disk_log_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sas_physical_drive_discovery_poll_per_day_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_discovery_poll_per_day_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sas_physical_drive_monitor_discovery_edge_in_fail_state_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
    
/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_physical_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_physical_drive);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sas_physical_drive,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_physical_drive_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_physical_drive_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_protocol_edges_not_ready_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY,
        sas_physical_drive_protocol_edges_not_ready_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_attach_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE,
        sas_physical_drive_attach_protocol_edge_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_open_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE,
        sas_physical_drive_open_protocol_edge_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        sas_physical_drive_discovery_poll_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_reset_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET,
        sas_physical_drive_reset_cond_function)
};


static fbe_lifecycle_const_cond_t sas_physical_drive_fw_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD,
        sas_physical_drive_fw_download_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_fw_download_peer_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER,
        sas_physical_drive_fw_download_cond_peer_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_abort_fw_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD,
        sas_physical_drive_abort_fw_download_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE,
        sas_physical_drive_power_save_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_active_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE,
        sas_physical_drive_power_save_active_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_passive_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE,
        sas_physical_drive_power_save_passive_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_exit_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_EXIT_POWER_SAVE,
        sas_physical_drive_exit_power_save_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_disk_collect_flush_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH,
        sas_physical_drive_disk_collect_flush_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_discovery_poll_per_day_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL_PER_DAY,
        sas_physical_drive_discovery_poll_per_day_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_health_check_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK,
        sas_physical_drive_health_check_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_initiate_health_check_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK,
        sas_physical_drive_initiate_health_check_cond_function)
};

static fbe_lifecycle_const_cond_t sas_physical_drive_health_check_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE,
        sas_physical_drive_health_check_complete_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/
static fbe_lifecycle_const_base_cond_t sas_physical_drive_protocol_edge_detached_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_protocol_edge_detached_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_DETACHED,
        sas_physical_drive_protocol_edge_detached_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/*--- constant base condition entries --------------------------------------------------*/
static fbe_lifecycle_const_base_cond_t sas_physical_drive_check_drive_busy_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_check_drive_busy_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_DRIVE_BUSY,
        sas_physical_drive_check_drive_busy_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* OBSOLETE - TODO: REMOVE THIS  */

static fbe_lifecycle_const_base_cond_t sas_physical_drive_protocol_edge_closed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_protocol_edge_closed_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_CLOSED,
        sas_physical_drive_protocol_edge_closed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY condition */
static fbe_lifecycle_const_base_cond_t sas_physical_drive_inquiry_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_inquiry_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_INQUIRY,
        sas_physical_drive_inquiry_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */   
        FBE_LIFECYCLE_STATE_READY,         /* READY */   
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY condition */
static fbe_lifecycle_const_base_cond_t sas_physical_drive_vpd_inquiry_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_vpd_inquiry_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_VPD_INQUIRY,
        sas_physical_drive_vpd_inquiry_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_identity_invalid_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_identity_invalid_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_IDENTITY_INVALID,
        sas_physical_drive_identity_invalid_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t sas_physical_drive_test_unit_ready_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_test_unit_ready_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_TEST_UNIT_READY,
        sas_physical_drive_test_unit_ready_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_not_spinning_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_not_spinning_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_NOT_SPINNING,
        sas_physical_drive_not_spinning_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_get_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_get_permission_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT,
        sas_physical_drive_get_permission_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_release_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_release_permission_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT,
        sas_physical_drive_release_permission_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t sas_physical_drive_release_permission_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "sas_physical_drive_release_permission_timer_cond",
            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT_TIMER,
            sas_physical_drive_release_permission_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),           /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    1200 /* fires after 12 seconds */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_get_mode_pages_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_get_mode_pages_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES,
        sas_physical_drive_get_mode_pages_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_set_mode_pages_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_set_mode_pages_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_MODE_PAGES,
        sas_physical_drive_set_mode_pages_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_get_mode_page_attributes_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_get_mode_page_attributes_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGE_ATTRIBUTES,
        sas_physical_drive_get_mode_page_attributes_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_read_capacity_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_read_capacity_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_CAPACITY,
        sas_physical_drive_read_capacity_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_DETACH_SSP_EDGE condition 
    This condition is preset in destroy rotary
*/
static fbe_lifecycle_const_base_cond_t sas_physical_drive_detach_ssp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_detach_ssp_edge_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_DETACH_SSP_EDGE,
        sas_physical_drive_detach_ssp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_physical_drive_dump_disk_log_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_dump_disk_log_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_DUMP_DISK_LOG,
        sas_physical_drive_dump_disk_log_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t sas_physical_drive_monitor_discovery_edge_in_fail_state_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_physical_drive_monitor_discovery_edge_in_fail_state_cond",
        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_MONITOR_DISCOVERY_EDGE_IN_FAIL_STATE,
        sas_physical_drive_monitor_discovery_edge_in_fail_state_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,           /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY),       /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_physical_drive)[] = {
    &sas_physical_drive_protocol_edge_detached_cond,
    &sas_physical_drive_protocol_edge_closed_cond,
    &sas_physical_drive_inquiry_cond,
    &sas_physical_drive_vpd_inquiry_cond,
    &sas_physical_drive_identity_invalid_cond,
    &sas_physical_drive_test_unit_ready_cond,
    &sas_physical_drive_not_spinning_cond,
    &sas_physical_drive_get_permission_cond,
    &sas_physical_drive_release_permission_cond,
    (fbe_lifecycle_const_base_cond_t *)&sas_physical_drive_release_permission_timer_cond,
    &sas_physical_drive_get_mode_pages_cond,
    &sas_physical_drive_set_mode_pages_cond,
    &sas_physical_drive_get_mode_page_attributes_cond,
    &sas_physical_drive_read_capacity_cond,
    &sas_physical_drive_detach_ssp_edge_cond,
    &sas_physical_drive_dump_disk_log_cond,
    &sas_physical_drive_monitor_discovery_edge_in_fail_state_cond,   
    &sas_physical_drive_check_drive_busy_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_physical_drive);


static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_specialize_rotary[] = {
    /* Derived conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_attach_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */   
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_protocol_edge_detached_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_protocol_edge_closed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_inquiry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};


static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* must come before fw_download cond */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_fw_download_peer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_reset_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_protocol_edge_closed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_identity_invalid_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_get_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_not_spinning_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_release_permission_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_test_unit_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_release_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /*VPD inquiry must be sent after a TUR and release permission condition.*/
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_vpd_inquiry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /*As part of the support for generic drive vendors get_mode_pages_cond and
     *  get_mode_page_attributes_cond are no longer preset since mode page operations
     *  will NOT be preformed on generic drives.  For all other vendor ID's they will be set 
     *  in the function sas_physical_drive_inquiry_cond_completion.
     */  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_get_mode_pages_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_set_mode_pages_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_get_mode_page_attributes_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_read_capacity_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_health_check_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_initiate_health_check_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* handles cleanup of health check.  must be at end of rotary after any additional recovery actions such as reset. */  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_health_check_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};


static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_ready_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* no dependencies */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_discovery_poll_per_day_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_disk_collect_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_check_drive_busy_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_get_mode_pages_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_set_mode_pages_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_hibernate_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* no dependencies */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_active_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_passive_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_exit_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
    
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_fail_rotary[] = {
    /* Derived conditions */    
    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_disk_collect_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_dump_disk_log_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_release_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_monitor_discovery_edge_in_fail_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};


static fbe_lifecycle_const_rotary_cond_t sas_physical_drive_destroy_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_physical_drive_detach_ssp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_physical_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_physical_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sas_physical_drive_activate_rotary),

    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_physical_drive_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, sas_physical_drive_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, sas_physical_drive_fail_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, sas_physical_drive_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_physical_drive);

/*--- global sas enclosure lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sas_physical_drive,
        FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_physical_drive))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
sas_physical_drive_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
sas_physical_drive_attach_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_attach_protocol_edge_cond_completion, sas_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sas_physical_drive_attach_ssp_edge(sas_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_attach_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_open_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_open_protocol_edge_cond_completion, sas_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sas_physical_drive_open_ssp_edge(sas_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_open_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_protocol_edge_detached_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_protocol_edge_detached_cond_completion, sas_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sas_physical_drive_attach_ssp_edge(sas_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_protocol_edge_detached_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

/* OBSOLETE - TODO: REMOVE THIS */

static fbe_lifecycle_status_t 
sas_physical_drive_protocol_edge_closed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_protocol_edge_closed_cond_completion, sas_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sas_physical_drive_open_ssp_edge(sas_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/* OBSOLETE - TODO: REMOVE THIS */
static fbe_status_t 
sas_physical_drive_protocol_edge_closed_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_path_attr_t path_attr;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &path_attr);

    if(path_attr & FBE_SSP_PATH_ATTR_CLOSED){
        
        /* Check protocol address */
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                (fbe_base_object_t*)sas_physical_drive, 
                FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_PROTOCOL_ADDRESS);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set get_protocol_address condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    
    /* We need to check some conditions after protocol edge is opened */
    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)sas_physical_drive, 
            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_IDENTITY_INVALID);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set identity_invalid condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)sas_physical_drive, 
            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_TEST_UNIT_READY);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set test_unit_ready condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;
    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)sas_physical_drive, 
            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set get_mode_pages condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sas_physical_drive_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Allocate a buffer */
    drive_info = (fbe_physical_drive_information_t *)fbe_transport_allocate_buffer();
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);
        fbe_transport_release_buffer(drive_info);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION, 
                                        drive_info, 
                                        sizeof(fbe_physical_drive_information_t));  

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_inquiry_cond_completion, sas_physical_drive);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    /* Call the executer function to do actual job */
    status = fbe_sas_physical_drive_send_inquiry(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    fbe_u32_t powersave_attr = 0;
    fbe_u32_t lpq_timer_ms = 0, hpq_timer_ms = 0;
    fbe_bool_t timer_changed = FBE_FALSE;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    fbe_payload_control_get_buffer(control_operation, &drive_info);
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK;
    }

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        /* We can't get the inquiry string, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_transport_release_buffer(drive_info);
        return FBE_STATUS_OK; 
    }

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        /* Copy drive info */
        fbe_copy_memory(info_ptr->serial_number, drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
        fbe_copy_memory(info_ptr->vendor_id, drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
        info_ptr->drive_vendor_id = drive_info->drive_vendor_id;
        fbe_copy_memory(info_ptr->product_id, drive_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1); 
        fbe_copy_memory(info_ptr->revision, drive_info->product_rev, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);       
        fbe_copy_memory(info_ptr->part_number, drive_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1); 
        fbe_copy_memory(info_ptr->tla, drive_info->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE + 1);
        fbe_copy_memory(info_ptr->bridge_hw_rev, "N/A", 3);
        info_ptr->product_rev_len = FBE_SCSI_INQUIRY_REVISION_SIZE;
        info_ptr->drive_price_class = drive_info->drive_price_class;
        info_ptr->drive_parameter_flags = drive_info->drive_parameter_flags;
        
        //copy SN into performance data, trim whitespace
        if (sas_physical_drive->performance_stats.counter_ptr.pdo_counters) 
        {
           fbe_u32_t i;
           fbe_copy_memory(&sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number, info_ptr->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1); 
           for (i = 0; i < FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE; i++) 
           {
              if (sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number[i] == '\0') 
              { //we're already null-terminated, good to go.
                 break;
              }
              else if (sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number[i] == ' ')
              { //change it to null terminator and break
                 sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number[i] = '\0';
                 break;
              }
           }       
        }
        
        /*As part of the support for generic drive vendors get_mode_pages_cond and
         *  get_mode_page_attributes_cond are no longer preset since mode page operations
         *  will NOT be preformed on generic drives.  For all other drive vendor ID's
         *  they are set below.
         */  
         if ( !(fbe_equal_memory(info_ptr->vendor_id, "GENERIC", 7)) &&
              !(fbe_equal_memory(info_ptr->vendor_id, "SATA-GEN", 8)) )
         {                
            sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES);
            
            if (status != FBE_STATUS_OK)
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                        FBE_TRACE_LEVEL_ERROR,
                        "%s can't set GET_MODE_PAGES condition, status: 0x%X\n",
                        __FUNCTION__, status);   
            }
            
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGE_ATTRIBUTES);
            
            if (status != FBE_STATUS_OK)
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                        FBE_TRACE_LEVEL_ERROR,
                        "%s can't set GET_MODE_PAGE_ATTRIBUTES condition, status: 0x%X\n",
                        __FUNCTION__, status);   
            }            
        }
 
        if (drive_info->spin_down_qualified) {
            fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
            powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED;
            fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);
        }

        /* By the time this completion function is called, we should know the drive type */
        if ((drive_info->drive_parameter_flags & FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN) > 0) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                    "%s FBE_PDO_FLAG_DRIVE_TYPE_UNKOWN is set. \n", __FUNCTION__);              
        }

        /* change to subclass if applicable
         */
        if ( ((drive_info->drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE) > 0) || ((drive_info->drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) > 0) )
        {
            fbe_base_object_change_class_id((fbe_base_object_t *) sas_physical_drive, FBE_CLASS_ID_SAS_FLASH_DRIVE);
        }
        else if (drive_info->paddlecard)   // if not flash and has a paddlecard then this is a SATA PADDLECARD subclass.
                                            // note: flash drives can have a sata paddlecards, but will always be
                                            // handled as a flash_drive subclass.  This subclass is only for 
                                            // "hard drives" with a sata paddlecard.
        {
            fbe_base_object_change_class_id((fbe_base_object_t *) sas_physical_drive, FBE_CLASS_ID_SATA_PADDLECARD_DRIVE);
        }

        /* Now we can get timer setting from DCS */
        if (fbe_sas_physical_drive_get_queuing_info_from_DCS(sas_physical_drive, &lpq_timer_ms, &hpq_timer_ms) == FBE_STATUS_OK) {
            fbe_sas_physical_drive_set_enhanced_queuing_timer(sas_physical_drive, lpq_timer_ms, hpq_timer_ms, &timer_changed);
        }
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_buffer(drive_info);

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_identity_invalid_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Allocate a buffer */
    drive_info = (fbe_physical_drive_information_t *)fbe_transport_allocate_buffer();
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);
        fbe_transport_release_buffer(drive_info);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION, 
                                        drive_info, 
                                        sizeof(fbe_physical_drive_information_t));  

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_identity_invalid_cond_completion, sas_physical_drive);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    /* Call the executer function to do actual job */
    status = fbe_sas_physical_drive_send_inquiry(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_identity_invalid_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;
    fbe_u32_t powersave_attr = 0;
    fbe_bool_t                           fw_changed = FBE_FALSE;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s, entry\n",
            __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);
    fbe_payload_control_get_buffer(control_operation, &drive_info);
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK;
    }

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        /* We can't get the inquiry string, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        fbe_transport_release_buffer(drive_info);
        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK; 
    }

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        if (/* fbe_equal_memory(info_ptr->vendor_id, drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE) && */
            fbe_equal_memory(info_ptr->serial_number, drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE))
        {
            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't clear current condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }

            /*  Detect if the FW has changed due to FW download */
            if(!fbe_equal_memory(info_ptr->revision, drive_info->product_rev,   drive_info->product_rev_len)              ||
               strncmp(info_ptr->tla,         drive_info->tla_part_number,      FBE_SCSI_INQUIRY_TLA_SIZE+1)         != 0 ||
               strncmp(info_ptr->part_number, drive_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1) != 0 ||
               strncmp(info_ptr->product_id,  drive_info->product_id,           FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1)  != 0 ||
               strncmp(info_ptr->vendor_id,   drive_info->vendor_id,            FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1)   != 0)
            {
                fw_changed = FBE_TRUE;
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,FBE_TRACE_LEVEL_INFO,
                                                           "SASPDO Notify DMO FW changed\n");
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,FBE_TRACE_LEVEL_INFO,
                                                           "SASPDO Old Inquiry [%s,%s,%s,%s,%s]\n",
                                                           info_ptr->tla, info_ptr->revision, info_ptr->part_number, info_ptr->product_id, info_ptr->vendor_id);
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,FBE_TRACE_LEVEL_INFO,
                                                           "SASPDO New Inquiry [%s,%s,%s,%s,%s]\n",
                                                           drive_info->tla_part_number, drive_info->product_rev, drive_info->dg_part_number_ascii, drive_info->product_id, drive_info->vendor_id);
            }

            fbe_copy_memory(info_ptr->revision, drive_info->product_rev, drive_info->product_rev_len + 1);
            fbe_copy_memory(info_ptr->vendor_id, drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
            info_ptr->drive_vendor_id = drive_info->drive_vendor_id;
            fbe_copy_memory(info_ptr->product_id, drive_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1); 
            fbe_copy_memory(info_ptr->part_number, drive_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1); 
            fbe_copy_memory(info_ptr->tla, drive_info->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE + 1);
            info_ptr->drive_price_class = drive_info->drive_price_class;

            fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
            if (drive_info->spin_down_qualified) {
                powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED;
            }
            else
            {
                powersave_attr &= ~FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED;
            }
            fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);

            /* Notify DMO, which notifies Admin, if the FW has changed */
            if (fw_changed == FBE_TRUE)
            {
                dataChangeInfo.phys_location.bus = base_physical_drive->port_number;
                dataChangeInfo.phys_location.enclosure = base_physical_drive->enclosure_number;
                dataChangeInfo.phys_location.slot = (fbe_u8_t)base_physical_drive->slot_number;
                dataChangeInfo.data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
                dataChangeInfo.device_mask = FBE_DEVICE_TYPE_DRIVE;
                fbe_base_physical_drive_send_data_change_notification(base_physical_drive, &dataChangeInfo);
            }
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  "%s validation failed\n",
                                  __FUNCTION__);

            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set DESTROY condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }        
        } 
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    fbe_transport_release_buffer(drive_info);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_test_unit_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_INFO,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_test_unit_ready_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = fbe_sas_physical_drive_send_tur(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_test_unit_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;

    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "PDO TUR compl: Invalid packet status: 0x%X\n",
                                status);
          
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {       
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "PDO TUR compl: can't clear current condition, status: 0x%X",
                                    status);
        }        

        sas_physical_drive->sas_drive_info.test_unit_ready_retry_count = 0;
    }
    else if ((control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NOT_SPINNING) || 
             (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_BECOMING_READY))
    {
        if (sas_physical_drive->sas_drive_info.test_unit_ready_retry_count == 0)
        {
            sas_physical_drive->sas_drive_info.test_unit_ready_retry_count++;

            /* Set FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT condition. */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                     (fbe_base_object_t*)sas_physical_drive, 
                     FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR,
                                      "PDO TUR compl: can't set GET_PERMISSION condition, status: 0x%X\n",
                                      status);
            }
            /* Set FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_NOT_SPINNING condition. */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                     (fbe_base_object_t*)sas_physical_drive, 
                     FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_NOT_SPINNING);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR,
                                      "PDO TUR compl: can't set NOT_SPINNING condition, status: 0x%X\n",
                                      status);
            }

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                        FBE_TRACE_LEVEL_INFO,
                                                        "PDO TUR compl:1st time, Drv not spinning. Retry_TUR count:%d,cs:%d,csq:%d\n",
                                                        sas_physical_drive->sas_drive_info.test_unit_ready_retry_count,
                                                        control_status, control_status_qualifier);
        }
        else if (sas_physical_drive->sas_drive_info.test_unit_ready_retry_count < (FBE_MAX_TUR_RETRY_COUNT))
        {
            sas_physical_drive->sas_drive_info.test_unit_ready_retry_count++;
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                        FBE_TRACE_LEVEL_INFO,
                                                        "PDO TUR compl:drive still not spinning. Retry_TUR count:%d,cs:%d,csq:%d\n",
                                                        sas_physical_drive->sas_drive_info.test_unit_ready_retry_count,
                                                        control_status, control_status_qualifier);
        }
        else
        {
            /* Set the BO:FAIL lifecycle condition */
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING);

            /* We need to set the BO:FAIL lifecycle condition */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "PDO TUR compl: can't set FAIL condition, status: 0x%X\n",
                                      status);
            }
           

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                        FBE_TRACE_LEVEL_INFO,
                                                        "PDO TUR compl: set the drive faulted after %d TUR(s)\n",
                                                        sas_physical_drive->sas_drive_info.test_unit_ready_retry_count);

            // Clear the TUR retry count just in case lifecycle is switched from FAIL to ACTIVATE
            sas_physical_drive->sas_drive_info.test_unit_ready_retry_count = 0;
        }
    }
    else if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) 
    {
        status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                          (fbe_base_object_t*)sas_physical_drive,  
                                          0);

    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_not_spinning_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_INFO,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_not_spinning_cond_completion, sas_physical_drive);

    /* Call the executor function to do spinup */
    status = fbe_sas_physical_drive_spin_down(sas_physical_drive, packet, TRUE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_not_spinning_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    fbe_payload_control_status_qualifier_t control_status_qualifier;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "PDO not spinning compl: Invalid packet status: 0x%X\n",
                                status);
        return FBE_STATUS_OK; 
    }

    /* Check the control status */    
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {    

#if 0 /* This is the logic from R29.  */
        if (control_status_qualifier == FBE_SAS_DRIVE_STATUS_HARD_ERROR)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                       FBE_TRACE_LEVEL_INFO,
                                                       "%s Spin up hard error\n",
                                                       __FUNCTION__);

            /* Set the BO:FAIL lifecycle condition */
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, 
                                                 FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN);

            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                           FBE_TRACE_LEVEL_ERROR,
                                                           "%s can't set FAIL condition, status: 0x%X\n",
                                                           __FUNCTION__, status);
            }
        }
#endif

        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
        
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                            FBE_TRACE_LEVEL_INFO,
                                            "PDO not spinning compl: status:0x%X, cs:%d, csq=%d\n",
                                            status, control_status, control_status_qualifier);

        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "PDO not spinning compl: can't clear current condition, status: 0x%X\n",
                                status);
    }

    /* Start a timer to release spinup credit. */
    status = fbe_lifecycle_force_clear_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT_TIMER);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "PDO not spinning compl: can't start a timer to release spinup credit, status: 0x%X\n",
                                status);
    }

    /*increment the number of spinups we did*/
    base_physical_drive->drive_info.spinup_count++;

    /* Notify DMO, which notifies Admin */
    dataChangeInfo.phys_location.bus = base_physical_drive->port_number;
    dataChangeInfo.phys_location.enclosure = base_physical_drive->enclosure_number;
    dataChangeInfo.phys_location.slot = (fbe_u8_t)base_physical_drive->slot_number;
    dataChangeInfo.data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
    dataChangeInfo.device_mask = FBE_DEVICE_TYPE_DRIVE;
    fbe_base_physical_drive_send_data_change_notification(base_physical_drive, &dataChangeInfo);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_INFO,
                                "PDO not spinning compl: spinup_count: %d\n",
                                base_physical_drive->drive_info.spinup_count);

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_get_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_path_attr_t path_attr;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_INFO,
                          "%s entry\n", __FUNCTION__);
    
    /* Check path state. If the enclosure has failed ( path state is broken )
     *   or is about to be destroyed ( path state is gone)
     *   we will clear this condition to prevent "invalid packet status" error messages from
     *   the associated base physical drive executer completion from filling the trace.
     */
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sas_physical_drive, &path_state);
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN || path_state == FBE_PATH_STATE_GONE)
    {
        /* Path is broken. Clear this condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        "%s can't clear current condition, status: 0x%X\n",
                                                        __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;       
    }   
    
    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sas_physical_drive, &path_attr);

    /* hack here! Always set PERMISSION_GRANTED until enclosure object implemented it */
    //path_attr |= FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED;

    if((status == FBE_STATUS_OK) && (path_attr & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: spin-up credit granted.\n",
                                __FUNCTION__);
        /* Permission granted. Clear the condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else if ((status == FBE_STATUS_OK) && (path_attr & FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING))
    {
        /* Enclosure object got the request. So we don't need to send it again. */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_get_permission_cond_completion, sas_physical_drive);

    /* Call the executor function to get permission */
    status = fbe_base_physical_drive_get_permission((fbe_base_physical_drive_t *)sas_physical_drive, packet, TRUE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_get_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    /* Check the control status */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s Invalid control_status: 0x%X\n",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sas_physical_drive_release_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_path_attr_t path_attr;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);
    
    /* Check path state. If the enclosure has failed ( path state is broken )
     *   or is about to be destroyed ( path state is gone)  
     *   we will clear this condition to prevent "invalid packet status" error messages from
     *   the associated base physical drive executer completion from filling the trace.
     */
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sas_physical_drive, &path_state);
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN || path_state == FBE_PATH_STATE_GONE)
    {
        /* Path is broken. Clear this condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        "%s can't clear current condition, status: 0x%X\n",
                                                        __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;       
    }       

    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sas_physical_drive, &path_attr);

    if((status == FBE_STATUS_OK) && !(path_attr & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_release_permission_cond_completion, sas_physical_drive);

    /* Call the executor function */
    status = fbe_base_physical_drive_get_permission((fbe_base_physical_drive_t *)sas_physical_drive, packet, FALSE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*
 This condition function checks if the drive is busy, also updates a timestamp indicating the last time
 such check was performed. This information is used by perfstats to perform analysis of idle and busy ticks.
 Due to the design of only incrementing busy/idle ticks on state transition, if drive then stays either 100% 
 busy or 100% idle, then we won't cacl anymore ticks. This will tell perfstats what state we are in so it can 
 extrapolate the ticks to give an accurate value within the 3 sec montior polling cycle.
*/
static fbe_lifecycle_status_t 
sas_physical_drive_check_drive_busy_cond_function (fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_pdo_performance_counters_t * temp_pdo_counters = NULL;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_time_t current_time_and_bit;  

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s Entered disk busy monitor\n",
            __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    /* Save a local copy of the pointer to pdo_counters in case it's set to NULL before we access it */
    temp_pdo_counters = sas_physical_drive->performance_stats.counter_ptr.pdo_counters;
    if (sas_physical_drive->b_perf_stats_enabled && temp_pdo_counters)
    {   
        /* Perfstats is enable for this drive, do the following:
         1. Update timestamp for last monitor poll
         2. Check for outstanding IO on the PDO.
         3. If != 0, set timestamp's first bit to 1 (busy)         
        */
        current_time_and_bit = fbe_get_time_in_us();
            
        /*
         Use least significant bit of the timestamp.
         */
        if (base_physical_drive->stats_num_io_in_progress != 0)
        {            
            // Set the bit.
            current_time_and_bit |= FBE_PERFSTATS_BUSY_STATE;        
        }
        else
        {
            // Clear the bit.            
            current_time_and_bit &= ~FBE_PERFSTATS_BUSY_STATE;
        }

        /* Set busy state on timestamp. Client need to clear this bit before using timestamp.
         */
        temp_pdo_counters->last_monitor_timestamp = current_time_and_bit;

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s current_time_and_bit: %llu \n",
                            __FUNCTION__,current_time_and_bit);

        return FBE_LIFECYCLE_STATUS_DONE;
    }       

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);

    if (status != FBE_STATUS_OK) {
         fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_ERROR,
                            "%s can't clear current condition, status: 0x%X\n",
                             __FUNCTION__, status);
    }


    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t 
sas_physical_drive_release_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    /* Check the control status */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s Invalid control_status: 0x%X\n",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: released spin-up credit.\n",
                                __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_get_mode_pages_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);


    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing in the Activate rotary, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_get_mode_pages_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = fbe_sas_physical_drive_mode_sense_10(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_get_mode_pages_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }

        /* Set set_mode_page condition. */
        if (sas_physical_drive->sas_drive_info.do_mode_select == TRUE)
        {            
            if (fbe_dcs_param_is_enabled(FBE_DCS_MODE_SELECT_CAPABILITY))
            {  
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_MODE_PAGES);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set SET_MODE_PAGES condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }  
            }
            else
            {
                 fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                          "%s FBE_DCS_MODE_SELECT_CAPABILITY disabled\n", __FUNCTION__);
            }
        }
        else
        {
            /* Nothing to select.  Always clear just incase MS was issued by cli */
            sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;  
        }
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_get_mode_page_attributes_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing in the Activate rotary, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_get_mode_page_attributes_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = fbe_sas_physical_drive_mode_sense_page(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_get_mode_page_attributes_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    fbe_u8_t state = sas_physical_drive->sas_drive_info.ms_state;
    fbe_u8_t max_pages;
    
    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_sas_physical_drive_get_current_page(sas_physical_drive, NULL, &max_pages);
        if (state >= (max_pages - 1))
        {
            sas_physical_drive->sas_drive_info.ms_state = 0;
            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
            }
        }
        else
        {
            sas_physical_drive->sas_drive_info.ms_state++;
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_set_mode_pages_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* debug hook */
    fbe_base_object_check_hook(p_object,  SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                               FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_SELECT, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        /* pause this condition by preventing it form completing. */
        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_set_mode_pages_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = fbe_sas_physical_drive_mode_select_10(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_set_mode_pages_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Always clear this just incase MS was issued from cli */
    sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }

        /* We do mode select at most 2 times */
        sas_physical_drive->sas_drive_info.ms_retry_count++;
        if (sas_physical_drive->sas_drive_info.ms_retry_count < 2)
        {
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set GET_MODE_PAGES condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
        else
        {
            sas_physical_drive->sas_drive_info.ms_retry_count = 0;
        }
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_read_capacity_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing in the Activate rotary, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_read_capacity_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = fbe_sas_physical_drive_read_capacity_16(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_read_capacity_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_physical_drive,  
                                              0);

        }
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_detach_ssp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t *)base_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_detach_ssp_edge_cond_completion, sas_physical_drive);

    fbe_sas_physical_drive_detach_edge(sas_physical_drive, packet);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_detach_ssp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

     
    fbe_sas_physical_drive_init(sas_physical_drive);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_physical_drive_reset_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* debug hook */
    fbe_base_object_check_hook(base_object,  SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                               FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_RESET, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        /* pause this condition by preventing it from completing. */
        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_reset_cond_completion, sas_physical_drive);

    /* Call the usurper function to do actual job */
    fbe_sas_physical_drive_reset_device(sas_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_reset_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    fbe_base_physical_drive_reset_completed(base_physical_drive);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    base_physical_drive->physical_drive_error_counts.reset_count++;

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sas_physical_drive_protocol_edges_not_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;
    fbe_path_attr_t path_attr;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_ERROR;

    sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Debug Hook to hold PDO in this condition 
    */
    fbe_base_object_check_hook(p_object,  SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                               FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_EDGES_NOT_READY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    /* All protocol edge manipulations are synchronous - ther are no hardware envolvment
     * We do not have to break the execution context.
     */
    fbe_semaphore_init(&sem, 0, 1);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_protocol_edges_not_ready_cond_completion, &sem);

    /* Call the executer function to do actual job */
    fbe_base_physical_drive_get_protocol_address((fbe_base_physical_drive_t *)sas_physical_drive, packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_payload_control_get_status(control_operation, &control_status);
    status = fbe_transport_get_status_code(packet);
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK)){
        /* We were unable to get protocol address. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                   "sas protocol_edges_not_ready_cond: status failed. pkt_status:%d cntrl_status:%d\n", status, control_status);
        fbe_semaphore_destroy(&sem);
        fbe_transport_complete_packet(packet);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Check if we need to attach protocol edge */
    /* Get the path state of the edge */
    fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &path_state);
    if(path_state == FBE_PATH_STATE_INVALID){ /* Not attached */
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, sas_physical_drive_protocol_edges_not_ready_cond_completion, &sem);
        /* Call the userper function to do actual job */
        fbe_sas_physical_drive_attach_ssp_edge(sas_physical_drive, packet);
        fbe_semaphore_wait(&sem, NULL); /* Wait for completion */
        /* Check path state again */
        fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &path_state);
        if(path_state == FBE_PATH_STATE_INVALID){ /* Not attached */
            /* We were unable to attach ssp edge. */
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                       "sas protocol_edges_not_ready_cond: edge invalid\n");
            fbe_semaphore_destroy(&sem);
            fbe_transport_complete_packet(packet);
            return FBE_LIFECYCLE_STATUS_PENDING;        
        }
    }

    /* Check if path state is ENABLED */
    if(path_state != FBE_PATH_STATE_ENABLED){ /* We should wait until path state will become ENABLED */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                   "sas protocol_edges_not_ready_cond: edge not enabled: state:%d \n", path_state);
        fbe_semaphore_destroy(&sem);
        fbe_transport_complete_packet(packet);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    /* Check if our edge is closed */
    /* Check the path attributes of ssp edge */
    fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &path_attr);

    //if(path_attr & FBE_SSP_PATH_ATTR_CLOSED){ /* The edge is closed */
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, sas_physical_drive_protocol_edges_not_ready_cond_completion, &sem);
        /* Call the userper function to do actual job */
        fbe_sas_physical_drive_open_ssp_edge(sas_physical_drive, packet);
        fbe_semaphore_wait(&sem, NULL); /* Wait for completion */
        /* Check path attributes again again */
        fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &path_attr);
        if(path_attr & FBE_SSP_PATH_ATTR_CLOSED){ /* The edge is closed */
            /* We were unable to open ssp edge. */
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                       "sas protocol_edges_not_ready_cond: edge is closed \n");
            fbe_semaphore_destroy(&sem);

            /* The expectation is that if edge is closed, then PDO will get an event notification from PO when edge comes enabled.
               The event handler will reschedule this condition, which will make another attempt to connect.   However,  I've seen 
               cases where we don't get the event notification so the connection doesn't happen until the 3 second monitor cycle 
               executes again.  This causes us to stay  in Activate state too long.  We now automatically reschedule sooner. 
             */
            fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                         (fbe_base_object_t*)sas_physical_drive,  
                         200);
            return FBE_LIFECYCLE_STATUS_DONE;        
        }
    //}

    fbe_semaphore_destroy(&sem);

    /* Edge is now open. */
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "sas protocol_edges_not_ready_cond: can't clear current condition, status: 0x%X", status);
    }

    /* re-initialize TUR count in case we lost edge due to powerglitch. */
    sas_physical_drive->sas_drive_info.test_unit_ready_retry_count = 0;

    fbe_transport_complete_packet(packet);
    return FBE_LIFECYCLE_STATUS_PENDING;
}


static fbe_status_t 
sas_physical_drive_protocol_edges_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    /* Simply release the sempahore the caller is waiting on. */
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!**************************************************************
 * @fn sas_physical_drive_fw_download_cond_function
 ****************************************************************
 * @brief
 *  This condition function handles sending the Write Buffer
 *  cmd on the active side.  This also does the error handling if
 *  the Write Buffer fails.   After this condition completes it's
 *  expected that the remaining Activate conditions will be executed
 *  which will refresh the Inquiry info.
 * 
 * @param p_object - PDO object ptr
 * @param packet - monitor packet ptr
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_physical_drive_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * monitor_packet)
{
    fbe_status_t                            status;
    fbe_sas_physical_drive_t *              sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_base_physical_drive_t *             base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_packet_t *                          usurper_packet = NULL;
    fbe_payload_control_operation_opcode_t  control_code = 0;
    fbe_bool_t                              is_found = FBE_FALSE;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t                               i = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_u32_t                               number_of_clients = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);


    /* debug hook */
    fbe_base_object_check_hook(base_object,  
                               SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                               FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY, 
                               0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        /* pause this condition by preventing it form completing. */
        return FBE_LIFECYCLE_STATUS_DONE; 
    }
    
    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);

        if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD)
        {
            /* Process FW_DOWNLOAD usurper command */
            is_found = FBE_TRUE;

            payload = fbe_transport_get_payload_ex(usurper_packet);
            control_operation = fbe_payload_ex_get_control_operation(payload);
            fbe_payload_control_get_buffer(control_operation, &download_info);

            if (download_info->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
            {
                fbe_transport_remove_packet_from_queue(usurper_packet);
                fbe_base_object_usurper_queue_unlock(base_object);

                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                           "%s FW download succeeded.\n", __FUNCTION__);

                /* If we are logically offline, then we need a powercycle to refresh the peer side.   In the EMC Logical
                   Spec, a drive can have up to 60 seconds to perform any background tasks after a Write Buffer.  We
                   should wait at least this long before powercycling, otherwise there may be a chance drive gets
                   reverted to old fw.  The following code will keep us in this condition until the powercycle is issued. */
                fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server, &number_of_clients);
                if (number_of_clients == 0) /* We are Logically Offline */
                {
                    if ( ! fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_POWERCYCLE_DELAY))  /*first time?*/
                    {
                        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                                   "%s Logically Offline. Issuing Powercycle in %d msec\n", 
                                                                   __FUNCTION__, FBE_PDO_DOWNLOAD_POWERCYCLE_DELAY_MS);

                        fbe_base_physical_drive_lock(base_physical_drive);
                        fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD);
                        fbe_base_physical_drive_set_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_POWERCYCLE_DELAY);
                        fbe_base_physical_drive_unlock(base_physical_drive);

                        download_info->powercycle_expiration_time = fbe_get_time() + FBE_PDO_DOWNLOAD_POWERCYCLE_DELAY_MS;

                        /* put packet back on queue until timer expires*/                       
                        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sas_physical_drive, usurper_packet);                        

                        return FBE_LIFECYCLE_STATUS_DONE;
                    }
                    else /* FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_POWERCYCLE_DELAY */
                    {
                        if (fbe_get_time() < download_info->powercycle_expiration_time)
                        {
                            /* Still waiting for timer to expire.  Check again next monitor cycle. */                    
                            fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sas_physical_drive, usurper_packet);                        

                            return FBE_LIFECYCLE_STATUS_DONE;
                        }                        
                        else 
                        {
                            /* Time to initiate powercycle and then fall through to clearing this condition */
                            fbe_base_physical_drive_set_powercycle_duration(base_physical_drive, FBE_PDO_DOWNLOAD_POWERCYCLE_DURATION);
                        
                            status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                                            (fbe_base_object_t*)base_physical_drive, 
                                                            FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE);   
                            if (status != FBE_STATUS_OK) 
                            {
                                fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                                                                           "%s can't set POWER_CYCLE condition, status: 0x%X\n", __FUNCTION__, status);
                            }                                                     
                        }
                    }                    
                }

                /* Clear the current condition. */
                status = fbe_lifecycle_clear_current_cond(base_object);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                               "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
                }

                /* clear download state */
                fbe_base_physical_drive_lock(base_physical_drive);
                fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_MASK);
                fbe_base_physical_drive_unlock(base_physical_drive);

                /* complete packet */
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);

                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else if (download_info->retry_count >= FBE_SAS_PHYSICAL_DRIVE_MAX_FW_DOWNLOAD_RETRIES)
            {
                fbe_transport_remove_packet_from_queue(usurper_packet);
                fbe_base_object_usurper_queue_unlock(base_object);

                fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                                                           "%s FW download failed: exceed retry limit.\n", __FUNCTION__);

                /* Clear the current condition. */
                status = fbe_lifecycle_clear_current_cond(base_object);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                               "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
                }


                /* clear download state */
                fbe_base_physical_drive_lock(base_physical_drive);
                fbe_base_physical_drive_clear_local_state(base_physical_drive,
                                                          FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_MASK);
                fbe_base_physical_drive_unlock(base_physical_drive);

                /* complete packet */
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);

                if (fbe_dcs_param_is_enabled(FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES))
                {
                    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_physical_drive, 
                                                         FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED);

                    /* We need to set the BO:FAIL lifecycle condition */
                    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                    FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                                                                   "%s can't set FAIL condition, status: 0x%X\n", __FUNCTION__, status);
                    }
                }

                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else  /* retry */
            {
                fbe_transport_remove_packet_from_queue(usurper_packet);
                fbe_base_object_usurper_queue_unlock(base_object);

                status = sas_physical_drive_fw_download(sas_physical_drive, monitor_packet, usurper_packet);

                if (status == FBE_STATUS_OK)
                {
                    return FBE_LIFECYCLE_STATUS_PENDING;
                }
                else
                {
                    /* clear download state */
                    fbe_base_physical_drive_lock(base_physical_drive);
                    fbe_base_physical_drive_clear_local_state(base_physical_drive,
                                                              FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_MASK);
                    fbe_base_physical_drive_unlock(base_physical_drive);

                    /* complete packet */
                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(usurper_packet);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
            }
        }
        else
        {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        i++;
        if(i > 0x00ffffff){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped usurper queue\n", __FUNCTION__);
            fbe_base_object_usurper_queue_unlock(base_object);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    fbe_base_object_usurper_queue_unlock(base_object);

    /* Note: The design of drive firmware download functionality dictates this condition function is the only one
     * that should complete usurper packets related to any of the download conditions.  We're also intentionally
     * not clearing this condition if we don't find a packet on the usurper queue, because that would indicate a
     * violation of this design. Instead, log the following trace error message to indicate that situation occurred.
     */
    if (!is_found)
    {
        fbe_base_physical_drive_lock(base_physical_drive);
        if (!fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD))
        {
            /* If download state not set and pkt not found, then it must have been aborted.  So clear this condition. */
            fbe_lifecycle_clear_current_cond(base_object);
        }
        else
        {
            /* Else we are still in download state.  pkt might have temporarily been moved off of usurper queue.  
               Currently this is not expected, so logging this as an error */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                       "%s, Didn't find expected packet on usurper queue.\n", __FUNCTION__);
        }
        fbe_base_physical_drive_unlock(base_physical_drive);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!**************************************************************
 * @fn sas_physical_drive_fw_download_cond_peer_function
 ****************************************************************
 * @brief
 *  This condition function holds the peer (non-active) side in
 *  the Activate state until the download completes.  When this
 *  condition is cleared it will go through remaining Activate
 *  conditions in order to refresh its Inquiry info.
 * 
 * @param p_object - PDO object ptr
 * @param packet - monitor packet ptr
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 * @author
 *  12/03/2013  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t sas_physical_drive_fw_download_cond_peer_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)base_object, FBE_TRACE_LEVEL_INFO,
                                               "%s waiting for download to complete \n", __FUNCTION__);

    /* nothing to do but hang out in Activate state until DOWNLOAD_STOP clears this condition 
    */
    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!**************************************************************
 * @fn sas_physical_drive_abort_fw_download_cond_function
 ****************************************************************
 * @brief
 *  This condition function aborts an in-progress download
 *  request.   
 * 
 * @param p_object - PDO object ptr
 * @param packet - monitor packet ptr
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 * @author
 *  02/08/2013  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_physical_drive_abort_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_physical_drive_t *              sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;    
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  control_code;        
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_packet_t *                          usurper_packet;
    fbe_u32_t                               i = 0;
    fbe_bool_t                              is_found = FBE_FALSE;
    fbe_status_t                            status;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Clear edge attr now. */
    fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&((fbe_base_physical_drive_t*)sas_physical_drive)->block_transport_server, 0,      
                                                                         FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);

    /* Search the download packet in usurper queue */
    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        payload = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_payload_control_get_opcode(control_operation, &control_code);
        if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD)
        {
            queue_element = fbe_queue_next(queue_head, queue_element);

            fbe_transport_remove_packet_from_queue(usurper_packet);            

            fbe_base_object_usurper_queue_unlock(base_object);            

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                    "%s packet %p cancelled\n", __FUNCTION__, usurper_packet);            

            fbe_transport_set_status(usurper_packet, FBE_STATUS_CANCELED, 0);
            fbe_transport_complete_packet(usurper_packet);

            /* clear download state */
            fbe_base_physical_drive_lock((fbe_base_physical_drive_t*)sas_physical_drive);
            fbe_base_physical_drive_clear_local_state((fbe_base_physical_drive_t*)sas_physical_drive,
                                                      FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_MASK);
            fbe_base_physical_drive_unlock((fbe_base_physical_drive_t*)sas_physical_drive);

            is_found = FBE_TRUE;

            fbe_base_object_usurper_queue_lock(base_object);              
            break;
        }
        else
        {
            /* We already started processing the download request to physical packet. return busy. */
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        i++;
        if(i > 0x00ffffff){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped usurper queue\n", __FUNCTION__);
            break;
        }
    }
    fbe_base_object_usurper_queue_unlock(base_object);

    /* If dl pkt not found, then either it's being sent to drive, via write buffer, or was just rejected by client.
       If pkt is currently being sent to drive then we will let it finish instead of aborting, which may include
       retries. So nothing to do if pkt not found. */
    if (!is_found) 
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                   "%s download pkt already removed. \n", __FUNCTION__);
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t 
sas_physical_drive_fw_download(fbe_sas_physical_drive_t * sas_physical_drive, 
                                 fbe_packet_t * monitor_packet, 
                                 fbe_packet_t * usurper_packet)
{
    fbe_status_t status;

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(monitor_packet, usurper_packet);

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(monitor_packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sas_physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sas_physical_drive, monitor_packet);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(usurper_packet, sas_physical_drive_fw_download_completion, sas_physical_drive);

    if (status == FBE_STATUS_OK)
    {
        /* Call the executer function to do actual job */
        status = fbe_sas_physical_drive_download_device(sas_physical_drive, usurper_packet);
    }

    return status;
}

static fbe_status_t 
sas_physical_drive_fw_download_completion(fbe_packet_t * usurper_packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_sas_physical_drive_t *              sas_physical_drive = NULL;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_packet_t *                          monitor_packet = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry\n", __FUNCTION__);

    /* Check packet status */
    status = fbe_transport_get_status_code(usurper_packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK;
    }

    /* Get control status */
    payload = fbe_transport_get_payload_ex(usurper_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_buffer(control_operation, &download_info);

    if (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Enqueue usurper packet again so that it can be completed */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sas_physical_drive, usurper_packet);
        fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                 (fbe_base_object_t*)sas_physical_drive,  
                                 0);
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) &&
             (download_info->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE))
    {
        download_info->retry_count++;
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_INFO,
                                    "%s retry_count: %d\n",
                                    __FUNCTION__, download_info->retry_count);
        /* Enqueue usurper packet again for the retry */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sas_physical_drive, usurper_packet);
        fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                 (fbe_base_object_t*)sas_physical_drive,  
                                 0);
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid control status: 0x%X\n",
                                __FUNCTION__, control_status);
    }

    /* Get master packet and remove it from the terminator queue */
    monitor_packet = (fbe_packet_t *)fbe_transport_get_master_packet(usurper_packet);
    fbe_transport_remove_subpacket(usurper_packet);
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_physical_drive, monitor_packet);

    /* Wake up the monitor */
    fbe_transport_set_status(monitor_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(monitor_packet);

    /* Clear GRANT bit on block edge. */
    fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&((fbe_base_physical_drive_t *)sas_physical_drive)->block_transport_server,
                                                                         0,     
                                                                         FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);

    return status;
}

static fbe_lifecycle_status_t sas_physical_drive_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_physical_drive_t *              sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_packet_t *                          usurper_packet = NULL;
    fbe_payload_control_operation_opcode_t  control_code = 0;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t                               i = 0;
    fbe_payload_ex_t  *                        payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_u32_t                               powersave_attr = 0;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);
        payload  = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
        if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE)
        {
            /* Process FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE usurper command */
            if (powersave_attr & FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON)
            {
                /*edge attribute is power_save_on*/
                fbe_base_object_usurper_queue_unlock(base_object);
                fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else if (payload->physical_drive_transaction.retry_count >= FBE_SPINDOWN_MAX_RETRY_COUNT)
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                           FBE_TRACE_LEVEL_INFO,
                                                           "%s Spin down failed: exceed retry limit.\n",
                                                           __FUNCTION__);
                fbe_base_object_usurper_queue_unlock(base_object);
                fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);

                /* Set the BO:FAIL lifecycle condition */
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, 
                                                     FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
                }
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            payload->physical_drive_transaction.retry_count++;
            fbe_base_object_usurper_queue_unlock(base_object);
            status = sas_physical_drive_enter_power_save(sas_physical_drive, packet, FALSE);
            return FBE_LIFECYCLE_STATUS_PENDING;
           
        }
        else if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE_PASSIVE)
        {
            /* Process FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE_PASSIVE usurper command */
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(usurper_packet);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE)
        {
            /* sas_physical_drive->sas_drive_info->power_save_mode = FALSE; */
            powersave_attr &= ~FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
            fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);

            /* Process FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE usurper command */
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);

            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
            }   

            /*and jumpt to activate*/
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sas_physical_drive, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(usurper_packet);
            fbe_base_object_usurper_queue_lock(base_object);
            queue_element = fbe_queue_front(queue_head);
        }
        else
        {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        
        i++;
        if(i > 0x00ffffff){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped usurper queue\n", __FUNCTION__);
            fbe_base_object_usurper_queue_unlock(base_object);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    fbe_base_object_usurper_queue_unlock(base_object);

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t sas_physical_drive_enter_power_save(fbe_sas_physical_drive_t * sas_physical_drive, 
                                                        fbe_packet_t * packet,
                                                        fbe_bool_t passive)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
  
    FBE_UNREFERENCED_PARAMETER(passive);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

#if 0
    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sas_physical_drive, &path_attr);
    if (status == FBE_STATUS_OK && (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED))
    {
        /* Send a command to enclosure object via discovery edge. */
        status = fbe_transport_set_completion_function(packet, sas_physical_drive_enter_power_save_completion, sas_physical_drive);
        fbe_sas_physical_drive_set_power_save(sas_physical_drive, packet, TRUE, passive);
        return FBE_STATUS_OK;
    }
#endif

    /* Send a command to port object via SSP edge. */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_enter_power_save_completion, sas_physical_drive);
    fbe_sas_physical_drive_spin_down(sas_physical_drive, packet, FALSE);

    return FBE_STATUS_OK;
}

static fbe_status_t sas_physical_drive_enter_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t *  sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    fbe_u32_t powersave_attr = 0;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK;
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK ||
       control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NOT_SPINNING)
    {
        /* Set powersave_on bit */
        fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
        powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
        fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);
        status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sas_physical_drive,  
                                        0);

    }
#if 0
    else if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) &&
             (control_status_qualifier == FBE_SAS_DRIVE_STATUS_HARD_ERROR))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s Spin down hard error\n",
                                __FUNCTION__);

        /* Set the BO:FAIL lifecycle condition */
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, 
            FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN);

        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
#endif
    else if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) &&
             (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE))
    {
        status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                          (fbe_base_object_t*)sas_physical_drive,  
                                          0);

    }

    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid control status: 0x%X\n",
                                __FUNCTION__, control_status);
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t sas_physical_drive_power_save_active_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_physical_drive_t *  sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_u32_t                   powersave_attr = 0;
    fbe_status_t                status;

    fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
    if (powersave_attr & FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "Saving power(Active)\n");

        /*edge attribute is power_save_on*/
        status  = fbe_lifecycle_clear_current_cond(base_object);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                          FBE_TRACE_LEVEL_ERROR,
                          "%s can't clear condition, status: 0x%X\n",
                          __FUNCTION__, status);
        }

        sas_physical_drive->sas_drive_info.spindown_retry_count = 0;/*reset for next time*/
        return FBE_LIFECYCLE_STATUS_DONE;

    }else if (sas_physical_drive->sas_drive_info.spindown_retry_count > FBE_SPINDOWN_MAX_RETRY_COUNT){
        /*we tried a few times, let's just kill the drive*/
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                       FBE_TRACE_LEVEL_INFO,
                                                       "%s Spin down failed: exceed retry limit.\n",
                                                       __FUNCTION__);
        
        /* Set the BO:FAIL lifecycle condition */
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, 
                                             FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN);

        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                          FBE_TRACE_LEVEL_ERROR,
                          "%s can't set FAIL condition, status: 0x%X\n",
                          __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;

    }

    sas_physical_drive->sas_drive_info.spindown_retry_count++;
    sas_physical_drive_enter_power_save(sas_physical_drive, packet, FBE_FALSE);
    
    return FBE_LIFECYCLE_STATUS_PENDING;


}

static fbe_lifecycle_status_t sas_physical_drive_power_save_passive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_physical_drive_t *              sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_u32_t                               powersave_attr = 0;

    /*nothing much to do here beise mark that we save power*/
    fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
    powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
    fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);
    
         fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "Saving power(passive)\n");

        fbe_lifecycle_clear_current_cond(base_object);
  
    
        return FBE_LIFECYCLE_STATUS_DONE;
    }

static fbe_lifecycle_status_t sas_physical_drive_exit_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_physical_drive_t *              sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_u32_t                               powersave_attr = 0;

    /*flip the attribute*/
    fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, &powersave_attr);
    powersave_attr &= ~FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
    fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sas_physical_drive, powersave_attr);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                        FBE_TRACE_LEVEL_ERROR,
                        "%s can't clear current condition, status: 0x%X\n",
                        __FUNCTION__, status);
    }   

    /*and jumpt to activate*/
    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sas_physical_drive, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
  
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_physical_drive_disk_collect_flush_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_atomic_t dc_lock = FBE_FALSE;
    
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);
    
        
    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }

        /* Clear the DC memory. */
        fbe_zero_memory(&base_physical_drive->dc, sizeof(fbe_base_physical_drive_dc_data_t));       

        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Skip this record if drive fail and doesn't get the disk capacity. */
    if (base_physical_drive->block_size == 0) 
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
      
        /* Clear the DC memory. */
        fbe_zero_memory(&base_physical_drive->dc, sizeof(fbe_base_physical_drive_dc_data_t));       
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }
        
    /* Grap the lock. */
    dc_lock = fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_TRUE);
    if (dc_lock == FBE_TRUE) {
        /* Don't get the lock. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 "%s DC is locked. lba:0x%llx Record cnt:%d.\n",
                                  __FUNCTION__, (unsigned long long)base_physical_drive->dc_lba, base_physical_drive->dc.number_record);
                                  
         return FBE_LIFECYCLE_STATUS_DONE;
    }
   
    /* If the drive is inserted, fill the header information to the dc structure. */
    if (base_physical_drive->dc_lba == 0) {
        base_physical_drive_fill_dc_header(base_physical_drive);
    }
    
  
#if 0 /* Ashok - Disable until we figure out what to do for 4K *///!defined(UMODE_ENV)  && !defined(SIMMODE_ENV)

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_disk_collect_flush_cond_completion, sas_physical_drive);

    /* Call the executor function to do actual job */
    status = sas_physical_drive_disk_collect_write(sas_physical_drive, packet);
    return FBE_LIFECYCLE_STATUS_PENDING;
#else

    /* Release the lock. */
    fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
#endif
}

static fbe_status_t 
sas_physical_drive_disk_collect_flush_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    base_physical_drive = &sas_physical_drive->base_physical_drive;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry lba:0x%llx RetryCnt:%d RecCnt:%d\n", 
                          __FUNCTION__, (unsigned long long)base_physical_drive->dc_lba, base_physical_drive->dc.retry_count,base_physical_drive->dc.number_record);
                          
    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
                                
        /* Release the lock. */
        fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);
    /* fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_INFO,
                                    "Disk_collect_cond_completion, Control status:0x%X Qualifier:0x%X lba:0x%llx Retry cnt:%d Record cnt: %d \n",
                                    control_status, control_status_qualifier, base_physical_drive->dc_lba, 
                                    base_physical_drive->dc.retry_count,base_physical_drive->dc.number_record);
    */                              
    if( (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (base_physical_drive->dc.retry_count >= FBE_DC_MAX_WRITE_ERROR_RETRIES))
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SET_PROACTIVE_SPARE)
        {
            /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
            fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_WARNING,
                                      "%s Set Proactive Spare for Disk Collect. \n", __FUNCTION__);         
        }
           
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
      
        /* Move the pointer ro the next sector, if over 2046, then reset it. */
        base_physical_drive->dc_lba++;
        if (base_physical_drive->dc_lba > 2047) {
            base_physical_drive->dc_lba = 1;
        }
        /* Clear the DC memory. */
        fbe_zero_memory(&base_physical_drive->dc, sizeof(fbe_base_physical_drive_dc_data_t));        
    }
    else 
    {
        base_physical_drive->dc.retry_count++;
        
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_REMAP)
        {
            base_pdo_payload_transaction_flag_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_DC_REMAP);
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_WARNING,
                                          "%s Setup remap for Disk Collect, Retry count: %d. \n", __FUNCTION__, base_physical_drive->dc.retry_count);

            /* Release the lock. */
            fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
            sas_physical_drive_setup_dc_remap(sas_physical_drive, packet);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RETRY)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                  FBE_TRACE_LEVEL_WARNING,
                                  "%s Error Retry for Disk Collect, Retry count: %d. \n", __FUNCTION__, base_physical_drive->dc.retry_count);
        
        }
        else
        {
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, 
                        FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED);

            /* We need to set the BO:FAIL lifecycle condition */
            status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)base_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                  FBE_TRACE_LEVEL_WARNING,
                                  "%s Hard Error for Disk Collect, Retry count: %d. \n", __FUNCTION__, base_physical_drive->dc.retry_count);
                                  
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_WARNING,
                                  "%s can't set FAILED condition for others, status: 0x%X\n",
                                  __FUNCTION__, status);
            }
        }
    }

    /* Release the lock. */
    fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
    
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_dump_disk_log_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,    
                                               FBE_TRACE_LEVEL_INFO,
                                               "%s entry\n", __FUNCTION__);
    
    // Check path state. If the enclosure is failed or gone, we should fail/destroy drive object too
    fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sas_physical_drive, &path_state);
    if(path_state == FBE_PATH_STATE_GONE)
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s path state gone", __FUNCTION__);
        
        // set lifecycle condition to destroy - discovery edge gone
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
        
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't set DESTROY condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }                
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_dump_disk_log_cond_completion, sas_physical_drive);

    /* Call the executor function to do dump the disk log. */
    status = sas_physical_drive_dump_disk_log(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_dump_disk_log_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
   
    sas_physical_drive = (fbe_sas_physical_drive_t*)context;
  
    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sas_physical_drive_discovery_poll_per_day_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Return if the receive diagoistic page for disk collect is not on. */                    
    if (sas_physical_drive->sas_drive_info.dc_rcv_diag_enabled != 1)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }   
    
    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);
                                
         fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DC_RCV_DIAG_ENABLED, 
                                        NULL, 
                                        0);  
                                        
    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_discovery_poll_per_day_cond_completion, sas_physical_drive);
                                    
    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);
 
    /* Send to executer for receive diagonstic page code 0x82. */
    status = fbe_sas_physical_drive_send_RDR(sas_physical_drive, packet, RECEIVE_DIAG_PG82_CODE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_discovery_poll_per_day_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        /* We can't get the inquiry string, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
                                
        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK; 
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    
    fbe_payload_ex_release_control_operation(payload, control_operation);   
    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t
sas_physical_drive_monitor_discovery_edge_in_fail_state_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;
    
    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    
    // Check path state. If the enclosure is failed or gone, we should fail/destroy drive object too
    fbe_base_discovered_get_path_state((fbe_base_discovered_t *) base_physical_drive, &path_state);
    if(path_state == FBE_PATH_STATE_GONE)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s path state gone", __FUNCTION__);
        
        // set lifecycle condition to destroy - discovery edge gone
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
        
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't set DESTROY condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}




static fbe_lifecycle_status_t 
sas_physical_drive_vpd_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);


    if (sas_pdo_is_in_sanitize_state(sas_physical_drive))
    {
        /* If we are in a sanitize state, then any cmd to the drive will return a check condition.  To
           prevent failing in the Activate rotary, we will skip this condition. Clients will be responsible
           for re-initalizing the PDO when in the maintenance mode.  Only conditions after TUR should make 
           this check, since maintenance bit is pre-set and cleared in TUR.
        */

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s In Sanitize state.  skipping condition\n", __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Allocate a buffer */
    drive_info = (fbe_physical_drive_information_t *)fbe_transport_allocate_buffer();
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);
        fbe_transport_release_buffer(drive_info);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION, 
                                        drive_info, 
                                        sizeof(fbe_physical_drive_information_t));  

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_physical_drive_vpd_inquiry_cond_completion, sas_physical_drive);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    /* Call the executer function to do actual job */
    status = fbe_sas_physical_drive_send_vpd_inquiry(sas_physical_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_physical_drive_vpd_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    fbe_payload_control_get_buffer(control_operation, &drive_info);
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK;
    }

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        /* We can't get the inquiry string, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_transport_release_buffer(drive_info);
        return FBE_STATUS_OK; 
    }

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }

        if(fbe_base_physical_drive_is_enhanced_queuing_supported((fbe_base_physical_drive_t*)sas_physical_drive))            
        {
            fbe_pdo_maintenance_mode_clear_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED);
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                       FBE_TRACE_LEVEL_DEBUG_LOW,
                                                       "Enhanced queuing not supported\n");

            if (fbe_dcs_param_is_enabled(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES))
            {   
                fbe_pdo_maintenance_mode_set_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                  FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED);
            }
            else /* not enforced*/
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                       FBE_TRACE_LEVEL_INFO,
                                                       "Enhanced queuing not supported.  EQ check is disabled.\n");

                /* must clear if not enforced */
                fbe_pdo_maintenance_mode_clear_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                    FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED);
            }            
        }          
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_buffer(drive_info);

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sas_physical_drive_health_check_cond_function(fbe_base_object_t * base_object, fbe_packet_t * monitor_packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_sas_physical_drive_t *             sas_physical_drive = (fbe_sas_physical_drive_t*)base_object;
    fbe_base_physical_drive_t *            base_physical_drive = (fbe_base_physical_drive_t *)base_object;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_packet_t *                         usurper_packet = NULL;
    fbe_payload_control_operation_opcode_t control_code = 0;
    fbe_queue_element_t *                  queue_element = NULL;
    fbe_queue_head_t    *                  queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t                              i = 0;
    fbe_payload_control_operation_t *      control_operation = NULL;
    fbe_payload_control_status_t           control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_drive_error_handling_bitmap_t      cmd_status = FBE_DRIVE_CMD_STATUS_INVALID; 
 
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);
    
    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while (queue_element)
    {
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);

        if (control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK)
        {
            /* Process HEALTH_CHECK usurper command */
            payload = fbe_transport_get_payload_ex (usurper_packet);
            control_operation = fbe_payload_ex_get_control_operation(payload);
            fbe_payload_control_get_status(control_operation, &control_status);
            fbe_payload_control_get_status_qualifier(control_operation, (fbe_payload_control_status_qualifier_t*)&cmd_status);
     
            /* check for special actions */
            if (cmd_status & FBE_DRIVE_CMD_STATUS_EOL)
            {
                fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
            }


            if (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK ||
                (cmd_status & FBE_DRIVE_CMD_STATUS_RESET) )  /* treat as success, since it will do reset as final step in HC */
            {
                /* Clear the current condition */
                status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)sas_physical_drive);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            "%s: Can't clear current condition, status: 0x%X\n",
                            __FUNCTION__, status);
                }
                
                /* Remove the usurper_packet from the usurper queue while it is locked to prevent threading issues.
                */
                fbe_transport_remove_packet_from_queue(usurper_packet);
                fbe_base_object_usurper_queue_unlock(base_object);                
 
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                        FBE_TRACE_LEVEL_INFO,
                        "PDO: Health Check SEND DIAGNOSTIC CDB completed with no SCSI Errors.\n");
            
                fbe_base_physical_drive_log_health_check_event(base_physical_drive, FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_COMPLETED);
            
                /* The health check succeeded so set the RESET condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET);
            
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                            "%s: Can't set RESET condition, status: 0x%x\n",
                            __FUNCTION__, status);
                }
             
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);
            
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else if ((cmd_status & FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE) &&
                     (payload->physical_drive_transaction.retry_count < FBE_HEALTH_CHECK_MAX_RETRY_COUNT)) 
            {                        
                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                           "send health check. retry=%d.\n", payload->physical_drive_transaction.retry_count);

                /* If this is the first attempt at sending the HC, then update the DIEH stats */
                if (payload->physical_drive_transaction.retry_count==0)
                {
                    sas_physical_drive_update_dieh_health_check_stats(sas_physical_drive);
                }

                payload->physical_drive_transaction.retry_count++;
                
                fbe_base_object_usurper_queue_unlock(base_object);
  
                status = sas_physical_drive_send_health_check(sas_physical_drive, monitor_packet);

                if (status == FBE_STATUS_OK)
                {
                    return FBE_LIFECYCLE_STATUS_PENDING;
                }
                else
                {
                    /* remove pkt from usurper Q and fail back to caller */

                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                        "PDO: Health Check failed call ..._send_health_check \n");

                    fbe_base_object_usurper_queue_lock(base_object);
                    fbe_transport_remove_packet_from_queue(usurper_packet);  
                    fbe_base_object_usurper_queue_unlock(base_object);

                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(usurper_packet);
                    return FBE_LIFECYCLE_STATUS_DONE;                       
                }
            }
            else

            {
                /*NOTE: If we got here because we exceeded our retry count, then the only time we should fail drive
                  is if HC was triggered by service timeout.   TODO: If HC was issued due to maintenance then let drive go back
                  to Ready.  */

                /* Only expect to fail drive if we hit our max retry count or status is FAIL_NON_RETRYABLE.  Log an error
                   if we get anything other than this. */
                if ( payload->physical_drive_transaction.retry_count < FBE_HEALTH_CHECK_MAX_RETRY_COUNT  &&                     
                     !(cmd_status & FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE)) 
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                            "Unexpected failure. Control status:%d & qualifier:%d \n", control_status, cmd_status);
                }

                /* Fail Health Check. */
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                        FBE_TRACE_LEVEL_WARNING,
                        "PDO: Health Check failed. Control status:%d & qualifier:%d \n",
                        control_status, cmd_status);
                
                /* Clear the current condition. */
                status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_physical_drive);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_ERROR,
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
                }
                
                /* Remove the usurper_packet from the usurper queue while it is locked to prevent threading issues.
                 * Since the usurper queue is already locked, directly call the transport layer to remove the packet.
                 */
                fbe_transport_remove_packet_from_queue(usurper_packet);
                fbe_base_object_usurper_queue_unlock(base_object);
                
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);

                /* No matter what SCSI Status we received for the failed SEND DIAGNOSTIC CDB,     */
                /* The health check failed so set the FAILED condition                            */
            
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED);
            
                fbe_base_physical_drive_log_health_check_event(base_physical_drive, FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_FAILED);
            
                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK)
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            "%s: Can't set FAIL condition, status: 0x%X\n",
                            __FUNCTION__, status);
                }
                
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }
        else
        {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }

        i++;
        if (i > 0x00ffffff)
        {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: Critical error looped usurper queue\n", __FUNCTION__);
            fbe_base_object_usurper_queue_unlock(base_object);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }   

    fbe_base_object_usurper_queue_unlock(base_object);

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!**************************************************************
 * @fn sas_physical_drive_update_dieh_health_check_stats
 ****************************************************************
 * @brief
 *  This function will update the DIEH HealthCheck bucket stats.
 *  These stats are used to monitor the number of times we enter
 *  HC.  If the frequency is too high then the drive will be shot.
 * 
 * @author
 *  09/30/2015  Wayne Garrett
 *
 ****************************************************************/
static void
sas_physical_drive_update_dieh_health_check_stats(fbe_sas_physical_drive_t * sas_physical_drive)
{
    fbe_base_physical_drive_t *  base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_drive_configuration_handle_t       drive_configuration_handle;
    fbe_drive_configuration_record_t  *    threshold_rec = NULL;
    fbe_status_t status;

    fbe_base_physical_drive_get_configuration_handle(base_physical_drive,  &drive_configuration_handle);

    if (drive_configuration_handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE)
    {
        status = fbe_drive_configuration_get_threshold_info_ptr(base_physical_drive->drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */                        
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                    "%s failed to get threshold rec.\n", __FUNCTION__);
            /* notify PDO to re-register. */
            fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                   (fbe_base_object_t*)base_physical_drive, 
                                   FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CONFIG_CHANGED);                
        }
    }

    /*  Note, IO did not fail as suggested by function name below. */
    fbe_base_physical_drive_io_fail(base_physical_drive, drive_configuration_handle, threshold_rec, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK, FBE_SCSI_SEND_DIAGNOSTIC, 
                                    FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); 

}

static fbe_status_t 
sas_physical_drive_send_health_check(fbe_sas_physical_drive_t * sas_physical_drive, 
                                     fbe_packet_t * monitor_packet)
{
    fbe_status_t status;

    /* We just push additional context on the packet stack */
    status = fbe_transport_set_completion_function(monitor_packet, sas_physical_drive_send_health_check_completion, sas_physical_drive);
    
    if (status == FBE_STATUS_OK)
    {
        /* Call the executer function that sends the CDB to the drive to trigger the health check  */
        status = fbe_sas_physical_drive_send_qst(sas_physical_drive, monitor_packet);
    }

    return status;
}
static fbe_status_t 
sas_physical_drive_send_health_check_completion(fbe_packet_t * monitor_packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_sas_physical_drive_t *                  sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    fbe_payload_control_operation_opcode_t      control_code;
    fbe_payload_control_status_t                control_status;
    fbe_payload_control_status_qualifier_t      control_status_qualifier;
    fbe_payload_ex_t *                          mntr_payload = NULL;
    fbe_payload_control_operation_t *           mntr_control_operation = NULL;
    fbe_packet_t *                              usrpr_packet = NULL;
    fbe_payload_ex_t *                          usrpr_payload = NULL;
    fbe_payload_control_operation_t *           usrpr_control_operation = NULL;
    fbe_queue_head_t *                          queue_head = fbe_base_object_get_usurper_queue_head((fbe_base_object_t*)sas_physical_drive);
    fbe_queue_element_t *                       queue_element;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s entry\n", __FUNCTION__);
    
    /* Check packet status */
    status = fbe_transport_get_status_code(monitor_packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                   "%s: Invalid packet, status:%d\n", __FUNCTION__, status);
        return FBE_STATUS_OK;
    }
    

    /* The processing of the returned control status is handled by the monitor HC condition. 
       Find the HC usurper pkt then update it with the status from the HC. 
    */
    fbe_base_object_usurper_queue_lock((fbe_base_object_t*)sas_physical_drive);
    queue_element = fbe_queue_front(queue_head);
    while (queue_element)
    {
        usrpr_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usrpr_packet);

        if (control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK)
        {
            /* get monitor status */
            mntr_payload = fbe_transport_get_payload_ex(monitor_packet);    
            mntr_control_operation = fbe_payload_ex_get_control_operation(mntr_payload);
            fbe_payload_control_get_status(mntr_control_operation, &control_status);
            fbe_payload_control_get_status_qualifier(mntr_control_operation, &control_status_qualifier);

            /* transfer to usurper pkt to allow HC condition to process it */
            usrpr_payload = fbe_transport_get_payload_ex(usrpr_packet);
            usrpr_control_operation = fbe_payload_ex_get_control_operation(usrpr_payload);
            fbe_payload_control_set_status(usrpr_control_operation, control_status);
            fbe_payload_control_set_status_qualifier(usrpr_control_operation, control_status_qualifier);                
        }
        queue_element = fbe_queue_next(queue_head, queue_element);
    }
    fbe_base_object_usurper_queue_unlock((fbe_base_object_t*)sas_physical_drive);
        
    /* reschedule monitor immediately */
    fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive,  
                             0);
    return FBE_STATUS_OK;  
}

/*!**************************************************************
 * @fn sas_physical_drive_initiate_health_check_cond_function
 ****************************************************************
 * @brief
 *  This condition function initiates the health check by first
 *  sending a request to PVD.  After PVD quiesces IO on both SPs
 *  it will ack request, at which point we do the actual health check.
 *  If PVD is not logically online then we immediately do the health
 *  check since there's no point in quiescing IO.  Also note, that
 *  the condition is intentially not cleared in order to keep PDO in
 *  activate state until we get the PVD ack.
 * 
 * @param p_object -   PDO object ptr
 * @param packet - monitor packet ptr
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 * @author
 *  05/21/2013  Wayne Garrett - Modified to only due HC when PVD is online.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_physical_drive_initiate_health_check_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{  
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)p_object;
    fbe_drive_configuration_handle_t drive_configuration_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    fbe_drive_configuration_record_t  *    threshold_rec = NULL;


    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry\n", __FUNCTION__);

    /* Only send request to PVD if it's online */
    if (FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE == base_physical_drive->logical_state)
    {    
        fbe_base_physical_drive_set_path_attr(base_physical_drive, 
                                              FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST);
    }
    else /* do HC now */
    {
        /* Following commented-out code has not been tested.  But this is basically what we need to do
           if we want to do health check without going through PVD. */
#if 0
        fbe_packet_t * packet;
        fbe_physical_drive_health_check_req_status_t proceed_status = FBE_DRIVE_HC_STATUS_PROCEED;

        /*TODO: make sure we are not already in Health Check.  Add another local_state to indicate HC_STARTED.  Don't forget to
          clear local_state HC mask when complete or aborted. */
        packet = fbe_transport_allocate_packet();
        if (packet == NULL) 
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR, 
                                                       "initiate_health_check_cond: fbe_transport_allocate_packet fail\n");

            fbe_lifecycle_clear_current_cond(p_object);

            /* do cleanup */
            fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                                   FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE);

            return FBE_LIFECYCLE_STATUS_DONE;
        }

        fbe_transport_initialize_packet(packet);

        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

        if (payload_p == NULL)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR, 
                                                       "initiate_health_check_cond: fbe_payload_ex_allocate_control_operation fail\n");

            fbe_lifecycle_clear_current_cond(p_object);

            /* do cleanup */
            fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                                   FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE);

            fbe_transport_release_packet(packet_p);

            return FBE_LIFECYCLE_STATUS_DONE;
        }
    
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_payload_control_set_status_qualifier(control_operation_p, 0);

        fbe_payload_control_build_operation (control_operation_p,
                                             FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK,
                                             &proceed_status,
                                             sizeof(fbe_physical_drive_health_check_req_status_t));

        status = fbe_transport_set_completion_function(packet, sas_physical_drive_initiate_health_check_no_pvd_cond_completion, sas_physical_drive);

#else

        /* TODO:  Due to the complicated nature of Health Check being hardwired to PVD, we cannot easily
           force it from PDO with changing a lot of the design.  At this time, late in Rockies program, this
           is too high risk.   What this means is that we don't support health check during SP boot, prior
           to SEP running, or cases where PVD is logically offline.  05/21/2013 -wayne */
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "initiate_health_check_cond: aborting. pvd not online. logical state:%d\n", 
                                                   base_physical_drive->logical_state);        

        /* Inform DIEH in case we go into HC too frequently.  Note, IO didn't fail, just reusing an existing function to count HCs. */
        fbe_base_physical_drive_get_configuration_handle(base_physical_drive,  &drive_configuration_handle);
        fbe_drive_configuration_get_threshold_info_ptr(base_physical_drive->drive_configuration_handle, &threshold_rec);

        fbe_base_physical_drive_io_fail(base_physical_drive, drive_configuration_handle, threshold_rec, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK, FBE_SCSI_SEND_DIAGNOSTIC, 
                                        FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); 

        /* do cleanup */
        fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                               FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE);

        fbe_lifecycle_clear_current_cond(p_object);
#endif

    }


    fbe_base_physical_drive_log_health_check_event(base_physical_drive, FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_INITIATED);
    
    /* Note, If PVD takes too long then condition will fire again.  However PVD handles getting multiple requests, 
    so it's not an issue. */

    /* TODO: we should have a timer condition that will fire if we wait for PVD too long, otherwise we fall back to default
    activate timer of 170 seconds.*/

    return FBE_LIFECYCLE_STATUS_DONE;   
}

/*!**************************************************************
 * @fn sas_physical_drive_initiate_health_check_no_pvd_cond_completion
 ****************************************************************
 * @brief
 *  This completion function for case where pvd is not online.  This
 *  will cleanup any resources allocated.
 * 
 * @param packet - packet for controlling health check
 * @param context - ptr to PDO object;
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 * @author
 *  05/21/2013  Wayne Garrett - created.
 *
 ****************************************************************/
static fbe_status_t           
sas_physical_drive_initiate_health_check_no_pvd_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t *          sas_physical_drive = (fbe_sas_physical_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code(packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (FBE_STATUS_OK != pkt_status ||
        FBE_PAYLOAD_CONTROL_STATUS_OK != control_status)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING, 
                                                   "initiate_health_check_no_pvd_cond_completion: failed. pkt:%d cntrl:%d\n",
                                                   pkt_status, control_status);            
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO, 
                                                   "initiate_health_check_no_pvd_cond_completion: success\n");
    }

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    fbe_transport_release_packet(packet_p);
 
    /* Note, no condition to clear.  By design it is cleared by usurper function for PVD ack, which is called directly by PDO
       in this case (pvd not online).  See condition function for where it is called. */
         
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sas_physical_drive_health_check_complete_cond_function
 ****************************************************************
 * @brief
 *  This function cleans up after healh check completes.
 * 
 * @param p_object -   PDO object ptr
 * @param packet - monitor packet ptr
 *
 * @return fbe_lifecycle_status_t - status of condition
 * 
 * @author
 *  11/26/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_physical_drive_health_check_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)p_object;
    fbe_status_t status;

    fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_physical_drive);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                   "%s: Can't clear current condition, status: 0x%X\n",
                                                   __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
