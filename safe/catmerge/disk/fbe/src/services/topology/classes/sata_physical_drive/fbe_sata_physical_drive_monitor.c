/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/

#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sata_physical_drive_private.h"

fbe_status_t 
fbe_sata_physical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;

    sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry.\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sata_physical_drive_lifecycle_const, (fbe_base_object_t*)sata_physical_drive, packet);

    return status;
}

fbe_status_t fbe_sata_physical_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sata_physical_drive));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sata_physical_drive_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sata_physical_drive_attach_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_attach_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_open_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_open_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sata_physical_drive_protocol_edge_detached_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_protocol_edge_detached_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_protocol_edge_closed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_protocol_edge_closed_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_identify_device_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_identify_device_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_read_native_max_address_ext_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_read_native_max_address_ext_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_read_test_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_read_test_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_check_power_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_check_power_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_execute_device_diag_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_execute_device_diag_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_write_inscription_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_write_inscription_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_read_inscription_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_read_inscription_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_flush_write_cache_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_flush_write_cache_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_disable_write_cache_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_disable_write_cache_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_enable_rla_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_enable_rla_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_set_pio_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_set_pio_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_set_udma_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_set_udma_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_enable_smart_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_enable_smart_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_enable_smart_autosave_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_enable_smart_autosave_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_smart_return_status_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_smart_return_status_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_smart_read_attributes_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_smart_read_attributes_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_sct_set_read_timer_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_sct_set_read_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_sct_set_write_timer_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_sct_set_write_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_detach_stp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_detach_stp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_reset_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_reset_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_protocol_edges_not_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t sata_physical_drive_protocol_edges_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_fw_download(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet, fbe_packet_t * u_packet);
static fbe_status_t sata_physical_drive_fw_download_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_enter_power_save(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_enter_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_retry_get_dev_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_retry_get_dev_info_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_get_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_get_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sata_physical_drive_release_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_release_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t sata_physical_drive_power_save_active_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sata_physical_drive_power_save_passive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sata_physical_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sata_physical_drive);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sata_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sata_physical_drive,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sata_physical_drive_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sata_physical_drive_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_protocol_edges_not_ready_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY,
        sata_physical_drive_protocol_edges_not_ready_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_attach_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE,
        sata_physical_drive_attach_protocol_edge_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_open_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE,
        sata_physical_drive_open_protocol_edge_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        sata_physical_drive_discovery_poll_cond_function)
};

/* Temporarily we will NOT instantiate base_logical drive_object */
/*
static fbe_lifecycle_const_cond_t sata_physical_drive_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        FBE_LIFECYCLE_NULL_FUNC)
};
*/

static fbe_lifecycle_const_cond_t sata_physical_drive_reset_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET,
        sata_physical_drive_reset_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_fw_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD,
        sata_physical_drive_fw_download_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE,
        sata_physical_drive_power_save_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_active_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE,
        sata_physical_drive_power_save_active_cond_function)
};

static fbe_lifecycle_const_cond_t sata_physical_drive_passive_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE,
        sata_physical_drive_power_save_passive_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/
static fbe_lifecycle_const_base_cond_t sata_physical_drive_protocol_edge_detached_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_protocol_edge_detached_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_DETACHED,
        sata_physical_drive_protocol_edge_detached_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_protocol_edge_closed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_protocol_edge_closed_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_CLOSED,
        sata_physical_drive_protocol_edge_closed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_identify_device_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_identify_device_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_IDENTIFY_DEVICE,
        sata_physical_drive_identify_device_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_write_inscription_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_write_inscription_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_WRITE_INSCRIPTION,
        sata_physical_drive_write_inscription_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_read_inscription_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_read_inscription_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_INSCRIPTION,
        sata_physical_drive_read_inscription_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_check_power_mode_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_check_power_mode_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_POWER_MODE,
        sata_physical_drive_check_power_mode_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_execute_device_diag_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_execute_device_diag_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_EXECUTE_DEVICE_DIAG,
        sata_physical_drive_execute_device_diag_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_read_native_max_address_ext_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_read_native_max_address_ext_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_NATIVE_MAX_ADDRESS_EXT,
        sata_physical_drive_read_native_max_address_ext_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_flush_write_cache_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_flush_write_cache_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_FLUSH_WRITE_CACHE,
        sata_physical_drive_flush_write_cache_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_disable_write_cache_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_disable_write_cache_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_DISABLE_WRITE_CACHE,
        sata_physical_drive_disable_write_cache_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_enable_rla_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_enable_rla_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_ENABLE_RLA,
        sata_physical_drive_enable_rla_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_set_pio_mode_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_set_pio_mode_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_PIO_MODE,
        sata_physical_drive_set_pio_mode_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_set_udma_mode_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_set_udma_mode_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_UDMA_MODE,
        sata_physical_drive_set_udma_mode_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_enable_smart_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_enable_smart_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_ENABLE_SMART,
        sata_physical_drive_enable_smart_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_enable_smart_autosave_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_enable_smart_autosave_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_ENABLE_SMART_AUTOSAVE,
        sata_physical_drive_enable_smart_autosave_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_smart_return_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_smart_return_status_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SMART_RETURN_STATUS,
        sata_physical_drive_smart_return_status_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_smart_read_attributes_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_smart_read_attributes_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SMART_READ_ATTRIBUTES,
        sata_physical_drive_smart_read_attributes_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_sct_set_read_timer_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_sct_set_read_timer_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SCT_SET_READ_TIMER,
        sata_physical_drive_sct_set_read_timer_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_sct_set_write_timer_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_sct_set_write_timer_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SCT_SET_WRITE_TIMER,
        sata_physical_drive_sct_set_write_timer_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_read_test_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_read_test_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_TEST,
        sata_physical_drive_read_test_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t sata_physical_drive_detach_stp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_detach_stp_edge_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_DETACH_STP_EDGE,
        sata_physical_drive_detach_stp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_retry_get_dev_info_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_retry_get_dev_info_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_RETRY_GET_DEV_INFO,
        sata_physical_drive_retry_get_dev_info_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_get_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_get_permission_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT,
        sata_physical_drive_get_permission_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sata_physical_drive_release_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sata_physical_drive_release_permission_cond",
        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT,
        sata_physical_drive_release_permission_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t sata_physical_drive_release_permission_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "sata_physical_drive_release_permission_timer_cond",
            FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT_TIMER,
            sata_physical_drive_release_permission_cond_function),
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

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sata_physical_drive)[] = {
    &sata_physical_drive_protocol_edge_detached_cond,
    &sata_physical_drive_protocol_edge_closed_cond,
    &sata_physical_drive_identify_device_cond,
    &sata_physical_drive_write_inscription_cond,
    &sata_physical_drive_read_inscription_cond,
    &sata_physical_drive_check_power_mode_cond,
    &sata_physical_drive_execute_device_diag_cond,
    &sata_physical_drive_read_native_max_address_ext_cond,
    &sata_physical_drive_flush_write_cache_cond,
    &sata_physical_drive_disable_write_cache_cond,
    &sata_physical_drive_enable_rla_cond,
    &sata_physical_drive_set_pio_mode_cond,
    &sata_physical_drive_set_udma_mode_cond,
    &sata_physical_drive_enable_smart_cond,
    &sata_physical_drive_enable_smart_autosave_cond,
    &sata_physical_drive_smart_return_status_cond,
    &sata_physical_drive_smart_read_attributes_cond,
    &sata_physical_drive_sct_set_read_timer_cond,
    &sata_physical_drive_sct_set_write_timer_cond,
    &sata_physical_drive_read_test_cond,
    &sata_physical_drive_detach_stp_edge_cond,
    &sata_physical_drive_retry_get_dev_info_cond,
    &sata_physical_drive_get_permission_cond,
    &sata_physical_drive_release_permission_cond,
    (fbe_lifecycle_const_base_cond_t *)&sata_physical_drive_release_permission_timer_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sata_physical_drive);

static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_specialize_rotary[] = {
    /* Derived conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_attach_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_protocol_edge_detached_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_protocol_edge_closed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_identify_device_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_write_inscription_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_read_inscription_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};


static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_reset_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_protocol_edge_closed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),  */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_get_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_release_permission_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_identify_device_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_read_inscription_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_release_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_check_power_mode_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_execute_device_diag_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_read_native_max_address_ext_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_flush_write_cache_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_disable_write_cache_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_enable_rla_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_set_pio_mode_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_set_udma_mode_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_enable_smart_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_enable_smart_autosave_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_smart_return_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_smart_read_attributes_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_sct_set_read_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_sct_set_write_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_read_test_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

};


static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_ready_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),*/
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_retry_get_dev_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
};

static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_hibernate_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_passive_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_active_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_fail_rotary[] = {
    /* Derived conditions */    
    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_release_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t sata_physical_drive_destroy_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_physical_drive_detach_stp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */

};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sata_physical_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sata_physical_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sata_physical_drive_activate_rotary),

    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sata_physical_drive_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, sata_physical_drive_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, sata_physical_drive_fail_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, sata_physical_drive_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sata_physical_drive);

/*--- global sata enclosure lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sata_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sata_physical_drive,
        FBE_CLASS_ID_SATA_PHYSICAL_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_physical_drive))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
sata_physical_drive_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
sata_physical_drive_attach_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_attach_protocol_edge_cond_completion, sata_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sata_physical_drive_attach_stp_edge(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_attach_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_path_state_t path_state;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check the path state of the edge */
    fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);

    if(path_state != FBE_PATH_STATE_INVALID){   
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_open_protocol_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_open_protocol_edge_cond_completion, sata_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sata_physical_drive_open_stp_edge(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_open_protocol_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_path_attr_t path_attr;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check the path attributes of stp edge */
    fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &path_attr);

    if(path_attr & FBE_STP_PATH_ATTR_CLOSED){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            "%s STP edge closed\n", __FUNCTION__);

        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                (fbe_base_object_t*)sata_physical_drive, 
                FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_PROTOCOL_ADDRESS);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set get_protocol_address condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }

    } else {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

     
    fbe_sata_physical_drive_init(sata_physical_drive);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sata_physical_drive_protocol_edge_detached_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_protocol_edge_detached_cond_completion, sata_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sata_physical_drive_attach_stp_edge(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_protocol_edge_detached_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_protocol_edge_closed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_protocol_edge_closed_cond_completion, sata_physical_drive);

    /* Call the userper function to do actual job */
    fbe_sata_physical_drive_open_stp_edge(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_protocol_edge_closed_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_path_attr_t path_attr;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &path_attr);

    if(path_attr & FBE_STP_PATH_ATTR_CLOSED){
        
        /* Check protocol address */
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                (fbe_base_object_t*)sata_physical_drive, 
                FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_PROTOCOL_ADDRESS);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set get_protocol_address condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    
    /* We need to check some conditions after protocol edge is opened */
    status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)sata_physical_drive, 
            FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_IDENTIFY_DEVICE);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set identity_invalid condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_identify_device_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Allocate a buffer */
    drive_info = (fbe_physical_drive_information_t *)fbe_transport_allocate_buffer();
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
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
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_identify_device_cond_completion, sata_physical_drive);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_identify_device(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_identify_device_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status; 
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)context;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sata_physical_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    fbe_u8_t zrd_sr_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];

    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);


    fbe_payload_control_get_buffer(control_operation, &drive_info);
    
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK;
    }

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    
    if (status != FBE_STATUS_OK) {
        /* We can't get the identify device buffer, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
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
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        
        fbe_zero_memory(zrd_sr_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);        
        if(fbe_equal_memory(info_ptr->serial_number, zrd_sr_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE) == false)
        {   /*non-zero serial number stored, verify if drive is same or different one*/
            if(fbe_equal_memory(info_ptr->serial_number, drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE) == true)
            {
                /*same drive*/                
                fbe_base_physical_drive_set_outstanding_io_max((fbe_base_physical_drive_t *) sata_physical_drive, drive_info->drive_qdepth);
                fbe_base_physical_drive_set_block_size((fbe_base_physical_drive_t *) sata_physical_drive, drive_info->block_size);
                fbe_base_physical_drive_set_block_count((fbe_base_physical_drive_t *)sata_physical_drive, drive_info->block_count);                
                /*  Set the new FW revision in PDO if it changed due to FW download */
                if(fbe_equal_memory(info_ptr->revision, drive_info->product_rev, FBE_SCSI_INQUIRY_REVISION_SIZE) == false) 
                {
                    fbe_copy_memory(info_ptr->revision, drive_info->product_rev, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);
                }
            }
            else
            {
                /*different drives, destroy the current object and have enclosure create new one*/
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      "%s validation failed\n",
                                      __FUNCTION__);

                status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)sata_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set DESTROY condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
        }
        else
        {
            /*drive information getting read first time*/
            fbe_base_physical_drive_set_outstanding_io_max((fbe_base_physical_drive_t *) sata_physical_drive, drive_info->drive_qdepth);
            fbe_base_physical_drive_set_block_size((fbe_base_physical_drive_t *) sata_physical_drive, drive_info->block_size);
            fbe_base_physical_drive_set_block_count((fbe_base_physical_drive_t *)sata_physical_drive, drive_info->block_count);
            
            fbe_copy_memory(info_ptr->serial_number, drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
            
            info_ptr->speed_capability = drive_info->speed_capability;
            info_ptr->drive_vendor_id = drive_info->drive_vendor_id;
            fbe_copy_memory(info_ptr->revision, drive_info->product_rev, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);       
            fbe_copy_memory(info_ptr->part_number, drive_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
        }
    }
    fbe_transport_release_buffer(drive_info);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_read_native_max_address_ext_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_read_native_max_address_ext_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_read_native_max_address_ext(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_read_native_max_address_ext_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_read_test_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_read_test_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_read_test(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_read_test_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_write_inscription_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_write_inscription_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_write_inscription(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_write_inscription_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_read_inscription_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_information_t * drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Allocate a buffer */
    drive_info = (fbe_physical_drive_information_t *)fbe_transport_allocate_buffer();
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate control payload */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
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
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_read_inscription_cond_completion, sata_physical_drive);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_read_inscription(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_read_inscription_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status; 
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)context;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sata_physical_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    fbe_u32_t powersave_attr = 0;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    fbe_payload_control_get_buffer(control_operation, &drive_info);
    
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_STATUS_OK;
    }

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    
    if (status != FBE_STATUS_OK) {
        /* We can't get the read sector buffer, retry again */
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        fbe_transport_release_buffer(drive_info);
        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_STATUS_OK; 
    }
    
    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
       
        /* Copy vendor and SN info */
        /* TODO: Check Serial number here */
        /*if my prev S/N is valid (already known), compare info_ptr->serial_number with drive_info->product_serial_num 
        and if they do not match you set FAIL condition*/
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

        if (drive_info->spin_down_qualified) {
            fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, &powersave_attr);
            powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED;
            fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, powersave_attr);
        }

        if ((info_ptr->drive_vendor_id == FBE_DRIVE_VENDOR_SAMSUNG) &&
            (((fbe_base_object_t *)sata_physical_drive)->class_id == FBE_CLASS_ID_SATA_PHYSICAL_DRIVE))
        {
            fbe_base_object_change_class_id((fbe_base_object_t *) sata_physical_drive, FBE_CLASS_ID_SATA_FLASH_DRIVE);
        }
    }

    fbe_transport_release_buffer(drive_info);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_check_power_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_check_power_mode_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_check_power_mode(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_check_power_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_execute_device_diag_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_execute_device_diag_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_execute_device_diag(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_execute_device_diag_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_flush_write_cache_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_flush_write_cache_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_flush_write_cache(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_flush_write_cache_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_disable_write_cache_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_disable_write_cache_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_disable_write_cache(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_disable_write_cache_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_enable_rla_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_enable_rla_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_enable_rla(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_enable_rla_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_set_pio_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_set_pio_mode_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_set_pio_mode(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_set_pio_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_set_udma_mode_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_set_udma_mode_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_set_udma_mode(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_set_udma_mode_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_enable_smart_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_enable_smart_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_enable_smart(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_enable_smart_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_enable_smart_autosave_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_enable_smart_autosave_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_enable_smart_autosave(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_enable_smart_autosave_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_smart_return_status_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_smart_return_status_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_smart_return_status(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_smart_return_status_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_smart_read_attributes_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_smart_read_attributes_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_smart_read_attributes(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_smart_read_attributes_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_sct_set_read_timer_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_sct_set_read_timer_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_sct_set_read_timer(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_sct_set_read_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_sct_set_write_timer_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_sct_set_write_timer_cond_completion, sata_physical_drive);

    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_sct_set_write_timer(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_sct_set_write_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_control_status_t control_status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_detach_stp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_detach_stp_edge_cond_completion, sata_physical_drive);

    fbe_sata_physical_drive_detach_stp_edge(sata_physical_drive, packet);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_detach_stp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_path_state_t path_state;
    
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);

    if(path_state == FBE_PATH_STATE_INVALID){   
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_reset_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_reset_cond_completion, sata_physical_drive);

    /* Call the usurper function to do actual job */
    fbe_sata_physical_drive_reset_device(sata_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}


static fbe_status_t 
sata_physical_drive_reset_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.reset_count++;


    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_protocol_edges_not_ready_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;
    fbe_path_attr_t path_attr;

    sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* All protocol edge manipulations are synchronous - ther are no hardware envolvment
       We do not have to break the execution context.
    */
    fbe_semaphore_init(&sem, 0, 1);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_protocol_edges_not_ready_cond_completion, &sem);

    /* Call the executer function to do actual job */
    fbe_base_physical_drive_get_protocol_address((fbe_base_physical_drive_t *)sata_physical_drive, packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_payload_control_get_status(control_operation, &control_status);
    status = fbe_transport_get_status_code(packet);
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK)){
        /* We were unable to get protocol address. */
        fbe_semaphore_destroy(&sem);
        fbe_transport_complete_packet(packet);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Check if we need to attach protocol edge */
    /* Get the path state of the edge */
    fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);
    if(path_state == FBE_PATH_STATE_INVALID){ /* Not attached */
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, sata_physical_drive_protocol_edges_not_ready_cond_completion, &sem);
        /* Call the userper function to do actual job */
        fbe_sata_physical_drive_attach_stp_edge(sata_physical_drive, packet);
        fbe_semaphore_wait(&sem, NULL); /* Wait for completion */
        /* Check path state again */
        fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);
        if(path_state == FBE_PATH_STATE_INVALID){ /* Not attached */
            /* We were unable to attach stp edge. */
            fbe_semaphore_destroy(&sem);
            fbe_transport_complete_packet(packet);
            return FBE_LIFECYCLE_STATUS_PENDING;        
        }
    }

    /* Check if path state is ENABLED */
    if(path_state != FBE_PATH_STATE_ENABLED){ /* We should wait until path state will become ENABLED */
        fbe_semaphore_destroy(&sem);
        fbe_transport_complete_packet(packet);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    /* Check if our edge is closed */
    /* Check the path attributes of stp edge */
    fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &path_attr);

    if(path_attr & FBE_STP_PATH_ATTR_CLOSED){ /* The edge is closed */
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, sata_physical_drive_protocol_edges_not_ready_cond_completion, &sem);
        /* Call the userper function to do actual job */
        fbe_sata_physical_drive_open_stp_edge(sata_physical_drive, packet);
        fbe_semaphore_wait(&sem, NULL); /* Wait for completion */
        /* Check path attributes again again */
        fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &path_attr);
        if(path_attr & FBE_STP_PATH_ATTR_CLOSED){ /* The edge is closed */
            /* We were unable to open stp edge. */
            fbe_semaphore_destroy(&sem);
            fbe_transport_complete_packet(packet);
            return FBE_LIFECYCLE_STATUS_PENDING;        
        }
    }

    fbe_semaphore_destroy(&sem);
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    fbe_transport_complete_packet(packet);
    return FBE_LIFECYCLE_STATUS_PENDING;
}


static fbe_status_t 
sata_physical_drive_protocol_edges_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    /* Simply release the sempahore the caller is waiting on. */
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_lifecycle_status_t 
sata_physical_drive_fw_download_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)base_object;
    fbe_packet_t * usurper_packet;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_queue_element_t * queue_element = NULL;
    fbe_queue_head_t * queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t i = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
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
       if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP)
        {
            /* Process FW_DOWNLOAD_STOP usurper command */
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't clear current condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            fbe_transport_set_status(usurper_packet, status, 0);
            fbe_transport_complete_packet(usurper_packet);
            fbe_base_object_usurper_queue_lock(base_object);
            queue_element = fbe_queue_front(queue_head);
        }
        else if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD)
        {
            /* Process FW_DOWNLOAD usurper command */
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
            status = sata_physical_drive_fw_download(sata_physical_drive, packet, usurper_packet);
            return FBE_LIFECYCLE_STATUS_PENDING;
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

static fbe_status_t 
sata_physical_drive_fw_download(fbe_sata_physical_drive_t * sata_physical_drive, 
                                 fbe_packet_t * packet, 
                                 fbe_packet_t * usurper_packet)
{
    fbe_status_t status;

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet, usurper_packet);

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sata_physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sata_physical_drive, packet);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(usurper_packet, sata_physical_drive_fw_download_completion, sata_physical_drive);
    /* Call the executer function to do actual job */
    fbe_sata_physical_drive_firmware_download(sata_physical_drive, usurper_packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_fw_download_completion(fbe_packet_t * usurper_packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_packet_t * master_packet = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    /* Check packet status */
    status = fbe_transport_get_status_code(usurper_packet);

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(usurper_packet);
    fbe_transport_remove_subpacket(usurper_packet);

    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sata_physical_drive, master_packet);
    /* Complete master packet here. */
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t sata_physical_drive_power_save_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sata_physical_drive_t *             sata_physical_drive = (fbe_sata_physical_drive_t*)base_object;
    fbe_packet_t *                          usurper_packet = NULL;
    fbe_payload_control_operation_opcode_t  control_code = 0;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t                               i = 0;
    fbe_payload_ex_t  *                        payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_u32_t                               powersave_attr = 0;
    
    fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);
        payload  = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, &powersave_attr);
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
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                           FBE_TRACE_LEVEL_INFO,
                                                           "%s Spin down failed: exceed retry limit.\n",
                                                           __FUNCTION__);
                fbe_base_object_usurper_queue_unlock(base_object);
                fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);

                /* Set the BO:FAIL lifecycle condition */
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_physical_drive, 
                                                     FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, (fbe_base_object_t*)sata_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
                }
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            payload->physical_drive_transaction.retry_count++;
            fbe_base_object_usurper_queue_unlock(base_object);
            status = sata_physical_drive_enter_power_save(sata_physical_drive, packet);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        else if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE)
        {
            powersave_attr &= ~FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
            fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, powersave_attr);
            
            /* Process FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE usurper command */
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);


            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
            }   

            /*and jumpt to activate*/
            status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sata_physical_drive, 
                                    FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(usurper_packet);
            fbe_base_object_usurper_queue_lock(base_object);
            queue_element = fbe_queue_front(queue_head);
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

static fbe_status_t 
sata_physical_drive_enter_power_save(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

#if 0
    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sata_physical_drive, &path_attr);
    if (status == FBE_STATUS_OK && (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED)) 
    {
        /* Send a command to enclosure object via discovery edge. */
        status = fbe_transport_set_completion_function(packet, sata_physical_drive_enter_power_save_completion, sata_physical_drive);
        fbe_sata_physical_drive_set_power_save(sata_physical_drive, packet, TRUE);
    }
    else
    {
        /* Send a command to port object via STP edge. */
        status = fbe_transport_set_completion_function(packet, sata_physical_drive_enter_power_save_completion, sata_physical_drive);
        fbe_sata_physical_drive_spin_down(sata_physical_drive, packet);
    }
#endif 

    /* Send a command to port object via STP edge. */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_enter_power_save_completion, sata_physical_drive);
    fbe_sata_physical_drive_spin_down(sata_physical_drive, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t sata_physical_drive_enter_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t           *sata_physical_drive = NULL;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_u32_t powersave_attr = 0;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_ERROR,
                          "%s packet finished with error: %d\n", __FUNCTION__, status);
        
        return FBE_STATUS_OK;
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_physical_drive_get_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, &powersave_attr);
        powersave_attr |= FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
        fbe_base_physical_drive_set_powersave_attr((fbe_base_physical_drive_t*)sata_physical_drive, powersave_attr);
        status = fbe_lifecycle_reschedule(&fbe_sata_physical_drive_lifecycle_const, 
                                          (fbe_base_object_t*)sata_physical_drive,  
                                          0);
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_ERROR,
                          "%s payload finished with error: %d\n", __FUNCTION__, control_status);

    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
sata_physical_drive_retry_get_dev_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sata_physical_drive_t *             sata_physical_drive = (fbe_sata_physical_drive_t*)base_object;
    fbe_packet_t *                          usurper_packet = NULL;
    fbe_payload_control_operation_opcode_t  control_code = 0;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_u32_t                               i = 0;
    fbe_payload_ex_t  *                        payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element)
    {
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);
        payload  = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        
        if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION)
        {
            /* Process FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION usurper command */
            if(sata_physical_drive->get_dev_info_retry_count >= FBE_SATA_PHYSICAL_DRIVE_MAX_GET_INFO_RETRIES)
            {
                status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                fbe_base_object_usurper_queue_unlock(base_object);
                fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(usurper_packet);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);

            /* We need to assosiate monitor packet with original one */
            fbe_transport_add_subpacket(packet, usurper_packet);

            /* Put packet on the termination queue while we wait for the subpacket to complete. */
            fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sata_physical_drive);
            fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sata_physical_drive, packet);

            status = fbe_transport_set_completion_function(usurper_packet, sata_physical_drive_retry_get_dev_info_cond_completion, sata_physical_drive);
            
            if (sata_physical_drive->sata_drive_info.is_inscribed)
            {
                /* Send to executer to read inscription. */
                status = fbe_sata_physical_drive_read_inscription(sata_physical_drive, usurper_packet);
            }
            else
            {
                /* Send to executer for identify. */
                status = fbe_sata_physical_drive_identify_device(sata_physical_drive, usurper_packet);
            }
            return FBE_LIFECYCLE_STATUS_PENDING;
            
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


static fbe_status_t 
sata_physical_drive_retry_get_dev_info_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sata_physical_drive_t *     sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_packet_t * master_packet = NULL;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK;
    }

    /* Get master packet (monitor packet) */
    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    fbe_transport_remove_subpacket(packet);
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sata_physical_drive, master_packet);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
        }
     }
    else
    {
        sata_physical_drive->get_dev_info_retry_count++;
        /* Enqueue usurper packet */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sata_physical_drive, packet);
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return status;
}

static fbe_lifecycle_status_t sata_physical_drive_power_save_active_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_lifecycle_clear_current_cond(base_object);

    /*SATA DRIVE NOT SUPPORTED ANYMORE, not point on implementng power saving*/
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)base_object, 
                                                FBE_TRACE_LEVEL_INFO,
                                               "Saving power(Active)\n");

    
    
    return FBE_LIFECYCLE_STATUS_DONE;
    

}

static fbe_lifecycle_status_t sata_physical_drive_power_save_passive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    /*shay:in original implementation, passive command did nothing so we keep it the same here*/
    fbe_lifecycle_clear_current_cond(base_object);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                "Saving power (passive)\n");

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t 
sata_physical_drive_get_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;
    fbe_path_attr_t path_attr;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;   

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Check path state. If the enclosure has failed ( path state is broken )
    *   we will clear this condition to prevent "invalid packet status" error messages from
    *   the associated base physical drive executer completion from filling the trace.
    *   This was seen with SAS drives but is also applicable to SATA drives
    */
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sata_physical_drive, &path_state);
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN)
    {
        /* Path is broken. Clear this condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;       
    }   
    
    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sata_physical_drive, &path_attr);

    /* hack here! Always set PERMISSION_GRANTED until enclosure object implemented it */
    //path_attr |= FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED;

    if((status == FBE_STATUS_OK) && (path_attr & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: spin-up credit granted.",
                                __FUNCTION__);
        /* Permission granted. Clear the condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }

        /* Start a timer to release spinup credit. */
        status = fbe_lifecycle_force_clear_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sata_physical_drive, 
                                            FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT_TIMER);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't start a timer to release spinup credit, status: 0x%X\n",
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
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_get_permission_cond_completion, sata_physical_drive);

    /* Call the executor function to get permission */
    status = fbe_base_physical_drive_get_permission((fbe_base_physical_drive_t *)sata_physical_drive, packet, TRUE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_get_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    /* Check the control status */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s Invalid control_status: 0x%X",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
sata_physical_drive_release_permission_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)p_object;
    fbe_path_attr_t path_attr;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Check path state. If the enclosure has failed ( path state is broken )
    *   we will clear this condition to prevent "invalid packet status" error messages from
    *   the associated base physical drive executer completion from filling the trace.
    *   This was seen with SAS drives but is also applicable to SATA drives
    */
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sata_physical_drive, &path_state);
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN)
    {
        /* Path is broken. Clear this condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;       
    }       

    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sata_physical_drive, &path_attr);

    if((status == FBE_STATUS_OK) && !(path_attr & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't clear current condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sata_physical_drive_release_permission_cond_completion, sata_physical_drive);

    /* Call the executor function */
    status = fbe_base_physical_drive_get_permission((fbe_base_physical_drive_t *)sata_physical_drive, packet, FALSE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_release_permission_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    sata_physical_drive = (fbe_sata_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    /* Check the control status */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s Invalid control_status: 0x%X",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: released spin-up credit.",
                                __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}
