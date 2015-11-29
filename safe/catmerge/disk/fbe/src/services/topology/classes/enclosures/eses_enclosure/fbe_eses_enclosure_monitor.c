/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains monitor functions for the ESES enclosure class.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   10/28/2008:  Created file from sas_eses_enclosure. BP
 *
 ***************************************************************************/

#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_private.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"

/*!*************************************************************************
* @fn fbe_eses_enclosure_monitor_entry(                  
*                    fbe_object_handle_t object_handle, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This is the entry point for the monitor.
*
* @param      object_handle - object handle.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = NULL;

    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
  
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_eses_enclosure_lifecycle_const, (fbe_base_object_t*)eses_enclosure, packet);

    return status;
}

fbe_status_t fbe_eses_enclosure_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(eses_enclosure));
}

/*--- local function prototypes --------------------------------------------------------*/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t eses_enclosure_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t eses_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t eses_enclosure_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t eses_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_status_t
fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                  fbe_packet_t * packet,
                                                  fbe_eses_ctrl_opcode_t opcode);
static fbe_lifecycle_status_t 
eses_enclosure_inquiry_data_not_exist_cond_function(fbe_base_object_t * p_object, 
                                                         fbe_packet_t * packet);
static fbe_status_t 
eses_enclosure_inquiry_data_not_exist_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t 
eses_enclosure_identity_not_validated_cond_function(fbe_base_object_t * p_object, 
                                                         fbe_packet_t * packet);
static fbe_status_t eses_enclosure_common_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t eses_enclosure_mode_select_cond_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t eses_enclosure_config_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t eses_enclosure_sas_encl_type_unknown_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t eses_enclosure_sas_encl_type_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_rp_size_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_additional_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_emc_specific_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_emc_statistics_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_configuration_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_firmware_download_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t eses_enclosure_firmware_download_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t eses_enclosure_emc_specific_control_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_expander_control_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_duplicate_ESN_unchecked_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_mode_unsensed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t eses_enclosure_mode_unselected_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_enclosure_status_t fbe_eses_enclosure_slot_discovery_update(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t slot_index);
static fbe_enclosure_status_t fbe_eses_enclosure_slot_update_bypass_attr(fbe_eses_enclosure_t * eses_enclosure, 
                                                                         fbe_u8_t slot_index,
                                                                         fbe_u32_t curr_discovery_path_attrs);
static fbe_enclosure_status_t fbe_eses_enclosure_slot_update_power_off_attr(fbe_eses_enclosure_t * eses_enclosure, 
                                                                         fbe_u8_t slot_index,
                                                                         fbe_u32_t curr_discovery_path_attrs);
static fbe_enclosure_status_t fbe_eses_enclosure_slot_update_power_cycle_attr(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t slot_num,
                                       fbe_u32_t curr_discovery_path_attrs);
static fbe_enclosure_status_t fbe_eses_enclosure_slot_update_phy_attr(fbe_eses_enclosure_t * eses_enclosure,
                                                              fbe_u8_t slot_num,
                                                              fbe_u32_t curr_discovery_path_attrs);
static fbe_enclosure_status_t fbe_eses_enclosure_expansion_port_discovery_update(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t port_num);
static fbe_enclosure_status_t fbe_eses_enclosure_expansion_port_check_bypass_state(fbe_eses_enclosure_t * eses_enclosure, 
                                                                                   fbe_u8_t entire_connector_index,
                                                                                   fbe_u32_t * discovery_path_attr_to_set_p);
static fbe_lifecycle_status_t eses_enclosure_init_ride_through_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static void fbe_eses_enclosure_update_poll_count(fbe_eses_enclosure_t *eses_enclosure);
static fbe_bool_t fbe_eses_enclosure_is_emc_specific_status_needed(fbe_eses_enclosure_t * eses_enclosure);

static fbe_bool_t eses_enclosure_need_to_notify_upstream(fbe_eses_enclosure_t * eses_enclosure);
static fbe_status_t fbe_eses_enclosure_process_fup_status_change(fbe_eses_enclosure_t * pEsesEncl);
static fbe_status_t fbe_eses_enclosure_get_fup_data_change_info(fbe_eses_enclosure_t * pEsesEncl,
                                            fbe_eses_encl_fw_component_info_t * pFwInfo,
                                            fbe_notification_data_changed_info_t * pDataChangeInfo);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(eses_enclosure);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(eses_enclosure);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(eses_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        eses_enclosure,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t eses_enclosure_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        eses_enclosure_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t eses_enclosure_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        eses_enclosure_discovery_update_cond_function)
};

static fbe_lifecycle_const_cond_t eses_enclosure_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        eses_enclosure_discovery_poll_cond_function)
};

#if 0
/* not using derived condition but declaring our own */
static fbe_lifecycle_const_cond_t eses_enclosure_firmware_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD,
        eses_enclosure_firmware_download_cond_function)
};
#endif
static fbe_lifecycle_const_cond_t eses_enclosure_get_element_list_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST,
        eses_enclosure_get_element_list_cond_function)
};

static fbe_lifecycle_const_cond_t eses_enclosure_inquiry_data_not_exist_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY,
        eses_enclosure_inquiry_data_not_exist_cond_function)
};

static fbe_lifecycle_const_cond_t eses_enclosure_init_ride_through_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_INIT_RIDE_THROUGH,
        eses_enclosure_init_ride_through_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/
/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_status_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_status_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN,
        eses_enclosure_status_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_ADDL_STATUS_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_additional_status_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_additional_status_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_ADDL_STATUS_UNKNOWN,
        eses_enclosure_additional_status_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_emc_specific_status_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_emc_specific_status_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN,
        eses_enclosure_emc_specific_status_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CHILDREN_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_children_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_children_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CHILDREN_UNKNOWN,
        eses_enclosure_get_element_list_cond_function),  /* reuse the same handler */
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_identity_not_validated_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_identity_not_validated_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED,
        eses_enclosure_identity_not_validated_cond_function),  
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CONFIGURATION_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_configuration_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_configuration_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CONFIGURATION_UNKNOWN,
        eses_enclosure_configuration_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_expander_control_needed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_expander_control_needed_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED,
        eses_enclosure_expander_control_needed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_emc_specific_control_needed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_emc_specific_control_needed_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED,
        eses_enclosure_emc_specific_control_needed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_DUPLICATE_ESN_UNCHECKED condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_duplicate_ESN_unchecked_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_duplicate_ESN_unchecked_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_DUPLICATE_ESN_UNCHECKED,
        eses_enclosure_duplicate_ESN_unchecked_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* This condition can't be the override from base_enclosure, because it needs to be put at
 * the very end of ready rotary.
 */
static fbe_lifecycle_const_base_cond_t eses_enclosure_firmware_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_firmware_download_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD,
        eses_enclosure_firmware_download_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,        /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,          /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t eses_enclosure_mode_unsensed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_mode_unsensed_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSENSED,
        eses_enclosure_mode_unsensed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t eses_enclosure_mode_unselected_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_mode_unselected_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSELECTED,
        eses_enclosure_mode_unselected_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t eses_enclosure_emc_statistics_status_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_emc_statistics_status_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_STATISTICS_STATUS_UNKNOWN,
        eses_enclosure_emc_statistics_status_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

/* FBE_ESES_ENCLOSURE_LIFECYCLE_COND_SAS_ENCL_TYPE_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t eses_enclosure_sas_encl_type_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_sas_encl_type_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_SAS_ENCL_TYPE_UNKNOWN,
        eses_enclosure_sas_encl_type_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t eses_enclosure_rp_size_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "eses_enclosure_rp_size_unknown_cond",
        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_RP_SIZE_UNKNOWN,
        eses_enclosure_rp_size_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* 
 * The conditions here in the BASE_COND_ARRY must match the condition order 
 * in the fbe_eses_enclosure_lifecycle_cond_id_e enum.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(eses_enclosure)[] = {
    &eses_enclosure_status_unknown_cond,
    &eses_enclosure_additional_status_unknown_cond,
    &eses_enclosure_emc_specific_status_unknown_cond,
    &eses_enclosure_children_unknown_cond,
    &eses_enclosure_identity_not_validated_cond,
    &eses_enclosure_configuration_unknown_cond,
    &eses_enclosure_firmware_download_cond,
    &eses_enclosure_expander_control_needed_cond,
    &eses_enclosure_emc_specific_control_needed_cond,
    &eses_enclosure_duplicate_ESN_unchecked_cond,
    &eses_enclosure_mode_unsensed_cond,
    &eses_enclosure_mode_unselected_cond,
    &eses_enclosure_emc_statistics_status_unknown_cond,
    &eses_enclosure_sas_encl_type_unknown_cond,
    &eses_enclosure_rp_size_unknown_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(eses_enclosure);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t eses_enclosure_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_inquiry_data_not_exist_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_sas_encl_type_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t eses_enclosure_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_init_ride_through_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_identity_not_validated_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* duplicate ESN check is disabled, leave the check to AEN, outside physical package */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_duplicate_ESN_unchecked_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
    /* Enclosure need to be operating under correct mode first */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_mode_unsensed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_mode_unselected_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_configuration_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_additional_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_rp_size_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_specific_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_statistics_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_expander_control_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_specific_control_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_children_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
};

static fbe_lifecycle_const_rotary_cond_t eses_enclosure_ready_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
//    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_firmware_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_get_element_list_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_additional_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_rp_size_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_specific_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_statistics_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_mode_unsensed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_mode_unselected_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_expander_control_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_emc_specific_control_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_firmware_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t eses_enclosure_fail_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_expander_control_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(eses_enclosure_rp_size_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(eses_enclosure)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, eses_enclosure_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, eses_enclosure_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, eses_enclosure_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, eses_enclosure_fail_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(eses_enclosure);

/*--- global eses enclosure lifecycle constant data ----------------------------------------*/
fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(eses_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        eses_enclosure,
        FBE_CLASS_ID_ESES_ENCLOSURE,          /* This class */
        FBE_LIFECYCLE_CONST_DATA(sas_enclosure))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
/*!*************************************************************************
* @fn eses_enclosure_discovery_poll_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition is scheduled to run every 3 second.  It triggers
* two other conditions to refresh enclosure status and children list.
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
* @todo
*       Refreshing of the children list should be removed from here once
* the notification from port about topology changes is provided.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );


    // Update the poll count.
    fbe_eses_enclosure_update_poll_count(eses_enclosure);

    /* We need to get the latest status from time to time.
     * We should have set FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE here
     * to allow recreate child objects. But since completion routine for get_status
     * sets it, we skip it here.
     */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                __FUNCTION__, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* This condition should be set when discovery completes.
     * i.e. the FBE_DISCOVERY_PATH_ATTR_DISCOVERY_IN_PROGRESS is cleared.
     *This condition will be set whenever FBE_SMP_PATH_ATTR_ELEMENT_CHANGE is detected
     */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST);
    

    if(fbe_eses_enclosure_is_time_sync_needed(eses_enclosure))
    {
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);
        if (status != FBE_STATUS_OK) 
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "Discovery_poll, can't set EMC_SPECIFIC_CONTROL_NEEDED, status: 0x%x.\n",
                        status);
   
        }
    }

    // See if EMC specific status needs to be requested
    if(fbe_eses_enclosure_is_emc_specific_status_needed(eses_enclosure))
    {
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);
        if (status != FBE_STATUS_OK) 
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "Discovery_poll, can't set EMC_SPECIFIC_STATUS_UNKNOWN, status: 0x%x.\n",
                        status);
   
        }
    }

    /* We set the condition which we have deferred while retrying SCSI command
     * when enclosure object was not in ready state.
     */
    if(eses_enclosure->update_lifecycle_cond) 
    {
        if(eses_enclosure->update_lifecycle_cond & FBE_ESES_UPDATE_COND_EMC_SPECIFIC_CONTROL_NEEDED)
        {
            eses_enclosure->update_lifecycle_cond &= ~FBE_ESES_UPDATE_COND_EMC_SPECIFIC_CONTROL_NEEDED;

            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                    (fbe_base_object_t*)eses_enclosure, 
                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Discovery_poll, can't set EMC_SPECIFIC_CONTROL_NEEDED condition, status: 0x%x.\n",
                            status);
        
            }
        }
        if (eses_enclosure->update_lifecycle_cond & FBE_ESES_UPDATE_COND_MODE_UNSENSED)
        {
            eses_enclosure->update_lifecycle_cond &= ~FBE_ESES_UPDATE_COND_MODE_UNSENSED;

            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                            (fbe_base_object_t*)eses_enclosure, 
                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSENSED);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Discovery_poll, can't set MODE_UNSENSED condition, status: 0x%x.\n",
                            status);
        
            }
        }
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
    if (status != FBE_STATUS_OK) {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s can't clear current condition, status: 0x%x.\n",
                        __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_update_poll_count()                  
***************************************************************************
* @brief
*       This function updates the poll count maintained in the ESES enclosure.
*
* @param   
*       eses_enclosure - The pointer to the ESES enclosure.
* 
* @return  none
*
* NOTES
*   This function should only be called in the monitor context whenever the
*   the ESES enclosure base poll condition fires.
*   (FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL)
*
* HISTORY
*   02-Nov-2009 - Rajesh V. Created.
***************************************************************************/
static void 
fbe_eses_enclosure_update_poll_count(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    if((++eses_enclosure->poll_count) > FBE_ESES_ENCLOSURE_MAX_POLL_COUNT_WRAP)
    {
        eses_enclosure->poll_count = 0;
    }
    
}// End of function fbe_eses_enclosure_is_time_sync_needed


/*!*************************************************************************
* @fn fbe_eses_enclosure_is_emc_specific_status_needed()                  
***************************************************************************
* @brief
*       This function decides if the EMC specific status needs to 
*           be requested.
*
* @param   
*       eses_enclosure - The pointer to the ESES enclosure.
* 
* @return  fbe_bool_t.
*   FBE_TRUE -- EMC specific status needs to be requested
*   FBE_FALSE -- EMC specific status need NOT be requested.
*
* NOTES
*   This function should only be called in the monitor context whenever the
*   the ESES enclosure base poll condition fires.
*
* HISTORY
*   02-Nov-2009 - Rajesh V. Created.
***************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_is_emc_specific_status_needed(fbe_eses_enclosure_t * eses_enclosure)
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    if(((eses_enclosure->poll_count - 1) % 
        (FBE_ESES_ENCLOSURE_EMC_SPECIFIC_STATUS_POLL_FREQUENCY)) == 0)
    {
        return(TRUE);
    }

    return(FALSE);
    
}// End of function fbe_eses_enclosure_is_emc_specific_status_needed


/*!*************************************************************************
* @fn eses_enclosure_self_init_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition completes the initialization of eses enclosure object.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;

    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );


    /* This code can be taken out after framework support is in.  
     * Otherwise, we need to manually set the condition during morphing,
     * because the preset may have been cleared in sas_enclosure.
     * We need this to extract UID from inquiry data.
     */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY);

    if (status != FBE_STATUS_OK) 
    {

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s can't set inquiry condition, status: 0x%X",
                            __FUNCTION__, status);
        
    }

    /* Initialize the eses enclosure attibutes. */
    fbe_eses_enclosure_init(eses_enclosure);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn eses_enclosure_discovery_update_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition creates children objects, 
* and update the discovery path attribute to reflect login/logouts.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t            status;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_enclosure_t  * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
    fbe_u32_t i;

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );

    
    if(eses_enclosure->active_state_to_update_cond == FALSE)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                              "Enclosure is ready\n");
        
        eses_enclosure->active_state_to_update_cond = TRUE;
    }
    /* Iterate through all slots */
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i ++)
    { 
        encl_status = fbe_eses_enclosure_slot_discovery_update(eses_enclosure, (fbe_u8_t)i);
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, discovery_update, slot_discovery_update failed, "
                            "encl_status: 0x%x.\n",  i, encl_status);

            
        }

    }/* for(i = 0 ; i < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i ++) */

    /* Iterate through all expansion ports, including incoming */
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure); i ++)
    {
        /* only need to do this if we have expansion port */
        encl_status = fbe_eses_enclosure_expansion_port_discovery_update(eses_enclosure, (fbe_u8_t)i);
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "discovery_update, expansion_port_discovery_update failed, "
                            "encl_status: 0x%x.\n",  encl_status);
        }
    }

    /*
     * Check to see if there were any changes in firmware upgrade status.
     */
    status = fbe_eses_enclosure_process_fup_status_change(eses_enclosure);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "discovery_update, process_fup_status_change failed, "
                        "status: 0x%x.\n",  status);

    }

    /*
     * Check to see if there were any changes in Enclosure EDAL Data
     */
    status = fbe_base_enclosure_process_state_change((fbe_base_enclosure_t *)eses_enclosure);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "discovery_update, process_state_change failed, "
                        "status: 0x%x.\n",  status);

    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
    if (status != FBE_STATUS_OK) {

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s can't clear current condition, status: 0x%X",
                        __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
 *  @fn fbe_eses_enclosure_process_fup_status_change
 **************************************************************************
 *  @brief
 *      This function will determine whether a firmware download/activate status change
 *      has occurred. If yes, send the notification.
 *
 *  @param pBaseEncl - pointer to fbe_base_enclosure_t
 *
 *  @return fbe_status_t - status of process State Change operation
 *
 *  @note
 *
 *  @version
 *    07-Jul-2010: PHE - Created. 
 *
 **************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_process_fup_status_change(fbe_eses_enclosure_t * pEsesEncl)
{ 
    fbe_u32_t index = 0;
    fbe_bool_t needToNotify = FALSE;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_eses_encl_fw_component_info_t  * pFwInfo = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t *)pEsesEncl,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pEsesEncl),
                        "%s entry.\n", 
                        __FUNCTION__);
   
    if(pEsesEncl->enclCurrentFupInfo.enclFupComponent != FBE_FW_TARGET_INVALID) 
    {
        // Before the comparison, save the current status and additional status for the current target.
        status = fbe_eses_enclosure_get_fw_target_addr(pEsesEncl,
                                                   pEsesEncl->enclCurrentFupInfo.enclFupComponent, 
                                                   pEsesEncl->enclCurrentFupInfo.enclFupComponentSide, 
                                                   &pFwInfo);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                           "%s, Failed to get fw_target_addr for current target %s, side %d.\n", 
                                           __FUNCTION__, 
                                           fbe_eses_enclosure_fw_targ_to_text(pEsesEncl->enclCurrentFupInfo.enclFupComponent),
                                           pEsesEncl->enclCurrentFupInfo.enclFupComponentSide);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwInfo->enclFupOperationStatus = pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus;
        pFwInfo->enclFupOperationAdditionalStatus = pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus;
    }
    
    for(index = 0; index < FBE_FW_TARGET_MAX; index ++)
    {
        needToNotify = FALSE;

        pFwInfo = &pEsesEncl->enclFwInfo->subencl_fw_info[index];
            
        if((pFwInfo->enclFupOperationStatus != pFwInfo->prevEnclFupOperationStatus) || 
           (pFwInfo->enclFupOperationAdditionalStatus != pFwInfo->prevEnclFupOperationAdditionalStatus))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                               "FUP: %s, side %d, firmware status/addlStatus 0x%X/0x%X, prevStatus/prevAddlStatus 0x%X/0x%X.\n",
                                               fbe_eses_enclosure_fw_targ_to_text(pFwInfo->enclFupComponent),
                                               pFwInfo->enclFupComponentSide,
                                               pFwInfo->enclFupOperationStatus,
                                               pFwInfo->enclFupOperationAdditionalStatus,
                                               pFwInfo->prevEnclFupOperationStatus,
                                               pFwInfo->prevEnclFupOperationAdditionalStatus);  

            pFwInfo->prevEnclFupOperationStatus = pFwInfo->enclFupOperationStatus;
            pFwInfo->prevEnclFupOperationAdditionalStatus = pFwInfo->enclFupOperationAdditionalStatus;
            needToNotify = TRUE;
        }
    
        if(needToNotify)
        {
            status = fbe_eses_enclosure_get_fup_data_change_info(pEsesEncl, pFwInfo, &dataChangeInfo);
    
            if(status != FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                        "FUP: fwTarget %s, side %d, Sending firmware status change notification.\n",
                                        fbe_eses_enclosure_fw_targ_to_text(pFwInfo->enclFupComponent),
                                        pFwInfo->enclFupComponentSide);   

                fbe_base_enclosure_send_data_change_notification((fbe_base_enclosure_t *)pEsesEncl,
                                                                  &dataChangeInfo); 
            }
        }
    }
        
    return FBE_STATUS_OK;

} // End of function fbe_eses_enclosure_process_fup_status_change


/*!*************************************************************************
 *   @fn fbe_eses_enclosure_get_fup_data_change_info(fbe_eses_enclosure_t * pEsesEncl,
 *                               fbe_notification_data_changed_info_t * pDataChangeInfo)     
 **************************************************************************
 *  @brief
 *      This function will get the necessary data change info 
 *      for firmware upgrade status change notification.
 *
 *  @param pBaseEncl - The pointer to the fbe_base_enclosure_t.
 *  @param pDataChangeInfo - The pointer to the data change info for data change notification.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    07-Jul-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_eses_enclosure_get_fup_data_change_info(fbe_eses_enclosure_t * pEsesEncl,
                                            fbe_eses_encl_fw_component_info_t * pFwInfo,
                                            fbe_notification_data_changed_info_t * pDataChangeInfo) 
{
    fbe_u32_t              port = 0;
    fbe_u32_t              enclosure = 0;
    fbe_status_t           status = FBE_STATUS_OK;
     
    fbe_base_object_customizable_trace((fbe_base_object_t *)pEsesEncl,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pEsesEncl),
                        "%s entry.\n", 
                        __FUNCTION__);

    fbe_zero_memory(pDataChangeInfo, sizeof(fbe_notification_data_changed_info_t));

    fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)pEsesEncl, &port);
    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)pEsesEncl, &enclosure);

    pDataChangeInfo->phys_location.bus = port;
    pDataChangeInfo->phys_location.enclosure = enclosure;
    pDataChangeInfo->phys_location.slot = pFwInfo->enclFupComponentSide;
    pDataChangeInfo->phys_location.componentId = 
        fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)pEsesEncl);
    pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_FUP_INFO;

    switch(pFwInfo->enclFupComponent)
    {
        case FBE_FW_TARGET_PS: 
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_PS;
            break;

        case FBE_FW_TARGET_LCC_EXPANDER:
        case FBE_FW_TARGET_LCC_BOOT_LOADER:
        case FBE_FW_TARGET_LCC_INIT_STRING:
        case FBE_FW_TARGET_LCC_FPGA:
        case FBE_FW_TARGET_LCC_MAIN:
            /* Jetfire Base Module is treated as LCC in eses enclosure */
            if ((pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_FALLBACK) ||
                (pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CALYPSO))
            {
                pDataChangeInfo->phys_location.slot = 0;
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_BACK_END_MODULE;
                break;
            }
            /* Sentry and Beachcomber SP board is treated as LCC in eses enclosure */
            else if  (pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CITADEL ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_BUNKER ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_STEELJAW ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_RAMHORN ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_BOXWOOD ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_KNOT ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_RHEA ||
                 pEsesEncl->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_MIRANDA)
            {
                pDataChangeInfo->phys_location.slot = 0;
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SP;
                break;
            }
            else
            {
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_LCC;
                break;
            }
        case FBE_FW_TARGET_COOLING:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_FAN;
            break;

        case FBE_FW_TARGET_SPS_PRIMARY:
        case FBE_FW_TARGET_SPS_SECONDARY:
        case FBE_FW_TARGET_SPS_BATTERY:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SPS;
            break;

        default:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}// End of function fbe_eses_enclosure_get_fup_data_change_info


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_discovery_update(fbe_eses_enclosure_t * eses_enclosure, 
*                                                      fbe_u8_t slot_index)                 
***************************************************************************
* @brief
*       This function creates the drive children object, 
*       and update the discovery path attribute for the specified slot.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index in edal.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   24-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_slot_discovery_update(fbe_eses_enclosure_t * eses_enclosure, 
                                                      fbe_u8_t slot_index)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_discovery_edge_t  *     discovery_edge = NULL;
    fbe_bool_t                  driveInserted = FALSE;
    fbe_bool_t                  driveLoggedIn = FALSE;
    fbe_u32_t                   curr_discovery_path_attrs = 0;
    fbe_u8_t                    slot_num;

    /********
     * BEGIN
     ********/
    fbe_base_discovering_transport_server_lock((fbe_base_discovering_t *) eses_enclosure);
    discovery_edge = fbe_base_discovering_get_client_edge_by_server_index((fbe_base_discovering_t *) eses_enclosure, slot_index);
    fbe_base_discovering_transport_server_unlock((fbe_base_discovering_t *) eses_enclosure);


    // get the slot number
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slot_index,                 // drive component index
                                                &slot_num); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_INSERTED,  // attribute
                                                FBE_ENCL_DRIVE_SLOT, // Component type
                                                slot_index, // Component index
                                                &driveInserted);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_LOGGED_IN, //Attribute
                                                FBE_ENCL_DRIVE_SLOT, // Component type
                                                slot_index,  // Component index
                                                &driveLoggedIn);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

   /* (1) Drive instantiation is based on the inserted-ness.
    * If the drive is removed, set the FBE_DISCOVERY_PATH_ATTR_REMOVED attribute on the discovery edge.
    * so the drive will go destroy.
    */
    if(driveInserted) /* we have a drive inserted. */
    {
        if(discovery_edge == NULL)
        {  
            /* Drive is not instantiated. Instantiate a base physical drive object. */
            status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) eses_enclosure, 
                                                            FBE_CLASS_ID_BASE_PHYSICAL_DRIVE, 
                                                            slot_index);
        } 
    }
  
   /* Continue to set discovery path attributes. 
    * But if the discovery edage is NULL, no need to continue.
    */
    if(discovery_edge == NULL)
    {
        // No need to continue.
        return FBE_ENCLOSURE_STATUS_OK;
    }

    status = fbe_base_discovering_get_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                slot_index, 
                                                &curr_discovery_path_attrs);
    if(status != FBE_STATUS_OK)
    {
        // It should not happen.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "slot: %d, slot_discovery_update, get_path_attr failed, "
                        "status: 0x%x.\n", slot_num, status);     


        return FBE_ENCLOSURE_STATUS_PATH_ATTR_UNAVAIL;
    }

   /* (2) The setting and clearing of the attribute FBE_DISCOVERY_PATH_ATTR_REMOVED on the discovery edge 
    * is based on the drive inserted-ness.
    */
    if(driveInserted)
    {
        /* The drive is inserted and has the discovery edge. */
        if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_REMOVED)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, drive inserted, current edge attr 0x%x, will clear 0x%x.\n",
                            slot_num, curr_discovery_path_attrs, FBE_DISCOVERY_PATH_ATTR_REMOVED);

            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            FBE_DISCOVERY_PATH_ATTR_REMOVED);
        }
    }
    else
    {
        if(!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_REMOVED))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, drive removed, current edge attr 0x%x, will set 0x%x.\n",
                            slot_num, curr_discovery_path_attrs, FBE_DISCOVERY_PATH_ATTR_REMOVED);

            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            FBE_DISCOVERY_PATH_ATTR_REMOVED);
        } 
    }

    
   /* (3) The setting and clearing of the attribute FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT on the discovery edge 
    * is based on the drive logged in/out.
    */

    if(driveLoggedIn)
    {   
        /* The drive is logged in and has the discovery edge. */
        if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, drive logged in, current edge attr 0x%x, will clear 0x%x.\n",
                            slot_num, curr_discovery_path_attrs, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);

            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                    slot_index, 
                                    FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT); 
        }
    } 
    else 
    { 
        /* Peter Puhov. The path state of the edge represents the surrogate state of the object.
        We should use NOT_PRESENT bit instead.
        fbe_base_discovering_set_path_state((fbe_base_discovering_t *) eses_enclosure, i, FBE_PATH_STATE_GONE);
        */
        if(!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT))
        {
            /* drive can be in logout state forever */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, drive logged out, current edge attr 0x%x, will set 0x%x.\n",
                            slot_num, curr_discovery_path_attrs, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);

            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                slot_index, 
                                                FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
        }
    }    
    
    /* (4) Check whether the bypass related discovery edge attributes need to be cleared or set. */
    encl_status = fbe_eses_enclosure_slot_update_bypass_attr(eses_enclosure, slot_index, curr_discovery_path_attrs);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "slot: %d, slot_discovery_update, slot_check_bypass_state failed, "
                        "encl_status: 0x%x.\n", slot_num, encl_status);
        
        return encl_status;
    }

   
    /* (5) Check whether the power off related discovery edge attributes need to be cleared or set. */
    encl_status = fbe_eses_enclosure_slot_update_power_off_attr(eses_enclosure, slot_index, curr_discovery_path_attrs);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "slot: %d, slot_discovery_update, slot_update_power_off_attr failed, "
                        "encl_status: 0x%x.\n",  slot_num, encl_status);        

        
        return encl_status;
    }    

    /* (6) Check whether the power cycle related discovery edge attributes need to be cleared or set. */
    encl_status = fbe_eses_enclosure_slot_update_power_cycle_attr(eses_enclosure, slot_index, curr_discovery_path_attrs);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "slot: %d, slot_discovery_update, slot_update_power_cycle_attr failed, "
                        "encl_status: 0x%x.\n",  slot_num, encl_status);        

    }


    encl_status = fbe_eses_enclosure_slot_update_phy_attr(eses_enclosure, slot_index, curr_discovery_path_attrs);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "slot: %d, slot_discovery_update, slot_update_phy_attr failed, "
                        "encl_status: 0x%x.\n",  slot_num, encl_status);        


        return encl_status;
    }

    return encl_status;
} // End of function fbe_eses_enclosure_slot_discovery_update


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_bypass_attr(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t slot_index,
*                                              fbe_u32_t curr_discovery_path_attrs)           
***************************************************************************
* @brief
*   This function updates the slot discovery path bypass related attributes
*   based on the disable state and disable reason of the phy to which the slot
*   is connected to.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index in edal.
* @param   curr_discovery_path_attrs - The current discovery path attributes.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   24-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_slot_update_bypass_attr(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t slot_index,
                                              fbe_u32_t curr_discovery_path_attrs)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                phy_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t              phy_disabled = FALSE;
    fbe_u8_t                phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID;
    fbe_u32_t               discovery_path_bypass_attr_to_set = 0;
    fbe_u8_t                slot_num;

    /********
     * BEGIN
     ********/
    // get the slot number
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slot_index,                 // drive component index
                                                &slot_num); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get phy index.
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_DRIVE_PHY_INDEX,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        &phy_index);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get the phy_disabled. 
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EXP_PHY_DISABLED,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        &phy_disabled);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }
    
    if(phy_disabled)
    {
        // Get the phy_disable_reason. 
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_EXP_PHY_DISABLE_REASON,  //Attribute
                                            FBE_ENCL_EXPANDER_PHY, // Component type
                                            phy_index, // Component index
                                            &phy_disable_reason);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
    
        // The phy is disabled.
        switch(phy_disable_reason)
        {
            case FBE_ESES_ENCL_PHY_DISABLE_REASON_HW:
                discovery_path_bypass_attr_to_set = FBE_DISCOVERY_PATH_ATTR_BYPASSED_UNRECOV;
                break;
            
            case FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST:
                discovery_path_bypass_attr_to_set = FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST;
                break;

            case FBE_ESES_ENCL_PHY_DISABLE_REASON_PERSIST:
            default:
                discovery_path_bypass_attr_to_set = FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST;
                break;
        }
    }
    else
    {
        // The phy is enabled.
        discovery_path_bypass_attr_to_set = 0;  
    }

    if(discovery_path_bypass_attr_to_set == 0)
    {      
        if(curr_discovery_path_attrs & (FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST | 
                                        FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST |
                                        FBE_DISCOVERY_PATH_ATTR_BYPASSED_UNRECOV))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, phy enabled, current edge attr 0x%x, will clear 0x%x.\n",
                            slot_num, 
                            curr_discovery_path_attrs, 
                            FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT|FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST);

            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            (FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST |
                                            FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST)); 
        }
    }
    else 
    {
        /* The discovery path attribute has not been set yet, so set it.
         * If this is USER_POWERED_OFF -> the power on will be automatically issued. 
         * If this is HARDWARE_POWERED_OFF -> the power on will NOT be automatically issued. 
         */
        if(!(curr_discovery_path_attrs & discovery_path_bypass_attr_to_set))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, phy disabled, current edge attr 0x%x, will set 0x%x.\n",
                            slot_num, 
                            curr_discovery_path_attrs, 
                            discovery_path_bypass_attr_to_set);

            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                        slot_index, 
                                        discovery_path_bypass_attr_to_set); 
        }
    }   

    return encl_status;

} // end of function fbe_eses_enclosure_slot_update_bypass_attr


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_power_off_attr(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t slot_index,
*                                              fbe_u32_t curr_discovery_path_attrs)                  
***************************************************************************
* @brief
*   This function updates the slot discovery path power off related attributes
*   based on the slot power off state and the device off reason. 
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index in edal.
* @param   curr_discovery_path_attrs - The current discovery path attributes.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   24-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_slot_update_power_off_attr(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t slot_index,
                                              fbe_u32_t curr_discovery_path_attrs)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              slot_powered_off = FALSE;
    fbe_bool_t				power_cycle_pending = FALSE;
    fbe_u8_t                device_off_reason = FBE_ENCL_POWER_OFF_REASON_INVALID;
    fbe_u32_t               discovery_path_power_off_attr_to_set = 0;
    fbe_u8_t                slot_num;

    /********
     * BEGIN
     ********/
    // get the slot number
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slot_index,                 // drive component index
                                                &slot_num); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get the slot_powered_off. 
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWERED_OFF,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        &slot_powered_off);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    if(slot_powered_off)
    {
        // The slot is powered off.
        // Get the device_off_reason. 
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_DRIVE_DEVICE_OFF_REASON,  //Attribute
                                            FBE_ENCL_DRIVE_SLOT, // Component type
                                            slot_index, // Component index
                                            &device_off_reason);
    
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        switch(device_off_reason)
        {
            case FBE_ENCL_POWER_OFF_REASON_HW:
                discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV;
                break;
            
            case FBE_ENCL_POWER_OFF_REASON_POWER_SAVE:
            case FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE:
                discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON;
                break;

            case FBE_ENCL_POWER_OFF_REASON_NONPERSIST:
                discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST;
                break;

            case FBE_ENCL_POWER_OFF_REASON_PERSIST:
            default:
                discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST;
                break;
        }
    }
    else
    {
        // Get the power_cycle_pending.
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_POWER_CYCLE_PENDING,  //Attribute
                                            FBE_ENCL_DRIVE_SLOT, // Component type
                                            slot_index, // Component index
                                            &power_cycle_pending);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        // The slot is powered on.
        if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV)
        {
            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV);

            discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_ON_NEED_DESTROY;

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Slot: %d, POWERED_ON_NEED_DESTROY.\n", slot_num);
        }
        else if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST) &&
                !(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING))
        {
            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure,
                                            slot_index,
                                            FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST);

            discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_ON_NEED_DESTROY;

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Slot: %d, POWERED_ON_NEED_DESTROY.\n", slot_num);
        }
        else
        {
            discovery_path_power_off_attr_to_set = 0;
        }
    }

    if(discovery_path_power_off_attr_to_set == 0)
    {
        if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV)
        {
            discovery_path_power_off_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERED_ON_NEED_DESTROY;

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Slot: %d,POWERED_ON_NEED_DESTROY,current edge attr 0x%x, will clear 0x%x and set 0x%x.\n",
                            slot_num, 
                            curr_discovery_path_attrs,
                            FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV,
                            discovery_path_power_off_attr_to_set);

             fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV);

             fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                        slot_index, 
                                        discovery_path_power_off_attr_to_set); 
        }

        if(curr_discovery_path_attrs & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST | 
                                        FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST |
                                        FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, Powered On, current edge attr 0x%x, will clear 0x%x.\n",
                            slot_num, 
                            curr_discovery_path_attrs, 
                            (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST |
                             FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST |
                             FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON |
                             FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV));

            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST |
                                             FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST |
                                             FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON |
                                             FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV));
        }
    }
    else 
    {
        /* The discovery path attribute has not been set yet, so set it.
         * If this is USER_POWERED_OFF -> the power on will be automatically issued. 
         * If this is HARDWARE_POWERED_OFF -> the power on will NOT be automatically issued. 
         */
        if(!(curr_discovery_path_attrs & discovery_path_power_off_attr_to_set))
        {

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "slot: %d, Powered Off, current edge attr 0x%x, will set 0x%x.\n",
                            slot_num, 
                            curr_discovery_path_attrs, 
                            discovery_path_power_off_attr_to_set);

            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                        slot_index, 
                                        discovery_path_power_off_attr_to_set); 
        }
    }

    return encl_status;
}// End of - fbe_eses_enclosure_slot_update_power_off_attr

/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_power_cycle_attr(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t slot_index,
*                                              fbe_u32_t curr_discovery_path_attrs)                      
***************************************************************************
* @brief
*   This function updates the slot discovery path power cycle related attributes
*   based on the slot power cycle pending and power cycle completed states. 
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index in edal.
* @param   curr_discovery_path_attrs - The current discovery path attributes.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   29-Aug-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_slot_update_power_cycle_attr(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t slot_index,
                                              fbe_u32_t curr_discovery_path_attrs)  
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t              power_cycle_pending = FALSE;
    fbe_bool_t              power_cycle_completed = FALSE;
    fbe_u32_t               discovery_path_power_cycle_attr_to_set = 0;
    fbe_u8_t                slot_num;

    /********
     * BEGIN
     ********/
    // get the slot number
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slot_index,                 // drive component index
                                                &slot_num); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get the power_cycle_pending. 
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWER_CYCLE_PENDING,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        &power_cycle_pending);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get the power_cycle_completed. 
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWER_CYCLE_COMPLETED,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        &power_cycle_completed);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    
    if(power_cycle_completed)
    {
        discovery_path_power_cycle_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE;
    }
    else if(power_cycle_pending)
    {
        discovery_path_power_cycle_attr_to_set = FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING;
    }
    else 
    {
        discovery_path_power_cycle_attr_to_set = 0;
    }

    switch(discovery_path_power_cycle_attr_to_set)
    {
        case FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE:
            /* Set path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE
            * and clear path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING if needed.
            */
            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE) == 0) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "slot: %d, slot_update_power_cycle_attr, new attr: 0x%x.\n",
                                    slot_num, discovery_path_power_cycle_attr_to_set);

                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            discovery_path_power_cycle_attr_to_set); 
            }

            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING) != 0)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                slot_index, 
                                                FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING);  
            }
            break;
        
        case FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING:
            /* Set path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING if needed. */
            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING) == 0) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "slot: %d, slot_update_power_cycle_attr, new attr: 0x%x.\n",
                                    slot_num, discovery_path_power_cycle_attr_to_set);

                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_index, 
                                            discovery_path_power_cycle_attr_to_set); 
            }

            // Set the condition so that the EMC statistics status can be read again for the power cycle state.
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_STATISTICS_STATUS_UNKNOWN);

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Discovery_poll, can't set EMC_SPECIFIC_CONTROL_NEEDED, status: 0x%x.\n",
                            status);
       
            }
            break;        

        default:
            /* Clear path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING
             * and FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING if needed.
             */
            if((curr_discovery_path_attrs &(FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING |
                 FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE)) != 0)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                slot_index, 
                                                (FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING |
                                                 FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE));      
            }
            break;
    }

    return encl_status;
}// End of - fbe_eses_enclosure_slot_update_power_cycle_attr


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_phy_attr(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t slot_num,
*                                              fbe_u32_t curr_discovery_path_attrs)                      
***************************************************************************
* @brief
*   This function updates the slot discovery path phy related attributes. 
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_num - The drive slot number.
* @param   curr_discovery_path_attrs - The current discovery path attributes.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   23-Sep-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_slot_update_phy_attr(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t slot_num,
                                              fbe_u32_t curr_discovery_path_attrs)  
{
    fbe_u32_t               discovery_path_phy_attr_to_set = 0;
    fbe_u8_t                phy_index = 0;
    fbe_u8_t                phy_id = 0;
    fbe_bool_t              phy_link_ready = FALSE;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;

    /********
     * BEGIN
     ********/
    /* Get the phy Index for the slot. */
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_DRIVE_PHY_INDEX,
                                               FBE_ENCL_DRIVE_SLOT,
                                               slot_num,
                                               &phy_index);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    /* Get the phy Id for the slot. */
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_EXP_PHY_ID,
                                               FBE_ENCL_EXPANDER_PHY,
                                               phy_index,
                                               &phy_id);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    /* Get the phy link ready attribute. */
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_EXP_PHY_LINK_READY,
                                               FBE_ENCL_EXPANDER_PHY,
                                               phy_index,
                                               &phy_link_ready);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    if(phy_link_ready)
    {
        discovery_path_phy_attr_to_set = FBE_DISCOVERY_PATH_ATTR_LINK_READY;
    }
    else 
    {
        discovery_path_phy_attr_to_set = 0;
    }

    switch(discovery_path_phy_attr_to_set)
    {
        case FBE_DISCOVERY_PATH_ATTR_LINK_READY:
            /* Set path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE
            * and clear path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING if needed.
            */
            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_LINK_READY) == 0) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "slot: %d, slot_update_phy_attr, new attr: 0x%x.\n",
                                    slot_num, discovery_path_phy_attr_to_set);

                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            slot_num, 
                                            discovery_path_phy_attr_to_set); 
            }
            break;

        default:
            /* Clear path attribute FBE_DISCOVERY_PATH_ATTR_LINK_READY if needed.
             */
            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_LINK_READY) != 0)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                slot_num, 
                                                FBE_DISCOVERY_PATH_ATTR_LINK_READY);      
            }
            break;
    }

    return encl_status;
}// End of - fbe_eses_enclosure_slot_update_phy_attr

/*!*************************************************************************
* @fn fbe_eses_enclosure_expansion_port_discovery_update(fbe_eses_enclosure_t * eses_enclosure
                                                         fbe_u8_t port_num)                 
***************************************************************************
* @brief
*       This function creates the enclosure children object, 
*       and update the discovery path attribute for it.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   port_num - expansion port number.
* 
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   24-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_expansion_port_discovery_update(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_u8_t port_num)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_discovery_edge_t  * discovery_edge = NULL;
    fbe_u8_t                first_expansion_port_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                expansion_port_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                connector_sub_encl_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u32_t               curr_discovery_path_attrs = 0;
    fbe_u32_t               discovery_path_bypass_attr_to_set = 0;
    fbe_u8_t                expansion_port_entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t              connectorInserted = FALSE;
    fbe_u8_t                         side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_component_types_t  subencl_component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u8_t                         subencl_comp_index;
    fbe_u8_t                         port = 0; // for loop.


    /********
     * BEGIN
     ********/
    first_expansion_port_index = fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure);
    expansion_port_index = first_expansion_port_index + port_num;  /* maps connector id to server index */

    fbe_base_discovering_transport_server_lock((fbe_base_discovering_t *) eses_enclosure);
    discovery_edge = fbe_base_discovering_get_client_edge_by_server_index((fbe_base_discovering_t *) eses_enclosure,
                                                                           expansion_port_index);
    fbe_base_discovering_transport_server_unlock((fbe_base_discovering_t *) eses_enclosure);

    encl_status = fbe_eses_enclosure_get_connector_entire_connector_index(eses_enclosure, 
                                            port_num,
                                            &expansion_port_entire_connector_index);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)       
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "expansion_port_discovery_update, get_entire_connector failed, "
                        "encl_status: 0x%x.\n", encl_status);

        // Ignore it if expander didn't describe the port
        return encl_status;
    }

    // ignore the primary port
    for(port = 0; port < FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT; port ++) 
    {
        if (eses_enclosure->primary_port_entire_connector_index[port]== expansion_port_entire_connector_index)
        {
            return encl_status;
        }
    }

    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID,  // Attribute.
                                                FBE_ENCL_CONNECTOR,         // Component type.
                                                expansion_port_entire_connector_index,     // Component index.
                                                &connector_sub_encl_id);
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)       
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "expansion_port_discovery_update, get attached subenclosure id failed, "
                        "encl_status: 0x%x.\n", encl_status);
        return encl_status;
    }

    // for external, we need to make sure that we know which one if primary first.
    if ((eses_enclosure->primary_port_entire_connector_index[0] == FBE_ENCLOSURE_VALUE_INVALID) &&
        (connector_sub_encl_id == FBE_ESES_SUBENCL_SIDE_ID_INVALID))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "expansion_port_discovery_update, ignore %d, primary port unknown, "
                        "encl_status: 0x%x.\n", port_num, encl_status);
        return encl_status;  // status doesn't really matter
    }
   
    if (connector_sub_encl_id == FBE_ESES_SUBENCL_SIDE_ID_INVALID)
    {
        /* external connector uses connector insert status */
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,      // Component type.
                                                    expansion_port_entire_connector_index,     // Component index.
                                                    &connectorInserted);
    }
    else
    {
        /* internal connector insert status is based off the insert status of its attached sub enclosure */
        /* Get the subenclosure component type for this subencl_id. */
        encl_status = fbe_eses_enclosure_subencl_id_to_side_id_component_type(eses_enclosure,
                                             connector_sub_encl_id, 
                                             &side_id,
                                             &subencl_component_type);

        if((encl_status == FBE_ENCLOSURE_STATUS_OK) && (subencl_component_type != FBE_ENCLOSURE_VALUE_INVALID))
        {
            subencl_comp_index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SUB_ENCL_ID,  // attribute
                                                        subencl_component_type,     // Component type
                                                        0, //starting index
                                                        connector_sub_encl_id);

            if(subencl_comp_index == FBE_ENCLOSURE_VALUE_INVALID)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "expansion_port_discovery_update, Invalid subencl_comp_index for connector_sub_encl_id: %d.", connector_sub_encl_id);

                return FBE_ENCLOSURE_STATUS_OK;  // status doesn't really matter
            
            }
            else
            {
                encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                            subencl_component_type,  // Component type.
                                                            subencl_comp_index,      // Component index.
                                                            &connectorInserted);

            }
        }
    }

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        /* (1) Enclosure instantiation is based on the expansion port connector inserted-ness.
         * If the the expansion connector is removed, 
         * set the FBE_DISCOVERY_PATH_ATTR_REMOVED attribute on the discovery edge.
         * so the enclosure will go destroy.
         */
        if(connectorInserted ||(eses_enclosure->expansion_port_info[port_num].sas_address != FBE_SAS_ADDRESS_INVALID)) /* we have a cable inserted. */
        {
            if(discovery_edge == NULL)
            {
                /* Enclosure is not instantiated */
                status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) eses_enclosure, 
                                                                FBE_CLASS_ID_SAS_ENCLOSURE, 
                                                                expansion_port_index);
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "Creating a downstream enclosure,cable insert status:%d "
                        "Expansion port addr: 0x%llX.\n", 
                        connectorInserted, (unsigned long long)eses_enclosure->expansion_port_info[port_num].sas_address);
                /* clean up any leftover. */
                eses_enclosure->expansion_port_info[port_num].attached_encl_fw_activating = FALSE;

            } 
        }    

        /* Continue to set other discovery path attributes. 
         * But if the discovery edge is NULL, no need to continue.
         */
        if(discovery_edge == NULL)
        {
            // No need to continue.
            return FBE_ENCLOSURE_STATUS_OK;
        }

        status = fbe_base_discovering_get_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                    expansion_port_index, 
                                                    &curr_discovery_path_attrs);
        if(status != FBE_STATUS_OK)
        {
            // It should not happen.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "expansion_port_discovery_update, get_path_attr failed, "
                            "status: 0x%x.\n",  status);     

            return FBE_ENCLOSURE_STATUS_PATH_ATTR_UNAVAIL;
        }

        /* (2) The setting and clearing of the attribute FBE_DISCOVERY_PATH_ATTR_REMOVED on the discovery edge 
         * for the children enclosure is based on the parent enclosure connector inserted-ness.
         */   
        if(connectorInserted)
        {
            if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_REMOVED)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                    expansion_port_index,
                                                    FBE_DISCOVERY_PATH_ATTR_REMOVED);
            }
        }
        else 
        {
            /* The childen enclosure will be destroyed immediately if both FBE_DISCOVERY_PATH_ATTR_REMOVED and 
             * FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT attributes are set.
             * Note: The port sets the FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT attribute for the first enclosure 
             * in the discovery path but FBE_DISCOVERY_PATH_ATTR_REMOVED may not be set by the port while 
             * the enclosure is physically removed.
             * because the insert of downstream is based on OOB sequence detecting the LCC.  We've seen cases
             * where activating firmware causes upstream enclosure to think the cable is pull.  we need to
             * ignore this case.
             */
            if((!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_REMOVED)) &&
               (!eses_enclosure->expansion_port_info[port_num].attached_encl_fw_activating))
            {
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                    expansion_port_index,
                                                    FBE_DISCOVERY_PATH_ATTR_REMOVED);
            }
        }

        /* (3) The setting and clearing of the attribute FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT on the discovery edge 
         * is based on the children enclosure logged in/out.
         */    
        if(eses_enclosure->expansion_port_info[port_num].sas_address != FBE_SAS_ADDRESS_INVALID)
        {
            /* The enclosure is logged in. 
             * We have an enclosure and discovery edge. 
             * We clear the NOT_PRESENT discovery edge attribute in case it has been set.
             */
            if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                    expansion_port_index,
                                                    FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
            }
        } 
        else
        { 
            /* Enclosure is instantiated */
            /* Peter Puhov. The path state of the edge represents the surrogate state of the object.
                We should use NOT_PRESENT bit instead.
            */        
            
            if(!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT))
            {
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            expansion_port_index,
                                            FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
            }
        }

        if((!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_REMOVED) || 
            !(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)) &&
            (!connectorInserted) &&
            (eses_enclosure->expansion_port_info[port_num].sas_address == FBE_SAS_ADDRESS_INVALID) &&
            (!eses_enclosure->expansion_port_info[port_num].attached_encl_fw_activating))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_INFO,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                             "expansion_port_discovery_update, Downstream Enclosure Will Be Removed.\n");
        }

        /* (4) Check the bypass state */ 
        fbe_eses_enclosure_expansion_port_check_bypass_state(eses_enclosure, 
                            expansion_port_entire_connector_index, 
                            &discovery_path_bypass_attr_to_set);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "expansion_port_discovery_update, expansion_port_check_bypass_state failed, "
                        "encl_status: 0x%x.\n", encl_status);
        return encl_status;
    }

    if(discovery_path_bypass_attr_to_set == 0)
    {
        if(curr_discovery_path_attrs & (FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST |
                                       FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST))
        {
            /* Clear the bypass related attributes. */
            fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                                expansion_port_index, 
                                                (FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST | 
                                                FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST)); 
        }
    }
    else 
    {
        /* The discovery path attribute has not been set yet, so set it.
         * If this is USER_BYPASSED -> the unbypass will be automatically issued. 
         * If this is HARDWARE_BYPASSED -> the unbypass will NOT be automatically issued. 
         */
        if(!(curr_discovery_path_attrs & discovery_path_bypass_attr_to_set))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "expansion_port_discovery_update, new attr: 0x%x.\n",
                             discovery_path_bypass_attr_to_set);       
        
            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) eses_enclosure, 
                                            expansion_port_index, 
                                            discovery_path_bypass_attr_to_set); 
        }
    }

    return encl_status;

}// End of fbe_eses_enclosure_expansion_port_discovery_update

/*!*************************************************************************
* @fn fbe_eses_enclosure_expansion_port_check_bypass_state(
*                                  fbe_eses_enclosure_t * eses_enclosure, 
*                                  fbe_u8_t entire_connector_index, 
*                                  fbe_u32_t * discovery_path_attr_to_set_p)                  
***************************************************************************
* @brief
*   This function checks the expansion port bypass state and returns
*   the discovery path bypass related attribute to be set or cleared.
*       (1) At least one phy conneccted to the individual lane is enabled 
*           -> return 0 which means the bypass related attributes need to be cleared.
*       (2) Else if at least one phy connected to the individual lane is user bypassed 
*           -> return user bypassed attribute to be set.
*       (3) Else -> return the hardware bypass attribute to be set.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   entire_connector_index   - connector index in edal.
* @param   discovery_path_attr_to_set_p - The pointer to discovery path attribute.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   24-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_expansion_port_check_bypass_state(fbe_eses_enclosure_t * eses_enclosure, 
                                                     fbe_u8_t  expansion_port_entire_connector_index,
                                                     fbe_u32_t * discovery_path_attr_to_set_p)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                container_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                phy_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t              phy_disabled = FALSE;
    fbe_u8_t                phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID;

    /********
     * BEGIN
     ********/

    /* Initialize to FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST. */   
    *discovery_path_attr_to_set_p = FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST; 

    // Loop through all the connectors to find the individual lanes this entire connector contains.
    for(connector_index = 0; connector_index < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure); connector_index ++)
    {
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // attribute
                                                FBE_ENCL_CONNECTOR, // component type
                                                connector_index,  // Component index
                                                &container_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        if(container_index == expansion_port_entire_connector_index) 
        {
           /* This is the individual lane in the entire connector. 
            * Get the phy index for it.
            */
            encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_CONNECTOR_PHY_INDEX,  // attribute
                                                FBE_ENCL_CONNECTOR, // component type
                                                connector_index,  // Component index
                                                &phy_index);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            // Get the phy_disabled.
            encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EXP_PHY_DISABLED,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        &phy_disabled);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            if(phy_disabled)
            {
                // Get the phy_disable_reason.
                encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_EXP_PHY_DISABLE_REASON,  //Attribute
                                            FBE_ENCL_EXPANDER_PHY, // Component type
                                            phy_index, // Component index
                                            &phy_disable_reason);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }
            
                if(phy_disabled && (phy_disable_reason == FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST))
                {
                    /* Update to FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST, but need to 
                    * continue to see if there is any other phy which is enabled.
                    */
                    *discovery_path_attr_to_set_p = FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST;
                }
            }
            else
            {
                /* At least one phy is enabled.
                 * return 0 so that the bypassed related attributes will be cleared.
                 */
                *discovery_path_attr_to_set_p = 0; 
                break;
            }
        }// End of - if(container_index_i == expansion_port_entire_connector_index)         
    }// End of - for(i == 0; i < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure); i ++)

    return encl_status;
}// End of - fbe_eses_enclosure_expansion_port_check_bypass_state

/*!*************************************************************************
* @fn eses_enclosure_get_element_list_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition collects the children list
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry\n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    /* Call the executer function to do actual job */
    status = fbe_eses_enclosure_send_get_element_list_command(eses_enclosure, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                                 fbe_packet_t * packet,
*                                                  fbe_eses_ctrl_opcode_t opcode)     
***************************************************************************
* @brief
*       This function builds the control operation and sends down the SCSI command.
*
* @param      eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param      packet - The pointer to the fbe_packet_t.
* @param      opcode - The eses control opcode code.
*
* @return      fbe_status_t.
*
* NOTES
*
* HISTORY
*   12-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t
fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                  fbe_packet_t * packet,
                                                  fbe_eses_ctrl_opcode_t opcode)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /* Allocate a control operation. */
    payload = fbe_transport_get_payload_ex(packet);
    // The next operation is the control operation.
    control_operation = fbe_payload_ex_allocate_control_operation(payload);   

    if(control_operation == NULL)
    {    
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "opcode: 0x%x, build_control_op_send_scsi_cmd, allocate_control_operation failed.\n",
                         opcode);

        // Set the packet status to FBE_STATUS_GENERIC_FAILURE.
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    status = fbe_payload_control_build_operation(control_operation,  // control operation
                                    opcode,  // opcode
                                    NULL, // buffer
                                    0);   // buffer_length 

    /* Call the executer function to send the comand to get status. */
    status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);

    if (status != FBE_STATUS_OK) 
    {        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "build_control_op_send_scsi_cmd,opcode 0x%x,send_scsi_cmd failed,status 0x%x.\n",
                        opcode, status);
        
    }

    return status;
}// End of - fbe_eses_enclosure_alloc_control_op_send_scsi_cmd

/*!*************************************************************************
* @fn eses_enclosure_inquiry_data_not_exist_cond_function(
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs get Enclosure Serial Number from inquiry data
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   09-Sept-2008 NChiu - Created.
*   12-Dec-2008 PHE - Used the control operation.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_inquiry_data_not_exist_cond_function(fbe_base_object_t * p_object, 
                                                         fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_inquiry_data_not_exist_cond_completion, eses_enclosure);
    
    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA);
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!*************************************************************************
* @fn eses_enclosure_inquiry_data_not_exist_cond_completion(
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)                  
***************************************************************************
*
* @brief
*       This function handles the completion of the inquiry command.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The pointer to the context (eses_enclosure).
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   05-November-2008 bphilbin - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_inquiry_data_not_exist_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t   opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t   ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t    retry_status = FBE_CMD_NO_ACTION;

    /********
     * BEGIN
     ********/
    eses_enclosure = (fbe_eses_enclosure_t*)context;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );


    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        if(control_operation != NULL)
        { 
            // The control operation was allocated while sending down the command.
            fbe_payload_control_get_status(control_operation, &ctrl_status);
        
            if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
            {
                fbe_payload_control_get_opcode(control_operation, &opcode);
                fbe_payload_control_get_status_qualifier(control_operation, &ctrl_status_qualifier);
                retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, opcode, 
                    (fbe_enclosure_status_t) ctrl_status_qualifier);
                
                switch(retry_status)
                {
                    case FBE_CMD_NO_ACTION:
                    {     
                        /* 
                        * The command retry is not needed, clear the current condition.
                        * only clear the current condition when the command retry is not needed. 
                        * Otherwise, don't clear the current condition 
                        * so that the condition will be executed again.
                        */      
                        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

                        if (status != FBE_STATUS_OK) 
                        {        
                            
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't clear current condition,  status: 0x%X",
                                        __FUNCTION__, status);
                        }

                        /*
                        * If we are still an ESES enclosure at this time check the sas_enclosure_type
                        * to see if we can morph into one of the leaf classes.
                        * For now we will assume we are always an ESES enclosure.  In the future we may
                        * need this check.          
                        if(base_object->class_id == FBE_CLASS_ID_ESES_ENCLOSURE)
                        */
                        {        
                            switch(eses_enclosure->sas_enclosure_type)
                            {
                            case FBE_SAS_ENCLOSURE_TYPE_BULLET:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_BULLET_ENCLOSURE);
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_VIPER:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_VIPER_ENCLOSURE);  
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE);  
                                break;
                            case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE);  
                                break;
                            case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE);  
                                break;  
                            case FBE_SAS_ENCLOSURE_TYPE_TABASCO:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE);  
                                break;  
                                     
                            case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE);  
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_VOYAGER_EE_LCC);  
                                break;				
                                
                            case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE);  
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE);  
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_KNOT:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_KNOT_ENCLOSURE);  
                                break;
                            case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE);  
                                break;                                    
                            case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE);  
                                break;

                                
                            case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_RHEA:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_RHEA_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE);  
                                break;
                                
                            case FBE_SAS_ENCLOSURE_TYPE_VIKING:
                            case FBE_SAS_ENCLOSURE_TYPE_CAYENNE:                                
                            case FBE_SAS_ENCLOSURE_TYPE_NAGA:
                                /* But we don't know whether this is IOSXP or DRVSXP.
                                 * Set the condition to get the sas encl type from the 
                                 * configuration page primary subenclosure product id.
                                 */
                                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                                    (fbe_base_object_t*)eses_enclosure, 
                                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_SAS_ENCL_TYPE_UNKNOWN);
                                if (status != FBE_STATUS_OK) 
                                {
                                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                            FBE_TRACE_LEVEL_ERROR,
                                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                            "%s, can't set SAS_ENCL_TYPE_UNKNOWN condition, status: 0x%x.\n",
                                                            __FUNCTION__, status);
                                    
                                } 
                                break;

                            // fail the enclosure if we do not support it.
                            case FBE_SAS_ENCLOSURE_TYPE_INVALID:                
                            default:
                                {
                                    encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                                        &scsi_error_info);
                                    // update fault information and fail the enclosure
                                    encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                                    opcode,
                                                                    0,
                                                                    FBE_ENCL_FLTSYMPT_INVALID_ENCL_PLATFORM_TYPE,
                                                                    scsi_error_info.scsi_error_code);
                                    // trace out the error if we can't handle the error
                                    if(!ENCL_STAT_OK(encl_status))
                                    {
                                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n",
                                            __FUNCTION__, encl_status);
                                    }

                                    status = FBE_STATUS_GENERIC_FAILURE;
                                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                    "%s unsupported sas enclosure type: 0x%X",
                                                    __FUNCTION__, eses_enclosure->sas_enclosure_type);
                                    break;
                                } //default case
                            } //Switch case                      
                        } // ESES enclosure
                        break;
                    }// case FBE_CMD_NO_ACTION
                    case FBE_CMD_FAIL:
                    {
                        // get scsi error code
                        encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                            &scsi_error_info);
                        // update fault information and fail the enclosure
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                        opcode,
                                                        0,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(ctrl_status_qualifier),
                                                        scsi_error_info.scsi_error_code);
                        // trace out the error if we can't handle the error
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                            __FUNCTION__, encl_status);   
                            
                        }
                        break;
                    }// End of - case FBE_CMD_FAIL
                }// End of - switch(retry_status)

                /*
                 * We can add a case when we want to handle/limit number of retries
                 */

            }// End of - if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);

        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s get_control_operation failed.\n",
                             __FUNCTION__);
            
        }         
    } // OK status
    else
    {
        // status is not ok.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s status: 0x%x.\n",
                        __FUNCTION__, status);
        
        if(control_operation != NULL)
        { 
            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
    }

    return FBE_STATUS_OK;

}

/*!*************************************************************************
* @fn eses_enclosure_identity_not_validated_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs get Enclosure Serial Number from inquiry data
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   09-Sept-2008 NChiu - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_identity_not_validated_cond_function(fbe_base_object_t * p_object, 
                                                         fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY);

    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!*************************************************************************
* @fn eses_enclosure_status_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the status unknown condition.
*       It sets the completion routine and sends down the get status command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_status_unknown_cond_function(fbe_base_object_t * p_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn eses_enclosure_common_cond_completion(  
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)
***************************************************************************
*
* @brief
*       This routine can be used as the completion funciton for a condition
*       if it only needs to clear the current condition if it completes successfully.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The completion context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_common_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_operation_opcode_t   opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t   ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t    retry_status = FBE_CMD_NO_ACTION;

    /********
     * BEGIN
     ********/
    eses_enclosure = (fbe_eses_enclosure_t*)context;

    status = fbe_transport_get_status_code(packet);    

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if(control_operation != NULL)
    {
        fbe_payload_control_get_opcode(control_operation, &opcode);
        if(opcode == FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL)
        {
            eses_enclosure->emc_encl_ctrl_outstanding_write --;
        }
    }

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        if(control_operation != NULL)
        { 
            // The control operation was allocated while sending down the command.
            fbe_payload_control_get_status(control_operation, &ctrl_status);
        
            if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
            {
                fbe_payload_control_get_opcode(control_operation, &opcode);
                fbe_payload_control_get_status_qualifier(control_operation, &ctrl_status_qualifier);
                retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, opcode, 
                    (fbe_enclosure_status_t) ctrl_status_qualifier);
                
                switch (retry_status)
                {
                    case FBE_CMD_NO_ACTION:
                    {     
                        /* The command retry is not needed, clear the current condition.
                        * only clear the current condition when the command retry is not needed. 
                        * Otherwise, don't clear the current condition 
                        * so that the condition will be executed again.
                        */      
                        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

                        if (status != FBE_STATUS_OK) 
                        {        
                            
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't clear current condition,  status: 0x%X",
                                        __FUNCTION__,  status);
                        }
                    }// case FBE_CMD_NO_ACTION
                    break;
                    case FBE_CMD_FAIL:
                    {
                        // get scsi error code
                        encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                            &scsi_error_info);
                        // update fault information and fail the enclosure
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                        opcode,
                                                        0,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(ctrl_status_qualifier),
                                                        scsi_error_info.scsi_error_code);
                        // trace out the error if we can't handle the error
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                            __FUNCTION__, encl_status); 
                            
                        }
                    } // case FBE_CMD_FAIL
                    break;
                } // switch(retry_status)

                /*
                 * We can add case FBE_CMD_RETRY after we decide to handle/limit number of retries.
                 */

            }// End of - if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);

        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s get_control_operation failed.\n",
                            __FUNCTION__ );
            
        }         
    } // OK status
    else
    {
        // status is not ok.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s status: 0x%x.\n",
                        __FUNCTION__,  status);
        
        if(control_operation != NULL)
        { 
            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
    }

    return FBE_STATUS_OK;

}// End of - eses_enclosure_common_cond_completion

/*!*************************************************************************
 * @fn eses_enclosure_config_cond_completion(  
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)
 ***************************************************************************
 *
 * @brief
 *       This routine can be used as the completion funciton for the config page
 *       if it only needs to clear the current condition if it completes successfully.
 *
 * @param      packet - The pointer to the fbe_packet_t.
 * @param      context - The completion context.
 *
 * @return     fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   17-Nov-2009 AS - Created.
 ***************************************************************************/
static fbe_status_t 
eses_enclosure_config_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_operation_opcode_t   opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t   ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t    retry_status = FBE_CMD_NO_ACTION;

    /********
     * BEGIN
     ********/
    eses_enclosure = (fbe_eses_enclosure_t*)context;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );

    status = fbe_transport_get_status_code(packet);    

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        if(control_operation != NULL)
        { 
            // The control operation was allocated while sending down the command.
            fbe_payload_control_get_status(control_operation, &ctrl_status);
        
            if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
            {
                fbe_payload_control_get_opcode(control_operation, &opcode);
                fbe_payload_control_get_status_qualifier(control_operation, &ctrl_status_qualifier);
                retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, opcode, 
                    (fbe_enclosure_status_t) ctrl_status_qualifier);
                
                switch (retry_status)
                {
                    case FBE_CMD_NO_ACTION:
                    {
                        /* We know at this point the config page completed successfully. It is now time to 
                         * set the additional status and EMC specific page condition 
                         */
                        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                        &fbe_eses_enclosure_lifecycle_const,
                                        (fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_ADDL_STATUS_UNKNOWN);

                        if(!ENCL_STAT_OK(encl_status))            
                        {
                            /* Failed to set the emc specific status condition. */
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set Additional status unknown condition, status: 0x%x.\n",
                                                __FUNCTION__, encl_status);

                            encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                            /* If we fail to set the above condition, break and don't clear the current condition. 
                             * This will help us in easy debugging when we fail to set the condition. 
                             */
                            break;
                        }

                        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                        &fbe_eses_enclosure_lifecycle_const,
                                        (fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

                        if(!ENCL_STAT_OK(encl_status))            
                        {
                            /* Failed to set the status condition. */
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set Status unknown condition, status: 0x%x.\n",
                                                __FUNCTION__, encl_status);

                            encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                            /* If we fail to set the above condition, break and don't clear the current condition. 
                             * This will help us in easy debugging when we fail to set the condition. 
                             */
                            break;
                        }

                        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                        &fbe_eses_enclosure_lifecycle_const,
                                        (fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);

                        if(!ENCL_STAT_OK(encl_status))            
                        {
                            /* Failed to set the emc specific status condition. */
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set EMC specific status unknown condition, status: 0x%x.\n",
                                                __FUNCTION__, encl_status);

                            encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                            /* If we fail to set the above condition, break and don't clear the current condition. 
                             * This will help us in easy debugging when we fail to set the condition. 
                             */
                            break;
                        }

                        /* 
                         * The command retry is not needed, clear the current condition.
                         * only clear the current condition when the command retry is not needed. 
                         * Otherwise, don't clear the current condition 
                         * so that the condition will be executed again.
                         */      
                        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

                        if (status != FBE_STATUS_OK) 
                        {        
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s can't clear current condition,  status: 0x%X",
                                            __FUNCTION__, status);
                        }
                    }// case FBE_CMD_NO_ACTION
                    break;
                    case FBE_CMD_FAIL:
                    {
                        // get scsi error code
                        encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                            &scsi_error_info);

                        // update fault information and fail the enclosure
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                        opcode,
                                                        0,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(ctrl_status_qualifier),
                                                        scsi_error_info.scsi_error_code);

                        // trace out the error if we can't handle the error
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                            __FUNCTION__, encl_status); 
                            
                        }
                    } // case FBE_CMD_FAIL
                    break;
                } // switch(retry_status)

                /*
                 * We can add case FBE_CMD_RETRY after we decide to handle/limit number of retries.
                 */
            }// End of - if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s get_control_operation failed.\n",
                            __FUNCTION__ );
        }         
    } // OK status
    else
    {
        // status is not ok.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s status: 0x%x.\n",
                        __FUNCTION__,  status);
        
        if(control_operation != NULL)
        { 
            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
    }

    return FBE_STATUS_OK;

}// End of - eses_enclosure_config_cond_completion

/*!*************************************************************************
* @fn eses_enclosure_mode_select_cond_completion(  
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)
***************************************************************************
*
* @brief
*       This routine handles completion of mode select command.
* condition.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The completion context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   08-Jul-2000 Nayana Chaudhari - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_mode_select_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_operation_opcode_t   opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t   ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t    retry_status = FBE_CMD_NO_ACTION;

    /********
     * BEGIN
     ********/
    eses_enclosure = (fbe_eses_enclosure_t*)context;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );


    status = fbe_transport_get_status_code(packet);    

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        if(control_operation != NULL)
        { 
            // The control operation was allocated while sending down the command.
            fbe_payload_control_get_status(control_operation, &ctrl_status);
        
            if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
            {
                fbe_payload_control_get_opcode(control_operation, &opcode);
                fbe_payload_control_get_status_qualifier(control_operation, &ctrl_status_qualifier);
                retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, opcode, 
                    (fbe_enclosure_status_t) ctrl_status_qualifier);
                
                switch (retry_status)
                {
                    case FBE_CMD_NO_ACTION:
                    {     
                        /* 
                        * The command retry is not needed, clear the current condition.
                        * only clear the current condition when the command retry is not needed. 
                        * Otherwise, don't clear the current condition 
                        * so that the condition will be executed again.
                        */      
                        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

                        if (status != FBE_STATUS_OK) 
                        {                                   
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't clear current condition,  status: 0x%X",
                                        __FUNCTION__,  status);

                            // Should we fail the enclosure?
                        }
                        // Let's go back and verify that we are in the expected operating mode
                        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                                        (fbe_base_object_t*)eses_enclosure, 
                                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSENSED);
                        if (status != FBE_STATUS_OK) 
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "mode select completion, can't set MODE_UNSENSED condition, status: 0x%x.\n",
                                        status);
                    
                        }

                        // We send mode_sense after mode select only thrice. 
                        // If retries are exhausted, we don't try it anymore.
                        if(eses_enclosure->mode_sense_retry_cnt <
                            eses_enclosure->properties.number_of_command_retry_max)
                        {
                            eses_enclosure->mode_sense_retry_cnt++;

                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, mode sense rcnt:0x%x. set lc cond to FBE_ESES_UPDATE_COND_MODE_UNSENSED.\n", 
                                __FUNCTION__, eses_enclosure->mode_sense_retry_cnt); 
                        }
                        else
                        {
                            // set attribute indicating max count exceeded and command is not supported.
                            encl_status = fbe_base_enclosure_edal_setBool(
                                (fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ENCL_MODE_SENSE_UNSUPPORTED, // Attribute.
                                FBE_ENCL_ENCLOSURE, // component type.
                                0, // only 1 enclosure
                                TRUE); // The value to be set to.
                            if(!ENCL_STAT_OK(encl_status))
                            {
                                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, Can't set FBE_ENCL_MODE_SELECT_UNSUPPORTED. status: 0x%x\n", 
                                    __FUNCTION__, encl_status); 
                            
                            }
                            /* we don't want to stuck in activate state because of testmode bit.
                             * let's disable it.
                             */
                            if (eses_enclosure->test_mode_rqstd)
                            {
                                eses_enclosure->test_mode_rqstd = FALSE;
                                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, mode sense rcnt:0x%x. disable test_mode request.\n", 
                                    __FUNCTION__, eses_enclosure->mode_sense_retry_cnt); 
                            }
                        }
                        break;
                    }// case FBE_CMD_NO_ACTION
                    case FBE_CMD_FAIL:
                    {
                        // get scsi error code
                        encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                            &scsi_error_info);
                        // update fault information and fail the enclosure
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                        opcode,
                                                        0,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(ctrl_status_qualifier),
                                                        scsi_error_info.scsi_error_code);
                        // trace out the error if we can't handle the error
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                            __FUNCTION__, encl_status); 
                            
                        }
                        break;
                    } // case FBE_CMD_FAIL
                }// switch (retry_status)

                /*
                 * We can add case FBE_CMD_RETRY after we decide to handle/limit number of retries.
                 */

            }// End of - if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);

        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s get_control_operation failed.\n",
                            __FUNCTION__ );
            
        }         
    } // OK status
    else
    {
        // status is not ok.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s status: 0x%x.\n",
                        __FUNCTION__,  status);
        
        if(control_operation != NULL)
        { 
            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
    }

    return FBE_STATUS_OK;

}// End of - eses_enclosure_common_cond_completion
   
/*!*************************************************************************
* @fn eses_enclosure_additional_status_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the additional status unknown condition.
*       It sets the completion routine and sends down the get additional status command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   06-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_additional_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);
   
    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS); // opcode

    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!*************************************************************************
* eses_enclosure_emc_specific_status_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the EMC specific status unknown condition.
*       It sets the completion routine and sends down the get EMC specific status command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_emc_specific_status_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry\n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS); // opcode
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn eses_enclosure_configuration_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the configuration unknown condition.
*       It sets the completion routine and sends down the get configuration command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   28-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_configuration_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
    fbe_bool_t  active_edal_valid = 0;
    fbe_edal_status_t               edalStatus;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_DEBUG_HIGH,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "%s entry\n", __FUNCTION__ );

    /* clean up power cycle request previously set, this way, 
     * we can have the drive object to re-issue the command, 
     * and we don't need statistics page being preset condition
     */
    encl_status = fbe_eses_enclosure_ride_through_handle_slot_power_cycle(eses_enclosure);

    if(!ENCL_STAT_OK(encl_status))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                           "ride_through_handle_slot_power_cycle failed, encl_status 0x%x.\n", 
                            encl_status);
    }
    /* check if active_edal_valid is true, and copy the edal blocks over the backup */
    status = fbe_base_enclosure_get_activeEdalValid((fbe_base_enclosure_t *) eses_enclosure, &active_edal_valid);
    
    if(status == FBE_STATUS_OK && active_edal_valid == TRUE)
    {
         edalStatus = fbe_base_enclosure_copy_edal_to_backup((fbe_base_enclosure_t *) eses_enclosure);

         if(edalStatus == FBE_EDAL_STATUS_OK)
         {
             status = fbe_base_enclosure_set_activeEdalValid((fbe_base_enclosure_t *) eses_enclosure, FALSE);
         }
         else
         {
              fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                 "Fail copy edal to backup, status:0x%X blockptr:%p, backupptr:%p \n", 
                                                 edalStatus,
                                                 ((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock,
                                                 ((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock_backup);
         }
    }

    /* We will start mapping, update generation count of EDAL here. */
    fbe_base_enclosure_edal_incrementGenerationCount((fbe_base_enclosure_t *) eses_enclosure);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_config_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION); // opcode

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn eses_enclosure_need_to_notify_upstream(                  
*                    fbe_eses_enclosure_t * eses_enclosure)                  
***************************************************************************
*
* @brief
*       This function checks to see if we need to notify upstream.
*
* @param      eses_enclosure - The pointer to current enclosure.
*
* @return     fbe_bool_t.
*
* NOTES
*
* HISTORY
*   20-Nov-2009 NChiu - Created.
***************************************************************************/
static fbe_bool_t eses_enclosure_need_to_notify_upstream(fbe_eses_enclosure_t * eses_enclosure)
{
    /* we need to notify upstream if we aborted the operation, but we have informed upstream
     * that we are taking an action.
     */
    if (((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ABORT) ||
         (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_NONE)) &&
        (eses_enclosure->inform_fw_activating))
    {
        return (FBE_TRUE);
    }
    /* If we are going to activate new fw, and we haven't tell upstream, we need to do so. */
    if ((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE) &&
        (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS ) &&
        ((eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_MAIN) ||
         (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_EXPANDER) ||
         (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_FPGA) ||
         (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_INIT_STRING))&&
        (!eses_enclosure->inform_fw_activating))
    {
        return (FBE_TRUE);
    }
    /* if we have complete activation, and haven't tell upstream, we need to do so. */
    if ((((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE) &&
          (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE )) ||
         (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_NONE)) &&
        (eses_enclosure->inform_fw_activating))
    {
        return (FBE_TRUE);
    }

    return (FBE_FALSE);
}

/*!*************************************************************************
* @fn eses_enclosure_firmware_download_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the download condition. 
*       Operation details stored in the enclosure object are used to 
*       determine what action to take. This function will typically initiate 
*       a send or receive diagnostics operation and set the completion routine.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   17-Aug-2008 GB - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_firmware_download_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_lifecycle_status_t ret_status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
    fbe_payload_control_operation_opcode_t opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_eses_tunnel_fup_context_t *tunnel_fup_context_p = &(eses_enclosure->tunnel_fup_context_p);
    fbe_eses_tunnel_fup_schedule_op_t schedule_op;
    fbe_eses_tunnel_fup_event_t event;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "firmware_download  op %d, xferred %d, sts %d, addsts %d\n", 
                          eses_enclosure->enclCurrentFupInfo.enclFupOperation,
                          eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred,
                          eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                          eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus);

    /* check and see if we need to notify upstream object */
    if (eses_enclosure_need_to_notify_upstream(eses_enclosure))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "eses fw dl: notify upstream. op %d sts %d, addsts %d\n", 
                        eses_enclosure->enclCurrentFupInfo.enclFupOperation,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus);
        eses_enclosure->inform_fw_activating = !eses_enclosure->inform_fw_activating;
        fbe_eses_enclosure_notify_upstream_fw_activation(eses_enclosure, packet);

        return (FBE_LIFECYCLE_STATUS_PENDING);
    }

    if ((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ABORT) ||
        (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_NONE))
    {
        // should check if an op is in progress to determine it can really be aborted mid-command
        //  a) download in progress - requires sending a send_diag page, need to halt/prevent sending 
        //  more in-progress pages
        //  b) image downloaded, waiting activation - can send a page that will result in the 
        //  image being discarded - is this the right thing to do?
        //  c) activation in progress - nothing can be done

        if(eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ABORT) 
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_ABORT;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        }

        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
        
        /* Release the physically contiguous memory allocated to save the image.*/
        if(eses_enclosure->enclCurrentFupInfo.enclFupImagePointer != NULL) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s, releasing enclFupImagePointer %p.\n", __FUNCTION__,
                           eses_enclosure->enclCurrentFupInfo.enclFupImagePointer);

            fbe_memory_ex_release(eses_enclosure->enclCurrentFupInfo.enclFupImagePointer);
            eses_enclosure->enclCurrentFupInfo.enclFupImagePointer = NULL;
        }

        /* no more work to be done */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

        if (status != FBE_STATUS_OK) 
        {        
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);

        }
        ret_status = FBE_LIFECYCLE_STATUS_DONE;
    }
    else if (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD &&
             eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
    {
        fbe_eses_tunnel_fup_context_set_packet(tunnel_fup_context_p, packet); 
        event = fbe_eses_tunnel_fup_context_get_event(tunnel_fup_context_p);
        schedule_op = fbe_eses_tunnel_download_fsm(tunnel_fup_context_p, event);
        ret_status = fbe_eses_enclosure_handle_schedule_op(eses_enclosure, schedule_op);
    }
    else if (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE &&
             eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
    {
        fbe_eses_tunnel_fup_context_set_packet(tunnel_fup_context_p, packet);
        event = fbe_eses_tunnel_fup_context_get_event(tunnel_fup_context_p);
        schedule_op = fbe_eses_tunnel_activate_fsm(tunnel_fup_context_p, event);
        ret_status = fbe_eses_enclosure_handle_schedule_op(eses_enclosure, schedule_op);
    }
    else
    {
        status = fbe_transport_set_completion_function(packet,                                                        
                                                    eses_enclosure_firmware_download_cond_function_completion,
                                                    eses_enclosure);
        
        if (((eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS) ||
                (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL)) &&
                (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_REQ))
        {
            // Need to get the microcode status page from the expander. This could be due to an error returned
            // from a send diag page or it could be because we're polling to check activation complete.
            opcode = FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS;
        }
        else
        {
            // else it's a download or activate operation       
            // update the completion function and context on the packet stack
            opcode = FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE;        
        }

        fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                opcode);        

        ret_status = FBE_LIFECYCLE_STATUS_PENDING;
        
    }
    return (ret_status);  
}

/*!*************************************************************************
* @fn eses_enclosure_firmware_download_cond_function_completion(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*   The completion function for "firmware download cond function".
*   It clears the additional status unknown condition.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The completion context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   17-Aug-2008 GB - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_firmware_download_cond_function_completion(fbe_packet_t * packet, 
                                                          fbe_packet_completion_context_t context)
{
    fbe_status_t                    status;
    fbe_eses_enclosure_t            * eses_enclosure = NULL;
    fbe_payload_ex_t                   * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_enclosure_status_t          enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    eses_enclosure = (fbe_eses_enclosure_t*)context;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );


    status = fbe_transport_get_status_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL)
    {
        // Invalid payload opcode, the payload is corrupt, quit
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s  get_control_operation FAILED\n",
                           __FUNCTION__ );
        
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
        /* Set to FBE_ENCLOSURE_FIRMWARE_OP_NONE, and properly clean up before clearing the condition 
         * Do not set to FBE_ENCLOSURE_FIRMWARE_OP_ABORT because ABORT would be the op received from the client. 
         */
        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure,  
                                        0);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_status_qualifier(control_operation, (fbe_payload_control_status_qualifier_t *)&enclosure_status);

    if (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
    {
        if (eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == FALSE)
        {
            if(enclosure_status == FBE_ENCLOSURE_STATUS_OK)
            {
                // check if byte count is not increasing, this indicates
                // something is wrong and we could be stuck in a loop 
                // going through this path
                if (eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize == 0)
                {
                    eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent++;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s 0 Bytes Transferred, Iterations:%d\n", 
                                __FUNCTION__, 
                                eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent);
                }
                else
                {
                    eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent = 0;
                }
                if (eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent < 5)
                {
                    fbe_eses_enclosure_fup_check_for_more_work(eses_enclosure);
                }
            }
            else
            {
                // else check for busy 
                fbe_eses_enclosure_fup_check_for_resend(packet, eses_enclosure);
            }  
            // reschedule to get immediate attention.
            status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)eses_enclosure,  
                                            0);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s can't clear current condition,  status: 0x%X.\n",
                                   __FUNCTION__,  status);
                
            }
        }
        else
        {
            status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const,
                                              (fbe_base_object_t*)eses_enclosure,
                                              (fbe_lifecycle_timer_msec_t)0);
            if (status != FBE_STATUS_OK)
            {       
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "FBE FUP dl_condi_completion, tunnel download, fbe_lifecycle_reschedule failed, status: 0x%x.\n", status);
            }
        }
    } 
    else if (eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE)
    {
        // The event for tunnel firmware upgrade FSM has been set in fbe_eses_enclosure_send_scsi_cmd_completion
        // and will be handled when eses_enclosure_firmware_download_cond_function is reschedule to run. Nothing 
        // needs to be done here.
        if (eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == FALSE)
        {
            // handle the activate operation complete
            fbe_eses_enclosure_handle_activate_op(packet, eses_enclosure);
        }
    }
    else
    {
        // handle the abort operation complete
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s firmware download op %s completed.\n", 
                            __FUNCTION__, 
                            fbe_eses_enclosure_fw_op_to_text(eses_enclosure->enclCurrentFupInfo.enclFupOperation));
          
        if(eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ABORT) 
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_ABORT;
        }
        else
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
        }

        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;

        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;

        // this op is done
        /* reschedule to do properly clean up before clearing the condition */
        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure,  
                                        0);

        if (status != FBE_STATUS_OK) 
        {        
            
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s can't clear current condition, status: 0x%X.\n",
                                __FUNCTION__, status); 
            
        }
    }
    // done with the packet
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;

} // end eses_enclosure_firmware_download_cond_function_completion

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_activate_op()                  
***************************************************************************
* @brief
*   Check the firmware download operation status and additional status.
*   The additioanl status field will indicate if more status needs to be 
*   requested from the expander.
*
* @param   packet - pointer to the packet
* @param   eses_enclosure - pointer to the enclosure object.
*
* @return   fbe_status_t. 
*         Alway returns FBE_STATUS_OK
*
* NOTES
*
* HISTORY
*   23-Dec-2008 GB - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_handle_activate_op(fbe_packet_t * packet,
                                        fbe_eses_enclosure_t *eses_enclosure)
{
    fbe_status_t     status = FBE_STATUS_OK;

    // reschedule immediately if there was an error response from the image activate page
    if (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL)
    {
        if (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_REQ)
        {
            // reschedule to issue receive diag to get microcode status page
        }
        else
        {
            // faulted - done
            /* let's use abort code path instead of clearing the condition here.
             * This to make sure that we send proper notification to upstream object 
             * 
             /* Set to FBE_ENCLOSURE_FIRMWARE_OP_NONE, and properly clean up before clearing the condition 
             * Do not set to FBE_ENCLOSURE_FIRMWARE_OP_ABORT because ABORT would be the op received from the client. 
             */
            eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
        }
        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure,  
                                        0);
    } 
    else if ((eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS) &&
            ((eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_REQ) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN)))
        
    {
        // After sending the ACTIVATE the status/extstaus will be IN_PROGRESS/NONE. We come through here after
        // command complete and set extended status to STATUS_REQ. The next time through the download condition
        // IN_PROGRESS/STATUS_REQ will result in sending receive diag to get download status from the expander.
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_REQ;
    }
    // op status is either in-progress or none
    else  if (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus != FBE_ENCLOSURE_FW_EXT_STATUS_REQ)
    {
        // If activation complete has been determined, op staus == none AND ext status == none.
        // If download activation status is in progress, op status == in progress AND ext status == staus request.
        // If a download status page operation hit a problem, op status == in progress AND ext staus == appropriate status code

        // Something has happened to prevent getting the download status page, so 
        // clear the condition. There is no more work to do for getting activation status.
        // !! Clearing current condition here means we won't come back through the monitor in download
        // download/activate mode. FupStatus needs to reflect this!!
        /* let's use abort code path instead of clearing the condition here.
         * This to make sure that we send proper notification to upstream object 
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
        */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s:ACTIVATE COMPLETE opsts:0x%x,addsts:0x%x.\n",
                            __FUNCTION__, 
                           eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus, 
                           eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus);

        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;

        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, can't set DISCOVERY_UPDATE condition, status: 0x%x.\n",
                            __FUNCTION__,
                            status);
       
        }

        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure,  
                                        0);

    } 
    // For the case of activation in progress and (ext status == status request) allow 
    // to cycle through and run again at the next iteration to get download microcode status 
    // page and come back through here

    return(status);
} // end fbe_eses_enclosure_handle_activate_op

/*!*************************************************************************
* @fn fbe_eses_enclosure_fup_send_funtional_packet(fbe_eses_enclosure_t * eses_enclosure, 
*                                              fbe_packet_t * packet,
*                                              fbe_eses_ctrl_opcode_t opcode)     
***************************************************************************
* @brief
*       This function sets up the condition complete function and sends down the SCSI command.
*
* @param      eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param      packet - The pointer to the fbe_packet_t.
* @param      opcode - The eses control opcode code.
*
* @return      none.
*
* NOTES
*
* HISTORY
*   17-Mar-2011 Kenny Huang - Created.
***************************************************************************/
fbe_status_t
fbe_eses_enclosure_fup_send_funtional_packet(fbe_eses_enclosure_t *eses_enclosure, 
                                         fbe_packet_t *packet,   
                                         fbe_eses_ctrl_opcode_t opcode)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_transport_set_completion_function(packet,   
                                         eses_enclosure_firmware_download_cond_function_completion,
                                         eses_enclosure);

    status = fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure,
                                                               packet,
                                                               opcode);
    if (FBE_STATUS_OK != status)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "send scsi cmd error: opcode 0x%x status 0x%x.\n",
                                           opcode, status);
    }

    return status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_fup_check_for_more_work()                  
***************************************************************************
* @brief
*   Check the firmware download operation satus. If ok, check if more 
*   image bytes need to be downloaded.
*
* @param   eses_enclosure - The pointer to the enclosure object.
*
* @return   fbe_status_t. 
*         Alway returns FBE_STATUS_OK
*
* NOTES
*
* HISTORY
*   23-Dec-2008 GB - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_fup_check_for_more_work(fbe_eses_enclosure_t *eses_enclosure)

{
    fbe_status_t status = FBE_STATUS_OK;

    // Check if a send page failed (possible if we sent a receive diag
    // to get the dl status page). Check for failed download status..
    if (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL)
    {

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_WARNING,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s:Failed DL opsts:0x%x,addsts:0x%x",
                           __FUNCTION__,
                           eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus, 
                           eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus);

        // no more work, let the client decide to retry
        /* Set to FBE_ENCLOSURE_FIRMWARE_OP_NONE, and properly clean up before clearing the condition 
         * Do not set to FBE_ENCLOSURE_FIRMWARE_OP_ABORT because ABORT would be the op received from the client. 
         */
        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
    }
    else
    {
        // update xfer and check for more bytes
        fbe_eses_enclosure_fup_check_for_more_bytes(eses_enclosure);
    }

    return(status);
} // end fbe_eses_enclosure_fup_check_for_more_work

/*!*************************************************************************
* @fn fbe_eses_enclosure_fup_check_for_more_bytes()                  
***************************************************************************
* @brief
*   Update the bytes transferred and reset the transfer size. Check if 
*   all bytes have been sent. If done, clear the condition. Otherwise,
*   reschedule to execute immediately.
*
* @param   eses_enclosure - The pointer to the enclosure object.
*
* @return   fbe_status_t. 
*         Alway returns FBE_STATUS_OK
*
* NOTES
*
* HISTORY
*   23-Dec-2008 GB - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_fup_check_for_more_bytes(fbe_eses_enclosure_t *eses_enclosure)                                                        
{
    fbe_status_t status;

    /* we just completed some transfer */
    eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred += 
        eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize; 
    eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize = 0;

    /* check to see if we need more work */
    if (eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred >=
                    eses_enclosure->enclCurrentFupInfo.enclFupImageSize)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s download completed (%d bytes)\n", 
                             __FUNCTION__, eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred);

        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED;

        /* no more work to be done */
        /* reschedule to do properly clean up before clearing the condition */
        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
    }
    else
    {
        /* reschedule right away */
        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                         (fbe_base_object_t*)eses_enclosure,  
                                          0);
        //debug

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s bytes downloaded (%d bytes)\n", 
                           __FUNCTION__, eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred);

        
    }

    return FBE_STATUS_OK;
} // end fbe_eses_enclosure_fup_check_for_more_bytes

/*!*************************************************************************
* @fn fbe_eses_enclosure_fup_check_for_resend()                  
***************************************************************************
* @brief
*   Check if the command needs to be resent or the retry limit 
*   has been reached.
*
* @param   eses_enclosure - The pointer to the enclosure object.
*
* @return   fbe_status_t. 
*         Alway returns FBE_STATUS_OK
*
* NOTES
*
* HISTORY
*   23-Dec-2008 GB - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_fup_check_for_resend(fbe_packet_t * packet,
                                                    fbe_eses_enclosure_t *eses_enclosure)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_payload_ex_t           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_qualifier_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t                    retry_status = FBE_CMD_NO_ACTION;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s fwsts:0x%x, retries:%d\n", 
                        __FUNCTION__,                          
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt);  

    payload = fbe_transport_get_payload_ex(packet);

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);
    fbe_payload_control_get_status_qualifier(control_operation, &encl_status);
    retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, (fbe_eses_ctrl_opcode_t) opcode,
                                               (fbe_enclosure_status_t) encl_status);

    switch (retry_status)
    {
        case FBE_CMD_RETRY:
        {
            // Retry count is only bumped for enclosure busy. When the 
            // retry count is non-zero, a retry of the same page will be re-sent.
            eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt++;
            break;
        }
        case FBE_CMD_NO_ACTION:
            // this formerly would have been "no retry"
            // we can come through here when need to request ucode download status
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_WARNING,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s,NoRetry\n", 
                                    __FUNCTION__);

            /* Reset the enclFupOperationRetryCnt to 0 so that the first page can be retried if needed 
             * for the next round of download initiated by the client.
             */
            eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt = 0;
            if(opcode == FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS)
            {
                /* Set it to FBE_ENCLOSURE_FIRMWARE_OP_NONE so that the download op won't be retried
                 * Otherwise, after the non-first page failed, the get_download status was sent.
                 * After the get_download status failed, it retried to download the page. 
                 * This was not what we want. The expander does not accept the non-first page again.
                 * It has to start from the first page again. 
                 * So we just let the client to do the retry for non-first page download failure.
                 */
                eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
            }
            break;

        default:
            //case FBE_CMD_FAIL:
            // get scsi error code
            encl_error_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                &scsi_error_info);
            // update fault information, this call will FAIL the Enclosure
            encl_error_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                            (fbe_eses_ctrl_opcode_t)opcode,
                                            0,
                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom((fbe_enclosure_status_t) encl_status),
                                            scsi_error_info.scsi_error_code);
            // trace out the error if we can't handle the error
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s,fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);    
                
            }
            // enclosure is failed, make sure the download status indicates failed for CM
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
            break;
    } // switch retry_status

    if ((eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL) &&
        (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_REQ))
    {
        // Failure occurred, re-schedule in order to get immediate microcode download 
        // fault status from the expander.
        // The check for retry count could prevent getting status when on the last retry 
        // of a download or activate command. But, this is better than getting into a retry forever case.
        if (eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt <= 
            fbe_eses_enclosure_get_enclosure_fw_dl_retry_max(eses_enclosure))
        {
            status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)eses_enclosure,  
                                            0);
        }
        else
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_WARNING,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s stat_req fwsts/extsts:0x%x/0x%x, retries:%d\n", 
                        __FUNCTION__,                          
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt);  
        }
    }
    else if (eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt > 
             fbe_eses_enclosure_get_enclosure_fw_dl_retry_max(eses_enclosure))
    {
        // If all retries are done, clear the condition, done
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
        /* Set to FBE_ENCLOSURE_FIRMWARE_OP_NONE, and properly clean up before clearing the condition 
         * Do not set to FBE_ENCLOSURE_FIRMWARE_OP_ABORT because ABORT would be the op received from the client. 
         */
        eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
        status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure,  
                                        0);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_WARNING,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s exceed retry fwsts/extsts:0x%x/0x%x, retries:%d\n", 
                        __FUNCTION__,                          
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt);  
    }
    else if (eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt == 0)
    {
        // This is the case that the last page failed for a reason other than "busy".
        // Set the fup status to fail/status_req, which will cause a receive diag to 
        // be sent to get the microcode download status from the expander.

        if ((eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL) &&
            (eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_REQ))
        {
            // reschedule right away to get download status page
            status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                              (fbe_base_object_t*)eses_enclosure,  
                                              0);
        }
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_WARNING,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s retryCnt0 fwsts/extsts:0x%x/0x%x, retries:%d\n", 
                        __FUNCTION__,                          
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus,
                        eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt);      
    }

    // If none of the if-else cases above are entered, the page needs to 
    // be re-sent, but not immediately. Allow a cycle to expire in hopes the
    // problem will clear.

    return(status);
} // end fbe_eses_enclosure_fup_check_for_resend

/*!*************************************************************************
* @fn eses_enclosure_emc_specific_control_needed_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the EMC specific control needed condition.
*       It sends down the SCSI command to set the port use for the sas connectors and 
*       sync the expander time with the system time.
*       It also sets the completion routine. 
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   25-Sep-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_emc_specific_control_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_lifecycle_status_t rtn_satus;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry \n", __FUNCTION__ ); 

    if(fbe_eses_no_emc_encl_ctrl_write_outstanding(eses_enclosure))
    {
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, 
                                    eses_enclosure_common_cond_completion, 
                                    eses_enclosure);

        fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                    packet,               
                                                    FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL); // opcode

        eses_enclosure->emc_encl_ctrl_outstanding_write ++;

        rtn_satus = FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "EMC enclosure control page write outstanding, Send the write later.\n");

        status = FBE_STATUS_OK;
        /* Now we are done with the packet */    
        fbe_transport_set_status(packet, status, 0);
        eses_enclosure_common_cond_completion(packet, eses_enclosure);
        rtn_satus = FBE_LIFECYCLE_STATUS_DONE;
    }

    return rtn_satus;
}

/*!*************************************************************************
* @fn eses_enclosure_duplicate_ESN_unchecked_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function checks duplicate enclosure Serial Number on the chain towards SP.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   17-Oct-2008 NC - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_duplicate_ESN_unchecked_cond_function(fbe_base_object_t *p_object, fbe_packet_t *packet)
{
    fbe_status_t                status;
    fbe_enclosure_number_t      enclosure_number = 0;
    fbe_eses_enclosure_t   *eses_enclosure = (fbe_eses_enclosure_t *)p_object;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry\n", __FUNCTION__ );
   
    /* we have no other enclosure to check against */
    if (enclosure_number == 0)
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        /* Now we are done with the packet */    
        eses_enclosure_common_cond_completion(packet, eses_enclosure);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    /* Call the executer function to check for duplicated enclosure Serial Number. */
    status = fbe_eses_enclosure_check_dup_encl_SN(eses_enclosure, packet);

    if (status != FBE_STATUS_OK) 
    {        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s Check duplicate Enclosure UID failed, status: 0x%X",
                           __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn eses_enclosure_expander_control_needed_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function generates control request to the expander 
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_expander_control_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_edal_status_t edal_status;	
    fbe_lifecycle_status_t ret_status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
    fbe_enclosure_component_block_t   *eses_component_block = NULL;
    fbe_bool_t write_needed = FALSE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry\n", __FUNCTION__ );

    /*
     * Check to see if there were any changes in Enclosure Data
     */
    fbe_base_enclosure_get_component_block_ptr((fbe_base_enclosure_t *)eses_enclosure, 
                                                &eses_component_block);

    /* This function generates overallStatus for each component type */
    if (eses_component_block!=NULL)
    {
        edal_status = fbe_edal_checkForWriteData(eses_component_block, &write_needed);
        if (edal_status == FBE_EDAL_STATUS_OK)
        {
            status = FBE_STATUS_OK;
        }
    }

    /* we only allow one write outstanding at a time, this simplies access to edal to track 
     * which write request has completed. 
     */
    if ((write_needed==TRUE) && 
        (fbe_eses_no_write_outstanding(eses_enclosure)))
    {
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);
    
        fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                            packet, 
                                            FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL); // opcode

        ret_status = FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s, no need to write.\n", 
                            __FUNCTION__);

        status = FBE_STATUS_OK;
        /* Now we are done with the packet */    
        fbe_transport_set_status(packet, status, 0);
        eses_enclosure_common_cond_completion(packet, eses_enclosure);
        ret_status = FBE_LIFECYCLE_STATUS_DONE;
    }
    
    return ret_status;
}


/*!*************************************************************************
* @fn eses_enclosure_mode_unsensed_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the mode unsensed condition.
*       It sets the completion routine and sends the mode sense command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_mode_unsensed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry\n", __FUNCTION__ ); 

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, 
                                 eses_enclosure_common_cond_completion, 
                                 eses_enclosure);
 
    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                FBE_ESES_CTRL_OPCODE_MODE_SENSE); // opcode

    return FBE_LIFECYCLE_STATUS_PENDING;
}// End of function eses_enclosure_mode_unsensed_cond_function

/*!*************************************************************************
* @fn eses_enclosure_mode_unselected_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the mode unselected condition.
*       It sets the completion routine and sends the mode select command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_mode_unselected_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
  
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry\n", __FUNCTION__ ); 

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, 
                                 eses_enclosure_mode_select_cond_completion, 
                                 eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, 
                                                packet, 
                                                FBE_ESES_CTRL_OPCODE_MODE_SELECT); // opcode

    return FBE_LIFECYCLE_STATUS_PENDING;
}// End of function eses_enclosure_mode_unselected_cond_function


/*!*************************************************************************
* @fn eses_enclosure_init_ride_through_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition completes the initialization of eses enclosure object.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   28-Jul-2009 SachinD - Created.
*
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_init_ride_through_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "%s entry\n", __FUNCTION__ ); 

    // We need to clean up data 
    fbe_eses_enclosure_init_drive_info(eses_enclosure);

    fbe_eses_enclosure_init_expansion_port_info(eses_enclosure);

    // calling base class function
    base_enclosure_init_ride_through_cond_function(p_object, p_packet);
   
    return FBE_LIFECYCLE_STATUS_DONE;
} // End Of eses_enclosure_init_ride_through_cond_function

/*!*************************************************************************
* @fn eses_enclosure_emc_statistics_status_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the emc statistics status unknown condition.
*       It sets the completion routine and sends down the get status command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   02-Sep-2009 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_emc_statistics_status_unknown_cond_function(fbe_base_object_t * p_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );
    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!*************************************************************************
* @fn eses_enclosure_sas_encl_type_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the sas encl type unknown condition.
*       It sets the completion routine and sends down the command to get the sas encl type.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   23-Nov-2013 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_sas_encl_type_unknown_cond_function(fbe_base_object_t * p_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );
    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_sas_encl_type_unknown_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE);

    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!*************************************************************************
* @fn eses_enclosure_sas_encl_type_unknown_cond_completion(
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)                  
***************************************************************************
*
* @brief
*       This function handles the completion of the sas encl type unknown condition.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The pointer to the context (eses_enclosure).
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   23-Nov-2013 PHE - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_sas_encl_type_unknown_cond_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t   opcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t   ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t  scsi_error_info;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_cmd_retry_code_t    retry_status = FBE_CMD_NO_ACTION;

    /********
     * BEGIN
     ********/
    eses_enclosure = (fbe_eses_enclosure_t*)context;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry. \n", __FUNCTION__ );

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        if(control_operation != NULL)
        { 
            // The control operation was allocated while sending down the command.
            fbe_payload_control_get_status(control_operation, &ctrl_status);
        
            if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
            {
                fbe_payload_control_get_opcode(control_operation, &opcode);
                fbe_payload_control_get_status_qualifier(control_operation, &ctrl_status_qualifier);
                retry_status = fbe_eses_enclosure_is_cmd_retry_needed(eses_enclosure, opcode, 
                    (fbe_enclosure_status_t) ctrl_status_qualifier);
                
                switch(retry_status)
                {
                    case FBE_CMD_NO_ACTION:
                    {     
                        /* 
                        * The command retry is not needed, clear the current condition.
                        * only clear the current condition when the command retry is not needed. 
                        * Otherwise, don't clear the current condition 
                        * so that the condition will be executed again.
                        */      
                        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

                        if (status != FBE_STATUS_OK) 
                        {        
                            
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't clear current condition,  status: 0x%X",
                                        __FUNCTION__, status);
                        }

                        /*
                        * If we are still an ESES enclosure at this time check the sas_enclosure_type
                        * to see if we can morph into one of the leaf classes.
                        * For now we will assume we are always an ESES enclosure.  In the future we may
                        * need this check. 
                        */ 
                                
                        switch(eses_enclosure->sas_enclosure_type)
                        {
                            case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE);
                                break;
    
                            case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC);  
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE);
                                break;
    
                            case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC);  
                                break;
                            
                            case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE);
                                break;

                            case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:  
                                fbe_base_object_change_class_id((fbe_base_object_t*)eses_enclosure, FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC);  
                                break;

                            // fail the enclosure if we do not support it.
                            case FBE_SAS_ENCLOSURE_TYPE_INVALID:                
                            default:
                            {
                                encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                                    &scsi_error_info);
                                // update fault information and fail the enclosure
                                encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                                opcode,
                                                                0,
                                                                FBE_ENCL_FLTSYMPT_INVALID_ENCL_PLATFORM_TYPE,
                                                                scsi_error_info.scsi_error_code);
                                // trace out the error if we can't handle the error
                                if(!ENCL_STAT_OK(encl_status))
                                {
                                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n",
                                        __FUNCTION__, encl_status);
                                }

                                status = FBE_STATUS_GENERIC_FAILURE;
                                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s unsupported sas enclosure type: 0x%X",
                                                __FUNCTION__, eses_enclosure->sas_enclosure_type);
                                break;
                            } //default case
                        } //Switch case                      
           
                        break;
                    }// case FBE_CMD_NO_ACTION
                    case FBE_CMD_FAIL:
                    {
                        // get scsi error code
                        encl_status = fbe_base_enclosure_get_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                            &scsi_error_info);
                        // update fault information and fail the enclosure
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                                        opcode,
                                                        0,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(ctrl_status_qualifier),
                                                        scsi_error_info.scsi_error_code);
                        // trace out the error if we can't handle the error
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n", 
                                            __FUNCTION__, encl_status);   
                            
                        }
                        break;
                    }// End of - case FBE_CMD_FAIL
                }// End of - switch(retry_status)

                /*
                 * We can add a case when we want to handle/limit number of retries
                 */

            }// End of - if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);

        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s get_control_operation failed.\n",
                             __FUNCTION__);
            
        }         
    } // OK status
    else
    {
        // status is not ok.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s status: 0x%x.\n",
                        __FUNCTION__, status);
        
        if(control_operation != NULL)
        { 
            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, control_operation);
        }
    }

    return FBE_STATUS_OK;

}


/*!*************************************************************************
* @fn eses_enclosure_rp_size_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the resume prom size unknown condition.
*       It sets the completion routine and sends down the get status command.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   10-Dec-2013 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
eses_enclosure_rp_size_unknown_cond_function(fbe_base_object_t * p_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t*)p_object;
    fbe_bool_t specialFormat = FBE_FALSE;
    fbe_eses_enclosure_buf_info_t   bufInfo = {0};

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__ );

    specialFormat = fbe_eses_enclosure_is_ps_resume_format_special(eses_enclosure);

    if(!specialFormat) 
    {
        //Clear the current condition.
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
        if (status != FBE_STATUS_OK) {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s can't clear current condition, status: 0x%x.\n",
                            __FUNCTION__, status);
            
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_eses_enclosure_get_buf_id_with_unavail_buf_size(eses_enclosure, &bufInfo);

    if(status != FBE_STATUS_OK) 
    {
        /* 
         * Did not find the buffer with unavailable buffer size.
         * Clear the current condition.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);
        if (status != FBE_STATUS_OK) 
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s can't clear current condition, status: 0x%x.\n",
                                __FUNCTION__, status);
        }
    
        return FBE_LIFECYCLE_STATUS_DONE;
    }
        
    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, eses_enclosure_common_cond_completion, eses_enclosure);

    fbe_eses_enclosure_alloc_control_op_send_scsi_cmd(eses_enclosure, packet, FBE_ESES_CTRL_OPCODE_GET_RP_SIZE);
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}

// End of file
