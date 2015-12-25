/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Drive Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_drive_mgmt_monitor_entry "drive_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @version
 *   23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_drive_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"

/* Class Member Data */


fbe_drive_mgmt_drive_capacity_lookup_t fbe_drive_mgmt_known_drive_capacity[] =
{
    {(fbe_u8_t *) " CLAR01 ", 0x1ED066},
    {(fbe_u8_t *) " CLAR09 ", 0x1076D24},
    {(fbe_u8_t *) " CLAR18 ", 0x2141301},
    {(fbe_u8_t *) " CLAR36 ", 0x420C278},
    {(fbe_u8_t *) " CLAR72 ", 0x854709A},
    {(fbe_u8_t *) " CLAR181", 0x147F5BC4},
    {(fbe_u8_t *) " CLAR146", 0x10B5C730},
    {(fbe_u8_t *) " CLAR250", 0x1CC54E40},
    {(fbe_u8_t *) " CLAR300", 0x218CEECE},
    {(fbe_u8_t *) " CLAR320", 0x25214A80},
    {(fbe_u8_t *) " CLAR400", 0x2DD98780},
    {(fbe_u8_t *) " CLAR500", 0x395313DF},
    {(fbe_u8_t *) " CLAR750", 0x55FC751A},
    {(fbe_u8_t *) "CLAR1000", 0x72A5D655},
    {(fbe_u8_t *) "CLAR2000", 0xE54B5B42}
};


const fbe_u32_t fbe_drive_mgmt_known_drive_capacity_table_entries = (sizeof(fbe_drive_mgmt_known_drive_capacity)/   
                                                                     sizeof(fbe_drive_mgmt_known_drive_capacity[0]));




extern fbe_drive_mgmt_fuel_gauge_supported_pages_t dmo_fuel_gauge_supported_pages[];
extern fbe_u16_t DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT; 
/*!***************************************************************
 * fbe_drive_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the Drive Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t * drive_mgmt = NULL;

    drive_mgmt = (fbe_drive_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT,
                 FBE_TRACE_LEVEL_DEBUG_HIGH, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "DMO: %s entry\n", __FUNCTION__);

    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt),
                                      (fbe_base_object_t*)drive_mgmt, packet);
}
/******************************************
 * end fbe_drive_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_drive_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the drive management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the drive management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt));
}
/******************************************
 * end fbe_drive_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_drive_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_discover_drive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_drive_mgmt_dieh_init_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_drive_mgmt_fuel_gauge_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_fuel_gauge_read_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_fuel_gauge_write_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_drive_mgmt_get_drive_log_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_drive_mgmt_write_drive_log_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_drive_mgmt_delayed_state_change_callback_register_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_status_t fbe_drive_mgmt_handle_new_drive(fbe_drive_mgmt_t *drive_mgmt, fbe_object_id_t object_id);
static fbe_status_t fbe_drive_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);
static fbe_status_t fbe_drive_mgmt_cmi_message_handler(fbe_base_object_t *base_object, fbe_base_environment_cmi_message_info_t *cmi_message);
static fbe_status_t fbe_drive_mgmt_updateEnclFaultLed(fbe_drive_mgmt_t *drive_mgmt, 
                                                      fbe_object_id_t drive_object_id);
static fbe_status_t fbe_drive_mgmt_handle_object_lifecycle_change(fbe_drive_mgmt_t * pDriveMgmt, 
                                                                 update_object_msg_t *update_object_msg);
static fbe_status_t fbe_drive_mgmt_handle_object_data_change(fbe_drive_mgmt_t * pDriveMgmt, 
                                                             update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_drive_mgmt_handle_generic_info_change(fbe_drive_mgmt_t * pDriveMgmt,
                                                              update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_drive_mgmt_process_enclosure_status(fbe_drive_mgmt_t * pDriveMgmt,
                                                            fbe_device_physical_location_t * pLocation,
                                                            fbe_object_id_t parentObject);
static fbe_status_t fbe_drive_mgmt_process_drive_status(fbe_drive_mgmt_t * pDriveMgmt,
                                                        fbe_device_physical_location_t * pLocation,
                                                        fbe_object_id_t parentObject);

static fbe_status_t fbe_drive_mgmt_is_os_drive(fbe_drive_mgmt_t * pDriveMgmt, 
                                        fbe_device_physical_location_t * pDrivePhysicalLocation,
                                        fbe_bool_t * pIsOsDrive,
                                        fbe_bool_t * pIsPrimary);
static fbe_status_t fbe_drive_mgmt_start_os_drive_avail_check(fbe_drive_mgmt_t * pDriveMgmt,
                                     fbe_device_physical_location_t * pDrivePhysicalLocation,
                                     fbe_lifecycle_state_t lifecycleState);
static fbe_lifecycle_status_t fbe_drive_mgmt_check_os_drive_avail_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_update_os_drive_avail_flag(fbe_drive_mgmt_t * pDriveMgmt);
static fbe_status_t drive_mgmt_set_system_drive_normal_qdepth_global(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   queue_depth);
static fbe_status_t drive_mgmt_set_system_drive_reduced_qdepth_global(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   queue_depth);
static fbe_status_t drive_mgmt_get_system_drive_normal_qdepth(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   *queue_depth);
static fbe_status_t drive_mgmt_get_system_drive_reduced_qdepth(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   *queue_depth);
static fbe_drive_info_t * fbe_drive_mgmt_handle_null_drive_in_fail_state(fbe_drive_mgmt_t *drive_mgmt, fbe_object_id_t object_id);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(drive_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(drive_mgmt);

/*  drive_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(drive_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        drive_mgmt,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};


/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the FBE API. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_drive_mgmt_register_state_change_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
        fbe_drive_mgmt_register_state_change_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_drive_mgmt_register_cmi_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_drive_mgmt_register_cmi_callback_cond_function)
};



/*--- constant base condition entries --------------------------------------------------*/


/* FBE_DRIVE_MGMT_DISCOVER_DRIVES condition -
*   The purpose of this condition is start the discovery process
*   of a new drive that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_drive_mgmt_discover_drive_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_discover_drive_cond",
        FBE_DRIVE_MGMT_DISCOVER_DRIVE,
        fbe_drive_mgmt_discover_drive_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};



static fbe_lifecycle_const_base_cond_t 
    fbe_drive_mgmt_dieh_init_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_dieh_init_cond",
        FBE_DRIVE_MGMT_DIEH_INIT_COND,
        fbe_drive_mgmt_dieh_init_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


static fbe_lifecycle_const_base_timer_cond_t fbe_drive_mgmt_fuel_gauge_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_drive_mgmt_fuel_gauge_timer_cond",
            FBE_DRIVE_MGMT_FUEL_GAUGE_TIMER_COND,
            fbe_drive_mgmt_fuel_gauge_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,      /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_INVALID),   /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    3600*100 /* fires after 1 hours*/
};

static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_fuel_gauge_read_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_fuel_gauge_read_cond",
        FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND,
        fbe_drive_mgmt_fuel_gauge_read_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_fuel_gauge_write_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_fuel_gauge_write_cond",
        FBE_DRIVE_MGMT_FUEL_GAUGE_WRITE_COND,
        fbe_drive_mgmt_fuel_gauge_write_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};
    
    static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
            "fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond",
            FBE_DRIVE_MGMT_FUEL_GAUGE_UPDATE_PAGES_TBL_COND,
            fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,          /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    };

static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_get_drive_log_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_get_drive_log_cond",
        FBE_DRIVE_MGMT_GET_DRIVE_LOG_COND,
        fbe_drive_mgmt_get_drive_log_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_write_drive_log_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_write_drive_log_cond",
        FBE_DRIVE_MGMT_WRITE_DRIVE_LOG_COND,
        fbe_drive_mgmt_write_drive_log_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t fbe_drive_mgmt_delayed_state_change_callback_register_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_drive_mgmt_delayed_state_change_callback_register_cond",
        FBE_DRIVE_MGMT_DELAYED_STATE_CHANGE_CALLBACK_REGISTER_COND,
        fbe_drive_mgmt_delayed_state_change_callback_register_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

fbe_lifecycle_const_base_timer_cond_t fbe_drive_mgmt_check_os_drive_avail_cond = {
    {
    FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
        "fbe_drive_mgmt_check_os_drive_avail_cond",
        FBE_DRIVE_MGMT_CHECK_OS_DRIVE_AVAIL_COND,
        fbe_drive_mgmt_check_os_drive_avail_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    300 /* fires every 3 seconds */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(drive_mgmt)[] = {    
    &fbe_drive_mgmt_delayed_state_change_callback_register_cond,
    &fbe_drive_mgmt_discover_drive_cond,
    &fbe_drive_mgmt_dieh_init_cond,
    (fbe_lifecycle_const_base_cond_t *)&fbe_drive_mgmt_fuel_gauge_timer_cond,
    &fbe_drive_mgmt_fuel_gauge_read_cond,
    &fbe_drive_mgmt_fuel_gauge_write_cond,
    &fbe_drive_mgmt_get_drive_log_cond,
    &fbe_drive_mgmt_write_drive_log_cond,    
    &fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_drive_mgmt_check_os_drive_avail_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(drive_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_drive_mgmt_specialize_rotary[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_dieh_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_drive_mgmt_activate_rotary[] = {        
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_delayed_state_change_callback_register_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_discover_drive_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_check_os_drive_avail_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t fbe_drive_mgmt_ready_rotary[] = {   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_check_os_drive_avail_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_fuel_gauge_read_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_fuel_gauge_write_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_fuel_gauge_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_get_drive_log_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_drive_mgmt_write_drive_log_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(drive_mgmt)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE,  fbe_drive_mgmt_specialize_rotary),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE,    fbe_drive_mgmt_activate_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY,       fbe_drive_mgmt_ready_rotary)
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(drive_mgmt);

/*--- global drive mgmt lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(drive_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        drive_mgmt,
        FBE_CLASS_ID_DRIVE_MGMT,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_drive_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on drive object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the drive management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_drive_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,  
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);   

    /* Set delayed registration condition */
    status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                    (fbe_base_object_t*)drive_mgmt, 
                                    FBE_DRIVE_MGMT_DELAYED_STATE_CHANGE_CALLBACK_REGISTER_COND);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't set delayed registration condition, status: 0x%X\n",
                              __FUNCTION__, status);
    } 


    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_drive_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on drive object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the drive management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_drive_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    /* Register with the CMI for the conditions we care about.
     */
    status = fbe_base_environment_register_cmi_notification((fbe_base_environment_t *)base_object,
                                                            FBE_CMI_CLIENT_ID_DRIVE_MGMT,
                                                            fbe_drive_mgmt_cmi_message_handler);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s API callback init failed, status: 0x%X",
                                __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_drive_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_state_change_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the drive management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_drive_mgmt_t        *drive_mgmt = (fbe_drive_mgmt_t*)context;
    fbe_status_t            status = FBE_STATUS_OK;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,  
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);   
    
    /*
     * Process the notification
     */

    if (notification_info->notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {
        if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) 
        {
            status = fbe_drive_mgmt_handle_object_lifecycle_change(drive_mgmt, update_object_msg);
        }
        return status;
    }

    switch (notification_info->notification_type)
    { 
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            if((notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) ||
               (notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC) ||
               (notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE))               
            {
                status = fbe_drive_mgmt_handle_object_data_change(drive_mgmt, update_object_msg);
            }           
            break;
            
        default:
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: %s unhandled notification type %d.\n",
                                  __FUNCTION__, 
                                  (int)notification_info->notification_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
  

/*!**************************************************************
 * fbe_drive_mgmt_handle_object_lifecycle_change()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change
 *  notification.
 *  
 *
 * @param drive_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  14-Mar-2011:  PHE - Seperated from state change handler.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_handle_object_lifecycle_change(fbe_drive_mgmt_t * drive_mgmt, 
                                                                 update_object_msg_t *update_object_msg)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_base_object_t       *base_object = (fbe_base_object_t*)drive_mgmt;
    fbe_drive_info_t        *drive = NULL;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_status_t            phys_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t              checkEnclFaultLed = FALSE;
    fbe_bool_t              isOsDrive = FBE_FALSE;
    fbe_bool_t              drive_information_not_available = FBE_FALSE;
    fbe_device_physical_location_t phys_location = {0};
    fbe_lifecycle_state_t   state = FBE_LIFECYCLE_STATE_INVALID;

    /* init to invalid location so don't accidentially report wrong info if location is unavailable */
    phys_location.bus = FBE_INVALID_PORT_NUM;    
    phys_location.enclosure = FBE_INVALID_ENCL_NUM;
    phys_location.slot = FBE_INVALID_SLOT_NUM;
    
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);    
    
    if (notification_info->notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE )
    {

        status = fbe_notification_convert_notification_type_to_state(notification_info->notification_type, &state);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(base_object, 
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: %s, can't map: 0x%llX\n", __FUNCTION__, notification_info->notification_type);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_base_object_trace(base_object, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "DMO: %s. PDO state change. object_id=0x%X, state=0x%X\n",                      
                      __FUNCTION__, update_object_msg->object_id, state); 

        switch(state)
        {            
            case FBE_LIFECYCLE_STATE_READY:

               /* If drive doesn't exist in drive_info table then this is a new drive, so
                   do all necessary processing.
                */
                                
                dmo_drive_array_lock(drive_mgmt,__FUNCTION__); 

                drive = fbe_drive_mgmt_get_drive_p(drive_mgmt, update_object_msg->object_id);  
                
                
                if (drive == NULL)
                {
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);               
                    status = fbe_drive_mgmt_handle_new_drive(drive_mgmt, update_object_msg->object_id);

                     if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "DMO: %s Error handling new drive, status: 0x%X\n", __FUNCTION__, status);
                        break;
                    }
                }
                else
                {
                    drive->state = state; 
                    
                    drive->death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;                    
                    
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);               
                }
                checkEnclFaultLed = TRUE;

                /* Get the physical location and send as part of notification */
                phys_status = fbe_drive_mgmt_get_drive_phy_location_by_objectId(drive_mgmt, 
                                                                                update_object_msg->object_id,
                                                                                &phys_location); 
                break;
                
            case FBE_LIFECYCLE_STATE_FAIL:
                {
                    fbe_physical_drive_information_t drive_info = {0};
                    fbe_u32_t reason = DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE; 
                    fbe_status_t status; 
                    dmo_drive_death_action_t action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;
                   
                    dmo_physical_drive_information_init(&drive_info);

                    /* Drive failed, so update its state
                    */                
                    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
                    drive = fbe_drive_mgmt_get_drive_p(drive_mgmt, update_object_msg->object_id);
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                    
                    /* If drive is NULL (i.e. not found) maybe it failed before it reached ready state. Try to get the drive info from PDO itself*/                    
                    if (drive == NULL) 
                    {                        
                        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "DMO: %s drive oid=0x%x drive isn't in DMO, get it from PDO. \n", __FUNCTION__, update_object_msg->object_id);

                        /* Get the drive information from PDO and add it to DMO. */
                        drive = fbe_drive_mgmt_handle_null_drive_in_fail_state(drive_mgmt, update_object_msg->object_id);
                    }
                    
                    if (drive != NULL)
                    {
                        dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
                        drive->state = state;  
                        
                        /* make a copy of the drive info for logging if the data is not NULL. 
                        If data is NULL, will filling with "unknow". */
                        if ( *(drive->sn) != '\0')
                        {
                            fbe_copy_memory(drive_info.product_serial_num, drive->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
                            drive_info.product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0'; 
                        }
                        
                        if ( *(drive->tla) != '\0')
                        {
                            fbe_copy_memory(drive_info.tla_part_number, drive->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
                            drive_info.tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0';
                        }
                        
                        if ( *(drive->rev) != '\0')
                        {
                            fbe_copy_memory(drive_info.product_rev, drive->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
                            drive_info.product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0'; 
                        }
                        
                        phys_location = drive->location;
                        dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                    }
                    else
                    {
                        fbe_port_number_t port;
                        fbe_enclosure_number_t enclosure;
                        fbe_enclosure_slot_number_t slot;

                        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "DMO: %s drive pdo=0x%x isn't in DMO. \n", __FUNCTION__, update_object_msg->object_id);

                        /* attempt to get b_e_s and drive_info from PDO object ID. */
                        fbe_api_get_object_port_number (update_object_msg->object_id, &port);
                        fbe_api_get_object_enclosure_number(update_object_msg->object_id, &enclosure);
                        fbe_api_get_object_drive_number (update_object_msg->object_id, &slot);
                        phys_location.bus = (fbe_u8_t)port;
                        phys_location.enclosure = (fbe_u8_t)enclosure;
                        phys_location.slot = (fbe_u8_t)slot;

                        fbe_api_physical_drive_get_drive_information(update_object_msg->object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    }
                    
                    /* get the death reason from the object */
                    status = fbe_api_get_object_death_reason(update_object_msg->object_id, &reason, NULL, FBE_PACKAGE_ID_PHYSICAL);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "DMO: %s unable to get death reason for PDO object_id=0x%X\n", __FUNCTION__, update_object_msg->object_id);
                        reason = DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE;
                    }  
                       
                    if (drive != NULL) {
                        drive->death_reason = reason;                    
                    }
                    
                    action = fbe_drive_mgmt_drive_failed_action(drive_mgmt,reason);

                    fbe_drive_mgmt_log_drive_offline_event(drive_mgmt, action, 
                                                           &drive_info, reason,
                                                           &phys_location);
                }
                checkEnclFaultLed = TRUE;
                break;
                
            case FBE_LIFECYCLE_STATE_DESTROY: 
                {
                    fbe_physical_drive_information_t drive_info = {0};
                    fbe_u32_t reason;
                    fbe_status_t status;
                    dmo_drive_death_action_t action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;

                    dmo_physical_drive_information_init(&drive_info);
                    
                    /* Get the physical location before drive_info get destory */
                    phys_status = fbe_drive_mgmt_get_drive_phy_location_by_objectId(drive_mgmt,    
                                                                                    update_object_msg->object_id,
                                                                                    &phys_location); 
                    
                    /* Update drive state, and then update Enclosure fault LED before destroying object. */
                    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);  
                    drive = fbe_drive_mgmt_get_drive_p(drive_mgmt, update_object_msg->object_id);
                    if (drive != NULL)
                    {
                        drive->state = state; 
                        drive->logical_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;
                        drive->death_reason = DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED;                          
                    }
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                    status = fbe_drive_mgmt_updateEnclFaultLed(drive_mgmt, update_object_msg->object_id );
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace(base_object,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "DMO: %s, Encl %d_%d, error updating EnclFaultLed, status 0x%X.\n",
                                              __FUNCTION__,
                                              phys_location.bus, 
                                              phys_location.enclosure,
                                              status);
                    }
                    /* Drive was destroyed, so remove it from local data structure and clean up
                    */                
                    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);  
                    if (drive != NULL)
                    {
                        /* make a copy of the drive info for logging if the data is not NULL. */
                        if (*(drive->sn) != '\0')
                        {
                            fbe_copy_memory(drive_info.product_serial_num, drive->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
                            drive_info.product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0'; 
                        }
                        
                        if (*(drive->tla) != '\0')
                        {
                            fbe_copy_memory(drive_info.tla_part_number, drive->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
                            drive_info.tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0';
                        }
                        
                        if ( *(drive->rev) != '\0')
                        {
                            fbe_copy_memory(drive_info.product_rev, drive->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
                            drive_info.product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0'; 
                        }
                        fbe_drive_mgmt_init_drive(drive); 
                    }
                    else  /* drive not in local array,  must have been destroyed before reaching ready.*/
                    {
                        /* Since drive information is not available set a flag so that we output the proper event log
                         *  message below. */
                        drive_information_not_available = FBE_TRUE;
                        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "DMO: %s drive oid=0x%x destoryed which is not in drive array \n", __FUNCTION__, update_object_msg->object_id);
                    }
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                    
                    if (drive_information_not_available)
                    {
                        /* The drive was destroyed before becomming ready so drive information is not available
                         *  here or in PDO.  We will output an event log message indicating that the location is unknown.
                         */
                        fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE, NULL, 0, "%s %s %s %s %s %s %s", 
                                            "Drive in unknown location",  "\0", "\0", "\0", "\0", "\0", "\0");                          
                    }
                    else
                    {
                        reason = DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED;  

                        action = fbe_drive_mgmt_drive_failed_action(drive_mgmt,reason);

                        fbe_drive_mgmt_log_drive_offline_event(drive_mgmt, action, 
                                                               &drive_info, reason,
                                                               &phys_location);

                    }
                }
                break;
                
            default:
                /* Update drive lifecycle state */
                dmo_drive_array_lock(drive_mgmt,__FUNCTION__); 
                drive = fbe_drive_mgmt_get_drive_p(drive_mgmt, update_object_msg->object_id);  
                if (drive != NULL)
                {
                    drive->state = state;                     
                }
                dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);               
                break;            
        }

        if((state == FBE_LIFECYCLE_STATE_FAIL) || 
           (state == FBE_LIFECYCLE_STATE_DESTROY))
        {
            fbe_drive_mgmt_is_os_drive(drive_mgmt, &phys_location, &isOsDrive, NULL);
    
            if(isOsDrive) 
            {
                fbe_drive_mgmt_start_os_drive_avail_check(drive_mgmt, &phys_location, state);
            }
        }
    }

    if(phys_status == FBE_STATUS_OK)
    {
        /* Send notification */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)drive_mgmt,
                                                           FBE_CLASS_ID_DRIVE_MGMT,
                                                           FBE_DEVICE_TYPE_DRIVE,
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &phys_location);
    }
    
    /*
     * Check if Enclosure Fault LED needs updating
     */
    if (checkEnclFaultLed)
    {
        status = fbe_drive_mgmt_updateEnclFaultLed(drive_mgmt, update_object_msg->object_id );
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: Encl %d_%d, error updating EnclFaultLed, status 0x%X %s.\n",
                                  phys_location.bus, phys_location.enclosure, status, __FUNCTION__);
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pDriveMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  18-Mar-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_handle_object_data_change(fbe_drive_mgmt_t * pDriveMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);

    
    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry, data type %d.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_drive_mgmt_handle_generic_info_change(pDriveMgmt, pUpdateObjectMsg);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: %s unhandled data type %d.\n",
                                  __FUNCTION__, 
                                  pDataChangedInfo->data_type);
            break;
    }

    return status;
}// End of function fbe_drive_mgmt_handle_object_data_change


/*!**************************************************************
 * @fn fbe_drive_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param pDriveMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info.
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  18-Mar-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_handle_generic_info_change(fbe_drive_mgmt_t * pDriveMgmt,
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t   * pDataChangedInfo = NULL;
    fbe_device_physical_location_t         * pLocation = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_topology_object_type_t               object_type;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    pLocation = &(pDataChangedInfo->phys_location);
    object_type = (pUpdateObjectMsg->notification_info.object_type);
    
    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry, device type 0x%llx.\n",
                          __FUNCTION__, 
                          pDataChangedInfo->device_mask);

    switch (pDataChangedInfo->device_mask)
    {
        case FBE_DEVICE_TYPE_DRIVE:
            if(object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE)
            {
                status = fbe_drive_mgmt_process_drive_status(pDriveMgmt, pLocation, pUpdateObjectMsg->object_id);            
            }
            else if((object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) || (object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC))
            {
                status = fbe_drive_mgmt_process_enclosure_status(pDriveMgmt, pLocation, pUpdateObjectMsg->object_id);
            }
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DMO: %s Processing of %d_%d_%d failed, object_type = 0x%x status 0x%x.\n",
                                      __FUNCTION__,
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot,
                                      (unsigned int)object_type,
                                      status);
            }
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s Unhandled %d_%d_%d device type 0x%llx\n",                                  
                                  __FUNCTION__,
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot,
                                  pDataChangedInfo->device_mask);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    return status;
}// End of function fbe_drive_mgmt_handle_generic_info_change

/*!**************************************************************
 * fbe_drive_mgmt_process_enclosure_status()
 ****************************************************************
 * @brief
 *  This function processes the enclosure status change.
 *
 * @param pDriveMgmt - This is the object handle.
 * @param pLocation - The location of the enclosure.
 * @param parentObject - the object id of the parent device
 *
 * @return fbe_status_t - 
 *
 * @author
 *  18-Mar-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_process_enclosure_status(fbe_drive_mgmt_t * pDriveMgmt,
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_object_id_t parentObjectId)
{
    fbe_drive_info_t                          * pDriveInfo = NULL;
    fbe_enclosure_mgmt_get_drive_slot_info_t    getDriveSlotInfo = {0};
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u8_t                                    deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_object_id_t                             encl_object_id;
    fbe_enclosure_types_t                       enclosure_type;
    fbe_number_of_slots_t                       slot_per_bank = 0;

    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH ,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %d_%d_%d %s\n",
                          pLocation->bus,
                          pLocation->enclosure,
                          pLocation->slot,
                          __FUNCTION__);

    if(parentObjectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %d_%d_%d %s, Invalid (missing) parent object id\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_drive_mgmt_get_drive_info_pointer(pDriveMgmt, pLocation, &pDriveInfo);
    if (pDriveInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d %s, No Drive Info\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }
    // We should now have a valid pointer to the drive info. If there's no parent obj id
    // save the parent obj id and the location.
    if (pDriveInfo->parent_object_id == FBE_OBJECT_ID_INVALID)
    {
        pDriveInfo->parent_object_id = parentObjectId;
        pDriveInfo->location = *pLocation;
    }
    
    // copy the location into the local struct and send off to get the slot info
    getDriveSlotInfo.location = *pLocation;
    status = fbe_api_enclosure_get_drive_slot_info(parentObjectId, &getDriveSlotInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %d_%d_%d %s, Error getting drive slot info, status 0x%x.\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__,
                              status);
        return status;
    }

    slot_per_bank = 0;
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus, pLocation->enclosure, &encl_object_id);
    if (status == FBE_STATUS_OK) 
    {
        status = fbe_api_enclosure_get_encl_type(encl_object_id, &enclosure_type);
        if ((status == FBE_STATUS_OK) && 
            ((enclosure_type == FBE_ENCL_SAS_VOYAGER_ICM) ||
             (enclosure_type == FBE_ENCL_SAS_VIKING_IOSXP) ||
             (enclosure_type == FBE_ENCL_SAS_CAYENNE_IOSXP) ||
             (enclosure_type == FBE_ENCL_SAS_NAGA_IOSXP)))
        {
            status = fbe_api_enclosure_get_slot_count_per_bank(encl_object_id, &slot_per_bank);
        }
    }

    if (slot_per_bank != 0)
    {
        pLocation->bank = (pLocation->slot / slot_per_bank) + 'A';
        pLocation->bank_slot = pLocation->slot % slot_per_bank;
    }
    else 
    {
        pLocation->bank = '\0';
        pLocation->bank_slot = 0;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

    if(pDriveInfo->local_side_id != getDriveSlotInfo.localSideId ||
       pDriveInfo->bypassed != getDriveSlotInfo.bypassed ||
       pDriveInfo->self_bypassed != getDriveSlotInfo.selfBypassed ||
       pDriveInfo->poweredOff != getDriveSlotInfo.poweredOff)
    {
        pDriveInfo->local_side_id = getDriveSlotInfo.localSideId;
        pDriveInfo->bypassed = getDriveSlotInfo.bypassed;
        pDriveInfo->self_bypassed = getDriveSlotInfo.selfBypassed;

        if (pDriveInfo->poweredOff != getDriveSlotInfo.poweredOff)
        {
            if (getDriveSlotInfo.poweredOff == FBE_TRUE)
            {
                fbe_event_log_write(ESP_INFO_DRIVE_POWERED_OFF,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);
            }
            else
            {
                fbe_event_log_write(ESP_INFO_DRIVE_POWERED_ON,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);
            }

            pDriveInfo->poweredOff = getDriveSlotInfo.poweredOff;
        }

        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pDriveMgmt,
                                                               FBE_CLASS_ID_DRIVE_MGMT,
                                                               FBE_DEVICE_TYPE_DRIVE,
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               pLocation);
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_drive_mgmt_process_drive_status()
 ****************************************************************
 * @brief
 *  This function processes the drive status change.
 *
 * @param pDriveMgmt - This is the object handle.
 * @param pLocation - The location of the drive.
 * @param parentObject - the object id of the parent device
 *
 * @return fbe_status_t - 
 *
 * @author
 *  08/07/2012 kothal created
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_process_drive_status(fbe_drive_mgmt_t * pDriveMgmt,
                                    fbe_device_physical_location_t * pLocation,
                                    fbe_object_id_t parentObjectId)
{
    fbe_drive_info_t                          * pDriveInfo = NULL;    
    fbe_status_t                                status = FBE_STATUS_OK;    
    fbe_physical_drive_information_t            physical_drive_info;
    fbe_block_transport_logical_state_t         logical_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;    
    fbe_bool_t                                  notify_admin = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH ,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %d_%d_%d %s, entry\n",
                          pLocation->bus,
                          pLocation->enclosure,
                          pLocation->slot,
                          __FUNCTION__);

    if(parentObjectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %d_%d_%d %s, Invalid (missing) parent object id\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_drive_mgmt_get_drive_info_pointer(pDriveMgmt, pLocation, &pDriveInfo);
    if (pDriveInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d %s, No Drive Info\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }       
 
    status = fbe_api_physical_drive_get_drive_information(pDriveInfo->object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s Drive oid=0x%x, failed to get drive info, status 0x%x.\n",
                              __FUNCTION__, pDriveInfo->object_id, status);     
        return status;
    }

    /* Notify Admin if we've been notified by PDO that spinup count or spin_up_time has changed */
    if((pDriveInfo->spin_up_time_in_minutes!= physical_drive_info.spin_up_time_in_minutes) ||
        (pDriveInfo->stand_by_time_in_minutes != physical_drive_info.stand_by_time_in_minutes) ||
        (pDriveInfo->spin_up_count != physical_drive_info.spin_up_count))
    {
        dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);
        pDriveInfo->spin_up_count = physical_drive_info.spin_up_count;
        pDriveInfo->spin_up_time_in_minutes = physical_drive_info.spin_up_time_in_minutes;
        pDriveInfo->stand_by_time_in_minutes = physical_drive_info.stand_by_time_in_minutes;
        dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);

        notify_admin = FBE_TRUE;
    }

    /* Notify Admin if we've been notified by PDO that our drive's firmware has changed */
    if (!fbe_equal_memory(pDriveInfo->rev,        physical_drive_info.product_rev,          physical_drive_info.product_rev_len)      ||
        strncmp(pDriveInfo->tla,                  physical_drive_info.tla_part_number,      FBE_SCSI_INQUIRY_TLA_SIZE+1)         != 0 ||
        strncmp(pDriveInfo->dg_part_number_ascii, physical_drive_info.dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1) != 0 ||
        strncmp(pDriveInfo->product_id,           physical_drive_info.product_id,           FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1)  != 0 ||
        strncmp(pDriveInfo->vendor_id,            physical_drive_info.vendor_id,            FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1)   != 0
        )
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d Notify Admin FW changed [%s,%s,%s,%s,%s]\n",
                              pLocation->bus, pLocation->enclosure, pLocation->slot,
                              physical_drive_info.tla_part_number,
                              physical_drive_info.product_rev,
                              physical_drive_info.dg_part_number_ascii,
                              physical_drive_info.product_id,
                              physical_drive_info.vendor_id);

        dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);
        fbe_copy_memory(pDriveInfo->rev, physical_drive_info.product_rev, physical_drive_info.product_rev_len);
        pDriveInfo->rev[physical_drive_info.product_rev_len] = '\0';
        fbe_copy_memory(pDriveInfo->vendor_id, physical_drive_info.vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1);
        fbe_copy_memory(pDriveInfo->product_id, physical_drive_info.product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1);
        fbe_copy_memory(pDriveInfo->tla, physical_drive_info.tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE+1);
        fbe_copy_memory(pDriveInfo->sn, physical_drive_info.product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
        dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);

        notify_admin = FBE_TRUE;
    }

    if(notify_admin == FBE_TRUE)
    {
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pDriveMgmt,
                                                   FBE_CLASS_ID_DRIVE_MGMT,
                                                   FBE_DEVICE_TYPE_DRIVE,
                                                   FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                   pLocation);
    }

    status = fbe_api_physical_drive_get_logical_drive_state(pDriveInfo->object_id, &logical_state);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: Error getting logical state for %d_%d_%d pdo(0x%x), status 0x%x.\n",                              
                              pLocation->bus, pLocation->enclosure, pLocation->slot, pDriveInfo->object_id, status);
        return status;
        
    }
    else
    {
    
        if(pDriveInfo->logical_state != logical_state)
        {
            char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
            fbe_block_transport_logical_state_t prev_logical_state = pDriveInfo->logical_state;

            pDriveInfo->logical_state = logical_state;
            
            /* generate device prefix string (used for tracing & events) */
            pLocation->bank = pDriveInfo->location.bank;
            pLocation->bank_slot = pDriveInfo->location.bank_slot;

            /* if logically offline */
            if(logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520 ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK ||
               logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER)
            {                
                fbe_u32_t reason = fbe_drive_mgmt_get_logical_offline_reason(pDriveMgmt, logical_state);
                dmo_drive_death_action_t action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;
                                    
                pDriveInfo->death_reason = reason;
                
                fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DMO: %d_%d_%d Drive Fault Led On logical_state:%d->%d , PDO Obj: 0x%x\n", 
                                      pLocation->bus, pLocation->enclosure, pLocation->slot, prev_logical_state, logical_state, pDriveInfo->object_id);  
                                        
                status = fbe_api_enclosure_update_drive_fault_led(pLocation->bus, 
                         pLocation->enclosure,
                         pLocation->slot,
                         FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON);
                
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,  
                                          "DMO: %d_%d_%d Failed to turn on the drive led, status 0x%x  %s.\n",
                                          pLocation->bus, pLocation->enclosure, pLocation->slot, status,  __FUNCTION__); 
                }

                /* Log an event for everything except DriveFault and LinkFault since this will be handled during lifecycle state change */                
                if (logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT &&
                    logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LINK_FAULT)
                {

                    action = fbe_drive_mgmt_drive_failed_action(pDriveMgmt,reason);

                    fbe_drive_mgmt_log_drive_offline_event(pDriveMgmt, action, 
                                                           &physical_drive_info, reason,
                                                           pLocation);
                }
            }
            /* else if logically online */
            else if (logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED ||
                     logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE)
            {

                pDriveInfo->death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;            
                
                fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DMO: %d_%d_%d Drive Fault Led Off logical_state:%d->%d , PDO Obj: 0x%x,\n",  
                                      pLocation->bus, pLocation->enclosure, pLocation->slot, prev_logical_state, logical_state, pDriveInfo->object_id);  
                                        
                status = fbe_api_enclosure_update_drive_fault_led(pLocation->bus, 
                         pLocation->enclosure,
                         pLocation->slot,
                         FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF); 
                
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,  
                                          "DMO: %d_%d_%d Failed to turn off the drive led, status 0x%x %s.\n",
                                          pLocation->bus, pLocation->enclosure, pLocation->slot, status,__FUNCTION__); 
                }

                /* If going from logicially offline to logically online, then generate another "Came Online" eventlog.*/
                if (prev_logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED &&
                    prev_logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE)
                {

                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
                             pLocation, 
                             &deviceStr[0],
                             FBE_DEVICE_STRING_LENGTH);

                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "DMO: %d_%d_%d Failed to create device string for Drive, status 0x%x %s.\n",   
                                              pLocation->bus, pLocation->enclosure, pLocation->slot, status, __FUNCTION__);          
                    }
                
                    fbe_event_log_write(ESP_INFO_DRIVE_ONLINE, NULL, 0, "%s %s %s %s", 
                                        &deviceStr[0], physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev);
                }
            }
            /* else unkown state*/
            else 
            {

                pDriveInfo->death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;            
                
                fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DMO: %d_%d_%d Logically Offline unknown state. PDO Obj:0x%x, logical_state:%d->%d\n", 
                                      pLocation->bus, pLocation->enclosure, pLocation->slot,
                                      pDriveInfo->object_id, prev_logical_state, logical_state);
                return FBE_STATUS_OK;
            }
            
            /* Update enclosure led. */
            status = fbe_drive_mgmt_updateEnclFaultLed(pDriveMgmt, pDriveInfo->object_id );
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DMO: Encl %d_%d, error updating EnclFaultLed, status 0x%X  %s.\n",
                                      pLocation->bus, pLocation->enclosure, status, __FUNCTION__);
            }
        }
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_start_os_drive_avail_check()
 ****************************************************************
 * @brief
 *  This function starts to check the OS drive availability by setting 
 *  the condition.
 * 
 * @param pDriveMgmt - This is the object handle.
 * @param pDrivePhysicalLocation - The physical location of the drive.
 * @param lifecycleState -
 *
 * @return fbe_status_t  - 
 *
 * @author
 *  26-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_start_os_drive_avail_check(fbe_drive_mgmt_t * pDriveMgmt,
                                     fbe_device_physical_location_t * pDrivePhysicalLocation,
                                     fbe_lifecycle_state_t lifecycleState)
{
    fbe_status_t           status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d CHECK_OS_DRIVE_AVAIL start! lifecycleState %d.\n",
                              pDrivePhysicalLocation->bus,
                              pDrivePhysicalLocation->enclosure,
                              pDrivePhysicalLocation->slot,
                              lifecycleState);

    status = fbe_lifecycle_set_cond(&fbe_drive_mgmt_lifecycle_const, 
                                    (fbe_base_object_t*)pDriveMgmt, 
                                    FBE_DRIVE_MGMT_CHECK_OS_DRIVE_AVAIL_COND);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: CHECK_OS_DRIVE_AVAIL can't set CHECK_OS_DRIVE_AVAIL condition, status: 0x%X\n",
                              status);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_is_os_drive()
 ****************************************************************
 * @brief
 *  This function checks whether the drive is the OS drive.
 * 
 * @param pDriveMgmt - This is the object handle.
 * @param pDrivePhysicalLocation - The pointer to the physical location of the drive.
 * @param pIsOsDrive(OUTPUT) - The pointer to the value whether this is OS drive or not.
 *
 * @return fbe_status_t  - Always return FBE_STATUS_OK.
 *
 * @author
 *  26-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_is_os_drive(fbe_drive_mgmt_t * pDriveMgmt, 
                                        fbe_device_physical_location_t * pDrivePhysicalLocation,
                                        fbe_bool_t * pIsOsDrive,
                                        fbe_bool_t * pIsPrimary)
{
    SP_ID                              localSp = SP_ID_MAX;
    fbe_device_physical_location_t     enclLocationForOsDrives = {0};

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *) pDriveMgmt, &localSp);
    fbe_base_env_get_encl_location_for_os_drives((fbe_base_environment_t *) pDriveMgmt, &enclLocationForOsDrives); 

    if((localSp == SP_A) &&
       (pDrivePhysicalLocation->bus == enclLocationForOsDrives.bus) && 
       (pDrivePhysicalLocation->enclosure == enclLocationForOsDrives.enclosure) &&
       ((pDrivePhysicalLocation->slot == 0) ||
        (pDrivePhysicalLocation->slot == 2)))
    {
        *pIsOsDrive = FBE_TRUE;
        if (pIsPrimary != NULL){
            *pIsPrimary = ((pDrivePhysicalLocation->slot == 0) ? FBE_TRUE : FBE_FALSE);
        }
    }
    else if((localSp == SP_B) &&
            (pDrivePhysicalLocation->bus == enclLocationForOsDrives.bus) && 
            (pDrivePhysicalLocation->enclosure == enclLocationForOsDrives.enclosure) &&
            ((pDrivePhysicalLocation->slot == 1) ||
             (pDrivePhysicalLocation->slot == 3)))
    {
        *pIsOsDrive = FBE_TRUE;
        if (pIsPrimary != NULL){
            *pIsPrimary = ((pDrivePhysicalLocation->slot == 1) ? FBE_TRUE : FBE_FALSE);
        }

    }
    else
    {
        *pIsOsDrive = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_check_os_drive_avail_cond_function()
 ****************************************************************
 * @brief
 *  This function checks whether the OS drive is available.
 *  If any OS drive is available, stop the timer to panic the SP.
 *  If there is no OS drive available for a period of time, panic the SP.
 * 
 * @param base_object - This is the object handle, or in our
 * case the base_environment.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  26-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_drive_mgmt_check_os_drive_avail_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t                  * pDriveMgmt = (fbe_drive_mgmt_t *)base_object;
    fbe_u32_t                           elapsedTimeInSec = 0;
    fbe_status_t                        status = FBE_STATUS_OK; 

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: CHECK_OS_DRIVE_AVAIL entry.\n");

    fbe_drive_mgmt_update_os_drive_avail_flag(pDriveMgmt);

    if(pDriveMgmt->osDriveAvail) 
    {
        /* OS drive is availabe. 
         * Reset startTimeForOsDriveUnavail to so that it can be used in the future. 
         */
        pDriveMgmt->startTimeForOsDriveUnavail = 0;

        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: CHECK_OS_DRIVE_AVAIL stop!\n");

        status = fbe_lifecycle_stop_timer(&fbe_drive_mgmt_lifecycle_const, 
                                          base_object,
                                          FBE_DRIVE_MGMT_CHECK_OS_DRIVE_AVAIL_COND);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: CHECK_OS_DRIVE_AVAIL can't stop timer, status: 0x%X\n",
                                  status);

        }
    }
    else
    {
        /* OS drive is unavailabe. 
         */
        if(pDriveMgmt->startTimeForOsDriveUnavail == 0)
        {
            fbe_base_object_trace(base_object, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "DMO: CHECK_OS_DRIVE_AVAIL, NO OS Drives avail, start ride through.\n"); 

            pDriveMgmt->startTimeForOsDriveUnavail = fbe_get_time();

            /* Clear the current condition to restart the timer. */
            status = fbe_lifecycle_clear_current_cond(base_object);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "DMO: CHECK_OS_DRIVE_AVAIL can't clear current condition, status: 0x%X\n",
                                        status);
            }
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(pDriveMgmt->startTimeForOsDriveUnavail);
            if(elapsedTimeInSec >= pDriveMgmt->osDriveUnavailRideThroughInSec) 
            {
                fbe_base_object_trace(base_object, 
                           FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "DMO: CHECK_OS_DRIVE_AVAIL, NO OS Drives avail for %d sec(limit %d sec), PANIC SP!!!\n",
                           elapsedTimeInSec,
                           pDriveMgmt->osDriveUnavailRideThroughInSec); 
            }
        }
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_update_os_drive_avail_flag()
 ****************************************************************
 * @brief
 *  The function checks whether there is any OS drive available for this SP
 *  and update the flag.

 * @param pDriveMgmt - 
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @author
 *  26-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_update_os_drive_avail_flag(fbe_drive_mgmt_t * pDriveMgmt)
{
    fbe_u32_t                           driveIndex = 0;
    fbe_bool_t                          isOsDrive = FBE_FALSE;
    fbe_drive_info_t                  * pDriveInfo = NULL;

    /* Init to FBE_FALSE */
    pDriveMgmt->osDriveAvail = FBE_FALSE;

    pDriveInfo = pDriveMgmt->drive_info_array;
    for (driveIndex = 0; driveIndex < pDriveMgmt->max_supported_drives; driveIndex++)
    {
        if(pDriveInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            fbe_drive_mgmt_is_os_drive(pDriveMgmt, &pDriveInfo->location, &isOsDrive, NULL);
    
            if(isOsDrive) 
            {
                /* 
                 * I was worried about the case that the drive went to FAIL first
                 * and then PENDING DESTROY which could stop the os drive unavail check.
                 * However, DMO does not register for the pending state.
                 * It means we should not see the PENDING_DESTROY. So we are safe there. - PINGHE
                 */
                if((pDriveInfo->state != FBE_LIFECYCLE_STATE_FAIL) &&        
                   (pDriveInfo->state != FBE_LIFECYCLE_STATE_DESTROY))
                {
                    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "DMO: %d_%d_%d is avail, CHECK_OS_DRIVE_AVAIL, lifecycleState %d.\n",
                           pDriveInfo->location.bus,
                           pDriveInfo->location.enclosure,
                           pDriveInfo->location.slot,
                           pDriveInfo->state); 
    
                    pDriveMgmt->osDriveAvail = FBE_TRUE;
                    break;
                }
            }
        }

        pDriveInfo++;
    }

    return FBE_STATUS_OK;
}
   
/*!**************************************************************
 * fbe_drive_mgmt_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for CMI message notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the drive management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_cmi_message_handler(fbe_base_object_t *base_object, 
                                   fbe_base_environment_cmi_message_info_t *cmi_message)
{
    fbe_drive_mgmt_cmi_msg_t *    user_msg;
    fbe_drive_mgmt_t *      drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t            status = FBE_STATUS_OK;

    user_msg = (fbe_drive_mgmt_cmi_msg_t *)cmi_message->user_message;

    fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "DMO: cmi_msg_handler receive cmi_msg %p, user_msg %p event 0x%x\n",
                       cmi_message, user_msg, cmi_message->event);

    switch (cmi_message->event)
    {
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                      "DMO: cmi_msg_handler msg %d\n",
                       user_msg->type);        
        fbe_drive_mgmt_process_cmi_message(drive_mgmt, user_msg);
        break;    

    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "DMO: cmi_msg_handler msg %d\n",
                       user_msg->type);
        break;

    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "DMO: cmi_msg_handler Err send CMI msg %d,(unable to contact Peer SP),event %d\n",
                       user_msg->type, cmi_message->event);

        status = fbe_drive_mgmt_process_cmi_peer_not_present(drive_mgmt, user_msg);
        break;   

    case FBE_CMI_EVENT_SP_CONTACT_LOST:   // peer is dead
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "DMO: cmi_msg_handler Lost contact with SP\n");
        /* Nothing to do.  Relying on PEER_NOT_PRESENT error when sending a message to determine that
           peer as gone away.
         */
        break;

    case FBE_CMI_EVENT_PEER_BUSY:
        fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_LEVEL_INFO,
                                  "DMO: cmi_msg_handler FBE_CMI_EVENT_PEER_BUSY\n");
        /* Handle this with a retry, it means the peer is there but did not have time to register with the CMI on the other side.
           Retry is handled automatically.  Currently the only event which could get this is 
           FBE_DRIVE_MGMT_CMI_MSG_CHECK_PEER_NTMIRROR and it's controlled by
           a timer condition, which will resend the next time it wakes up. */
        break;


    case FBE_CMI_EVENT_CLOSE_COMPLETED:
    case FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED:
    case FBE_CMI_EVENT_FATAL_ERROR:
    case FBE_CMI_EVENT_INVALID:
    default:
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: cmi_msg_handler Error sending CMI msg %d, event %d\n",
                               user_msg->type, cmi_message->event);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        // handle the message in base env
        status = fbe_base_environment_cmi_message_handler(base_object, cmi_message);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to handle CMI msg.\n",
                               __FUNCTION__);
        }
    }

    return status;
}
/*******************************************************************
 * end fbe_drive_mgmt_cmi_message_handler() 
 *******************************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_discover_drive_cond_function()
 ****************************************************************
 * @brief
 *  This function gets the list of all the new drive that was
 *  added and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the drive management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_drive_mgmt_discover_drive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_u32_t drive_count = 0, i;
    fbe_status_t status;
    fbe_drive_mgmt_t *drive_mgmt = (fbe_drive_mgmt_t *) base_object;
    fbe_device_physical_location_t phys_location = {0};
    fbe_object_id_t              * object_id_list = NULL;
    fbe_lifecycle_state_t         current_state = FBE_LIFECYCLE_STATE_INVALID;
    
    object_id_list = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);

    if(object_id_list == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Memory Allocation failed\n",
                              __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_set_memory(object_id_list, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);

    /* Get the list of all the drives the system sees*/
    status = fbe_api_get_all_drive_object_ids(object_id_list, FBE_MAX_PHYSICAL_OBJECTS, &drive_count);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s Error in getting the enclosrue IDs, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    drive_mgmt->total_drive_count = drive_count;

    for(i = 0; i < drive_count; i++) {
        status = fbe_api_get_object_lifecycle_state(object_id_list[i], &current_state, FBE_PACKAGE_ID_PHYSICAL);

        /*Only add drives with lifecycle state equal to "FAIL" or "READY"
        If drive is in activate or specialize state, then at the end
        it will go into FAIL, DESTROY or READY state which will be 
        handled in lifecycle state change notification from PDO */
        if((status == FBE_STATUS_OK)&&
          ((current_state == FBE_LIFECYCLE_STATE_FAIL)||
          (current_state == FBE_LIFECYCLE_STATE_READY)))
        {

            status = fbe_drive_mgmt_handle_new_drive(drive_mgmt, object_id_list[i]);
            if(status == FBE_STATUS_OK)
            {
                status = fbe_drive_mgmt_get_drive_phy_location_by_objectId(drive_mgmt,
                                                                           object_id_list[i],
                                                                           &phys_location);
                if(status == FBE_STATUS_OK)
                {

                    /* Send notification for new drive */
                    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)drive_mgmt,
                                                                        FBE_CLASS_ID_DRIVE_MGMT,
                                                                        FBE_DEVICE_TYPE_DRIVE,
                                                                        FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                        &phys_location);
                }
            }
        }
    }
    
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, object_id_list);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_discover_drive_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_apply_policy_verify_capacity()  
 ****************************************************************
 * @brief
 *  This function applies the policy of verify_capacity to the drive if 
 *  it is in READY state..
 *
 * @param drive_mgmt - pointer to the fbe_drive_mgmt_t structure.
 * @param object_id  - PDO object_id of the drive.
 * @param physical_drive_info - pointer to the structure. 
 * @param pdo_lifecycle_state- current drive lifecycle of PDO
 * @param deviceStr - Drive string.
 * @param lifecycle_state_str - drive lifecycle state
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  01-May-2013:  Moved here to be a separate function. Michael Li 
 ****************************************************************/ 
static fbe_status_t 
fbe_drive_mgmt_apply_policy_verify_capacity(fbe_drive_mgmt_t *drive_mgmt, 
                                            fbe_object_id_t object_id, 
                                            fbe_physical_drive_information_t *physical_drive_info,
                                            fbe_lifecycle_state_t pdo_lifecycle_state,
                                            char * deviceStr,
                                            char * lifecycle_state_str
                                            )
{

    fbe_status_t status = FBE_STATUS_OK;
    int i;
   
    //skip applying policy of "verify_capacity" if the drive is not in lifecycle READY state
    if(FBE_LIFECYCLE_STATE_READY != pdo_lifecycle_state)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "DMO: verify_capicity policy skipped due to !READY. DSK:%d_%d_%d %s %s %s %s\n",
                               physical_drive_info->port_number, physical_drive_info->enclosure_number,physical_drive_info->slot_number,
                               physical_drive_info->product_serial_num, physical_drive_info->tla_part_number, physical_drive_info->product_rev,
                               lifecycle_state_str);
        return status;
    }

    /* Workaround.  Terminator currently does not have capability to inject
       an inquiry string containing a product id.  Therefore this will only inject invalid capacities if
       built in simulation.  Under kernel it's a no-op 
     */
    fbe_drive_mgmt_inject_clar_id_for_simulated_drive_only(drive_mgmt, physical_drive_info);   

    for (i=0; i < fbe_drive_mgmt_known_drive_capacity_table_entries; i++)
    {
        /* If a known CLAR ID string is found then test against known capacity and fail drive if 
           incorrect.  If string not found then assume capacity given by drive is correct.
        */
        if (fbe_equal_memory(physical_drive_info->product_id+FBE_DRIVE_MGMT_CLAR_ID_OFFSET, 
                             fbe_drive_mgmt_known_drive_capacity[i].clar_id, 
                             FBE_DRIVE_MGMT_CLAR_ID_LENGTH)) 

        {                                                  
            if (physical_drive_info->gross_capacity != fbe_drive_mgmt_known_drive_capacity[i].gross_capacity)
            {
                fbe_event_log_write(ESP_ERROR_DRIVE_CAPACITY_FAILED, NULL, 0, "%s %s %s %s State:%s\n", 
                                    deviceStr, physical_drive_info->product_serial_num, physical_drive_info->tla_part_number, physical_drive_info->product_rev,
                                    lifecycle_state_str);

                fbe_api_physical_drive_fail_drive(object_id, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_DEBUG_LOW,FBE_TRACE_MESSAGE_ID_INFO,
                                    "DMO: %s Found CLAR ID string match. Capacity check passed\n",__FUNCTION__);
            }
            break;
        }        
    }                                                                                                                                                                   
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_drive_mgmt_apply_policy_verify_capacity() 
 ****************************************************/                   


/*!**************************************************************
 * fbe_drive_mgmt_apply_policy_verify_enhanced_queuing 
 ****************************************************************
 * @brief
 *  This function applies the policy of verifiying enhanced queuing to the drive.
 *
 * @param drive_mgmt - pointer to the fbe_drive_mgmt_t structure.
 * @param object_id  - PDO object_id of the drive.
 * @param physical_drive_info - pointer to the structure. 
 * @param pdo_lifecycle_state- current drive lifecycle of PDO
 * @param deviceStr - Drive string.
 * @param lifecycle_state_str - drive lifecycle state
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  01-May-2013:  Moved here to be a separate function. Michael Li 
 ****************************************************************/ 
static fbe_status_t 
fbe_drive_mgmt_apply_policy_verify_enhanced_queuing(fbe_drive_mgmt_t *drive_mgmt, 
                                                    fbe_object_id_t object_id, 
                                                    fbe_physical_drive_information_t *physical_drive_info,
                                                    fbe_drive_info_t *drive_info,
                                                    fbe_lifecycle_state_t pdo_lifecycle_state,
                                                    char * deviceStr,
                                                    char * lifecycle_state_str
                                                    )                                                      
{

    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t   is_os_drive = FBE_FALSE;
    fbe_bool_t   is_primary_os_drive = FBE_FALSE;
    fbe_bool_t   skip_drive_kill = FBE_FALSE;
    
    if ((physical_drive_info->drive_type == FBE_DRIVE_TYPE_SAS) ||
        (physical_drive_info->drive_type == FBE_DRIVE_TYPE_SAS_NL))
    {
        /* Special case for OS drives.*/
        status = fbe_drive_mgmt_is_os_drive(drive_mgmt, &drive_info->location, &is_os_drive, &is_primary_os_drive);
        if ((status == FBE_STATUS_OK) && (is_os_drive == FBE_TRUE))
        {
            /* Check whether both OS drives for the SPS are running bad f/w. */
            if (is_primary_os_drive)
            {
                if(FBE_LIFECYCLE_STATE_READY == pdo_lifecycle_state)
                {
                    drive_mgmt->primary_os_drive_policy_check_failed = !(physical_drive_info->enhanced_queuing_supported);
                }
                else
                {
                    //Fail policy check if drive not in READY state
                    drive_mgmt->primary_os_drive_policy_check_failed = FBE_TRUE;
                }
            }
            else
            {
                if(FBE_LIFECYCLE_STATE_READY == pdo_lifecycle_state)
                {
                    drive_mgmt->secondary_os_drive_policy_check_failed = !(physical_drive_info->enhanced_queuing_supported);
                }
                else
                {
                    drive_mgmt->secondary_os_drive_policy_check_failed = FBE_TRUE;
                }
            }
            if ((drive_mgmt->primary_os_drive_policy_check_failed == FBE_TRUE) &&
                (drive_mgmt->secondary_os_drive_policy_check_failed == FBE_TRUE))
            {

                    /* TEMP::: */
                    /* Set degraded mode and panic SP.*/
                    fbe_drive_mgmt_panic_sp(FBE_TRUE);
            }
            skip_drive_kill = FBE_TRUE;
        }

        if(FBE_LIFECYCLE_STATE_READY == pdo_lifecycle_state)
        {        
            /* Kill the drive and log information if enhanced queuing is not supported.*/
            if (physical_drive_info->enhanced_queuing_supported != FBE_TRUE)
            {

                fbe_event_log_write(ESP_ERROR_DRIVE_ENHANCED_QUEUING_FAILED, NULL, 0,"%s %s %s %s State:%s",  
                                    deviceStr, physical_drive_info->product_serial_num, physical_drive_info->tla_part_number, physical_drive_info->product_rev,
                                    lifecycle_state_str);
                                
                if (skip_drive_kill != FBE_TRUE)
                {
                    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "DMO: %s Failing unsupported drive due to enhanced queuing not supported by drive.\n",
                                          __FUNCTION__);

                    fbe_api_physical_drive_fail_drive(object_id, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED); 
                }
            }/* Queue not supported*/                                                                                                                                        
        }
    }                                                                                                                                                             

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_drive_mgmt_apply_policy_verify_enhanced_queuing 
 ****************************************************/          


/*!**************************************************************
 * fbe_drive_mgmt_apply_policy_set_system_drive_io_max 
 ****************************************************************
 * @brief
 *  This function applies the policy of set_system_drive_io_max.
 *
 * @param drive_mgmt - pointer to the fbe_drive_mgmt_t structure.
 * @param object_id  - PDO object_id of the drive.
 * @param physical_drive_info - pointer to the structure. 
 * @param pdo_lifecycle_state - current drive lifecycle of PDO
 * @param deviceStr - Drive string.
 * @param lifecycle_state_str - drive lifecycle state.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  01-May-2013:  Moved here to be a separate function. Michael Li 
 ****************************************************************/ 
static fbe_status_t 
fbe_drive_mgmt_apply_policy_set_system_drive_io_max(fbe_drive_mgmt_t *drive_mgmt,
                                                    fbe_object_id_t object_id, 
                                                    fbe_physical_drive_information_t *physical_drive_info,
                                                    fbe_drive_info_t *drive_info,
                                                    fbe_lifecycle_state_t pdo_lifecycle_state,
                                                    char * deviceStr,
                                                    char * lifecycle_state_str
                                                    )                                                      
{
    fbe_bool_t   is_system_drive = FBE_FALSE;    
    fbe_u32_t    dmo_system_drive_queue_depth;
    fbe_status_t status = FBE_STATUS_OK;
    
     //skip applying policy of "set_system_drive_io_max" if the drive is not in lifecycle READY state
    if(FBE_LIFECYCLE_STATE_READY != pdo_lifecycle_state)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "DMO: set_sysDrvIOMax policy skipped due to !READY,DSK:%d_%d_%d %s %s %s %s\n",
                               physical_drive_info->port_number, physical_drive_info->enclosure_number,physical_drive_info->slot_number,
                               physical_drive_info->product_serial_num, physical_drive_info->tla_part_number, physical_drive_info->product_rev,
                               lifecycle_state_str);
        return status;
    }   
    
    status = drive_mgmt_get_system_drive_normal_qdepth(drive_mgmt, &dmo_system_drive_queue_depth);
    if (status == FBE_STATUS_OK)
    {
        status = fbe_drive_mgmt_is_system_drive(drive_mgmt, &drive_info->location, &is_system_drive);
        if ((status == FBE_STATUS_OK) && (is_system_drive == FBE_TRUE))
        {
            status = fbe_api_physical_drive_set_object_queue_depth(object_id, dmo_system_drive_queue_depth, FBE_PACKET_FLAG_NO_ATTRIB);
            if (status == FBE_STATUS_OK){
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                                    "DMO: Setting system drive %d queue depth to %d. \n",drive_info->location.slot, dmo_system_drive_queue_depth);
            }else{
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "DMO: Error setting system drive %d queue depth to %d. Status 0x%x.\n",drive_info->location.slot, dmo_system_drive_queue_depth, status);
            }
        }
    }else{
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "DMO: Error querying system drive %d queue depth . Status 0x%x.\n",drive_info->location.slot, status);

    }                                                                                                                                                                            

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_drive_mgmt_apply_policy_set_system_drive_io_max 
 ****************************************************/                    

 /*!**************************************************************
* fbe_drive_mgmt_apply_policy_verify_sed_components 
****************************************************************
* @brief
*  This function applies the policy of verifiying if it's SED drive.
*	If detected as SED drive, drive is transitioned to FAIL state
*
* @param drive_mgmt - pointer to the fbe_drive_mgmt_t structure.
* @param object_id  - PDO object_id of the drive.
* @param physical_drive_info - pointer to the structure. 
* @param pdo_lifecycle_state- current drive lifecycle of PDO
* @param deviceStr - Drive string.
* @param lifecycle_state_str - drive lifecycle state
*
* @return fbe_status_t - FBE Status
*
* @author
*  01-May-2013:  Created: hakimh
****************************************************************/ 
static fbe_status_t 
fbe_drive_mgmt_apply_policy_verify_sed_components(fbe_drive_mgmt_t *drive_mgmt, 
		fbe_object_id_t object_id, 
		fbe_physical_drive_information_t *physical_drive_info,
		fbe_drive_info_t *drive_info,
		fbe_lifecycle_state_t pdo_lifecycle_state,
		char * deviceStr,
		char * lifecycle_state_str)
{

    fbe_status_t status = FBE_STATUS_OK;    
    
	if(FBE_LIFECYCLE_STATE_READY == pdo_lifecycle_state) {
		// Currently SED drives are not supported on both Rockies and KH program.
		// By default mark drive offline. Later if SED drives are supported and 
		// policy is platform such as SED vs non-SED arrays, logic for the same 
		// can be incorporated below.
		if (physical_drive_info->drive_type == FBE_DRIVE_TYPE_SAS_SED) {
			// Kill the drive and log information SED drive is not supported
			fbe_event_log_write(ESP_ERROR_DRIVE_SED_NOT_SUPPORTED_FAILED, NULL, 0,
								"%s %s %s %s State:%s", deviceStr,
								physical_drive_info->product_serial_num,
								physical_drive_info->tla_part_number,
								physical_drive_info->product_rev,
								lifecycle_state_str);
			fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,
								  FBE_TRACE_LEVEL_INFO,
								  FBE_TRACE_MESSAGE_ID_INFO,
								  "DMO: %s Failing drive %d_%d_%d as SED drives not supported.\n",
								  __FUNCTION__,
								  physical_drive_info->port_number,
								  physical_drive_info->enclosure_number,
								  physical_drive_info->slot_number);
			status = fbe_api_physical_drive_fail_drive(object_id, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED);
		}
	}
	return status;
}
/****************************************************
 * end fbe_drive_mgmt_apply_policy_verify_sed_components
 ****************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_handle_new_drive()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the drive management.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * @param object_id - Object ID of the drive that just got added
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *  03-Apr-2013   Arun Joseph. Added set vault drive max io policy 
 *  01-May-2013   Michael Li. Added skip applying policy if drive not READY
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_handle_new_drive(fbe_drive_mgmt_t *drive_mgmt, fbe_object_id_t object_id)
{
    fbe_physical_drive_information_t *physical_drive_info = NULL;
    fbe_status_t status;
    fbe_u32_t port_number, drive_number, enclosure_number;
    fbe_device_physical_location_t location;
    fbe_drive_info_t *drive_info = NULL;
    fbe_api_get_discovery_edge_info_t   edge_info;
    fbe_lifecycle_state_t current_state;
    char deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_object_id_t encl_object_id;
    fbe_enclosure_types_t enclosure_type;
    fbe_number_of_slots_t slot_per_bank;
    char * fullCurrentStateStr = NULL;
    fbe_u32_t reason = DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE;
    dmo_drive_death_action_t action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;
    fbe_block_transport_logical_state_t drive_logical_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;
    
    
    status = fbe_api_get_object_port_number (object_id, &port_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = fbe_api_get_object_enclosure_number(object_id, &enclosure_number);

    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = fbe_api_get_object_drive_number (object_id, &drive_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    physical_drive_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_physical_drive_information_t));
    if(physical_drive_info == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Memory Allocation failed\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_physical_drive_get_drive_information(object_id, physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
        return status;
    }
   
    status = fbe_api_physical_drive_get_logical_drive_state(object_id, &drive_logical_state);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Error getting logical state for %d_%d_%d pdo(0x%x), status 0x%x.\n",                              
                              __FUNCTION__, port_number, enclosure_number, drive_number, object_id, status);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
        return status;
    }  

    status = fbe_api_get_object_lifecycle_state(object_id, &current_state, FBE_PACKAGE_ID_PHYSICAL);
    
    location.bus = port_number;
    location.enclosure = enclosure_number;
    location.slot = drive_number;

    slot_per_bank = 0;
    status = fbe_api_get_enclosure_object_id_by_location(location.bus, location.enclosure, &encl_object_id);
    if (status == FBE_STATUS_OK) 
    {
        status = fbe_api_enclosure_get_encl_type(encl_object_id, &enclosure_type);
        if ((status == FBE_STATUS_OK) && 
            ((enclosure_type == FBE_ENCL_SAS_VOYAGER_ICM) ||
             (enclosure_type == FBE_ENCL_SAS_VIKING_IOSXP) ||
             (enclosure_type == FBE_ENCL_SAS_CAYENNE_IOSXP) ||
             (enclosure_type == FBE_ENCL_SAS_NAGA_IOSXP)))
        {
            status = fbe_api_enclosure_get_slot_count_per_bank(encl_object_id, &slot_per_bank);
        }
    }

    if (slot_per_bank != 0)
    {
        location.bank = (location.slot / slot_per_bank) + 'A';
        location.bank_slot = location.slot % slot_per_bank;
    }
    else 
    {
        location.bank = '\0';
        location.bank_slot = 0;
    }

    /* Find a free entry and then update local state*/
    status = fbe_drive_mgmt_get_drive_info_pointer(drive_mgmt, &location, &drive_info);
    if(drive_info == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Free handle not found",
                              __FUNCTION__);
        /* Free entry not found and so return an error */
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_get_discovery_edge_info (object_id, &edge_info);

    //TODO: consider copying the entire physical_drive_info and holding onto it as a data member.  No need to handpick stuff from here.

    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
    drive_info->object_id = object_id;
    drive_info->parent_object_id = edge_info.server_id;
    drive_info->state = current_state;
    drive_info->death_reason =  FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;   
    drive_info->location.bus = port_number;
    drive_info->location.enclosure = enclosure_number;
    drive_info->location.slot = drive_number;
    drive_info->location.bank = location.bank;
    drive_info->location.bank_slot = location.bank_slot;
    drive_info->drive_type = physical_drive_info->drive_type;
    drive_info->gross_capacity = physical_drive_info->gross_capacity;    
    fbe_copy_memory(&drive_info->vendor_id, &physical_drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
    drive_info->vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE] = '\0';
    fbe_copy_memory(&drive_info->tla, &physical_drive_info->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE);
    drive_info->tla[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0';

    //TODO: Investigate if paddle card rev is passed here. If not, is there a need to fetch it?
    fbe_copy_memory(&drive_info->rev, &physical_drive_info->product_rev, physical_drive_info->product_rev_len);
    drive_info->rev[physical_drive_info->product_rev_len] = '\0';
    fbe_copy_memory(&drive_info->sn, &physical_drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    drive_info->sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0';
    
    drive_info->block_count = physical_drive_info->block_count;
    drive_info->block_size = physical_drive_info->block_size;
    drive_info->speed_capability = physical_drive_info->speed_capability;
    drive_info->drive_qdepth = physical_drive_info->drive_qdepth;
    drive_info->drive_RPM = physical_drive_info->drive_RPM;    
    drive_info->spin_down_qualified= physical_drive_info->spin_down_qualified;
    fbe_copy_memory(&drive_info->drive_description_buff, &physical_drive_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE);
    drive_info->drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE] = '\0';
    fbe_copy_memory(&drive_info->bridge_hw_rev, &physical_drive_info->bridge_hw_rev, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE);
    drive_info->bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE] = '\0';
    fbe_copy_memory(&drive_info->dg_part_number_ascii, &physical_drive_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);
    drive_info->dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE] = '\0';
    fbe_copy_memory(&drive_info->product_id, &physical_drive_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    drive_info->product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE] = '\0';
    drive_info->spin_up_count = physical_drive_info->spin_up_count;
    drive_info->spin_up_time_in_minutes = physical_drive_info->spin_up_time_in_minutes;
    drive_info->stand_by_time_in_minutes = physical_drive_info->stand_by_time_in_minutes;   
    drive_info->logical_state = drive_logical_state; 
        
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        // generate the device string (used for tracing & events)
                                               &drive_info->location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {   
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s fbe_base_env_create_device_string failed, status: 0x%X\n", __FUNCTION__, status);
    }

    if(current_state == FBE_LIFECYCLE_STATE_FAIL)
    {
        /* get the death reason from the object */
        status = fbe_api_get_object_death_reason(object_id, &reason, NULL, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s unable to get death reason for PDO object_id=0x%X\n", __FUNCTION__, object_id);
            reason = DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE;
        }

        if (drive_info != NULL) {
            drive_info->death_reason = reason;
        }

        action = fbe_drive_mgmt_drive_failed_action(drive_mgmt,reason);

        fbe_drive_mgmt_log_drive_offline_event(drive_mgmt, action,
                                               physical_drive_info, reason,
                                               &location);
    }
    else
    {
        fbe_event_log_write(ESP_INFO_DRIVE_ONLINE, NULL, 0, "%s %s %s %s",
                            &deviceStr[0], drive_info->sn, drive_info->tla, drive_info->rev );

  
        if (drive_logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED &&
            drive_logical_state != FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE)
        {
            reason = fbe_drive_mgmt_get_logical_offline_reason(drive_mgmt, drive_logical_state);
            action = fbe_drive_mgmt_drive_failed_action(drive_mgmt, reason);
    
            fbe_drive_mgmt_log_drive_offline_event(drive_mgmt, action,
                                                   physical_drive_info, reason,
                                                   &location);
        }
    }

    /*
     Before the drive get to lifecycle FBE_LIFECYCLE_STATE_READY state, the drive information has not been inquiried to  
     physical_drive_info yet. We skip killing the drive in this case.
    */ 
    fullCurrentStateStr = fbe_api_esp_drive_mgmt_map_lifecycle_state_to_str(current_state);

    if (fbe_drive_mgmt_policy_enabled(FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY))
    {   

        status = fbe_drive_mgmt_apply_policy_verify_capacity(drive_mgmt,
                                                             object_id, 
                                                             physical_drive_info,
                                                             current_state,
                                                             &deviceStr[0],
                                                             fullCurrentStateStr
                                                            );
    }

    if (fbe_drive_mgmt_policy_enabled(FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING))
    {

        status = fbe_drive_mgmt_apply_policy_verify_enhanced_queuing(drive_mgmt,
                                                              object_id, 
                                                              physical_drive_info,
                                                              drive_info,
                                                              current_state,
                                                              &deviceStr[0],
                                                              fullCurrentStateStr
                                                             );
    }

    if (fbe_drive_mgmt_policy_enabled(FBE_DRIVE_MGMT_POLICY_SET_SYSTEM_DRIVE_IO_MAX)) 
    {
        status = fbe_drive_mgmt_apply_policy_set_system_drive_io_max(drive_mgmt,
                                                                     object_id, 
                                                                     physical_drive_info,
                                                                     drive_info,
                                                                     current_state,
                                                                     &deviceStr[0],
                                                                     fullCurrentStateStr
                                                                    );
    }                                                                                          

    if(fbe_drive_mgmt_policy_enabled(FBE_DRIVE_MGMT_POLICY_VERIFY_FOR_SED_COMPONENTS)) {
		status = fbe_drive_mgmt_apply_policy_verify_sed_components(drive_mgmt,
				 object_id, 
				 physical_drive_info,
				 drive_info,
				 current_state,
				 &deviceStr[0],
				 fullCurrentStateStr
																  );
	}
	
	fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);

    status = fbe_drive_mgmt_process_enclosure_status(drive_mgmt, &drive_info->location, drive_info->parent_object_id);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %d_%d_%d process_enclosure_status failed, status 0x%x.\n",
                              drive_info->location.bus,
                              drive_info->location.enclosure,
                              drive_info->location.slot,
                              status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set logical crc actions */
    status = fbe_api_physical_drive_set_crc_action(object_id, drive_mgmt->logical_error_actions);
    if (FBE_STATUS_OK != status)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s: failed to set crc action, object_id 0x%x",__FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_drive_mgmt_handle_new_drive() 
 **************************************/           

 
/*!**************************************************************
 * fbe_drive_mgmt_fuel_gauge_timer_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the fuel gauge timer to get flash
 *  drive fuel gauge information.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  11/15/10:  Created. chenl6
 *
 ****************************************************************/

static fbe_lifecycle_status_t
fbe_drive_mgmt_fuel_gauge_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t        status;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s entry\n",
                          __FUNCTION__);
    
    /* we would like to skip the first timer since the system is booting and is too busy to do Fuel Gauge. */
    if (drive_mgmt->fuel_gauge_info.is_1st_time)
    {       
        fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_1st_time, FBE_FALSE);
                
        /* don't start fuel gauge, it is the first time the timer is fired. */
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s System is booting up. Fuel gauge is skipped. \n",
                              __FUNCTION__);
        
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    if (drive_mgmt->fuel_gauge_info.is_in_progrss)
    {
        /* Can't start fuel gauge, it is in progress. */
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s Fuel gauge is in process. \n",
                              __FUNCTION__);
        
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    if ((drive_mgmt->fuel_gauge_info.auto_log_flag) /* Fuel Gauge is enabled.*/)
    {
        /* Set read condition */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                        (fbe_base_object_t*)drive_mgmt, 
                                        FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s can't set READ condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        } 
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_timer_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_fuel_gauge_read_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the fuel gauge read to get flash
 *  drive fuel gauge information.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  11/15/10:  Created. chenl6
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_fuel_gauge_read_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info; 
    fbe_u16_t i;
    fbe_atomic_t       is_lock = FBE_TRUE;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);
    
    if (fbe_drive_mgmt_fuel_gauge_get_eligible_drive(drive_mgmt) == FBE_STATUS_OK)
    {
        /* Initialize structures until necessary. */
        if (fuel_gauge_info->is_initialized == FBE_FALSE)
        {
            status = fbe_drive_mgmt_fuel_gauge_init(drive_mgmt);
        }

        if(status == FBE_STATUS_OK)
        {
            fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
            
            /* Fill out the fuel gauge operation paramsters. */
            fbe_drive_mgmt_fuel_gauge_fill_params(drive_mgmt, fuel_gauge_info->fuel_gauge_op, LOG_PAGE_0);

            fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
            
            /* Get fuel gauge info from the drive. */
            fbe_api_physical_drive_get_fuel_gauge_asynch(fuel_gauge_info->current_object, 
                                                     fuel_gauge_info->fuel_gauge_op);
            
         }
         else  /* status != FBE_STATUS_OK */
         {
             /* Set Fuel Gauge is not in process flag. */
             is_lock = fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_in_progrss, FBE_FALSE);
             if (is_lock == FBE_FALSE) {
                 fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                       "DMO: %s Fuel Gauge is_in_progrss flag is turned off. \n",
                                       __FUNCTION__);
             }
         }
    } 
    /* send each log page which is supported by the drive down to PDO to get the SMART/Fuel Gauge data.*/
    else if (fuel_gauge_info->current_supported_page_index <= DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT)
    {
        if (drive_mgmt->fuel_gauge_info.current_supported_page_index == DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT) 
        {
            /* Done with log pages, calcuate PE_cycle_warrantee_limit. */
            fbe_drive_mgmt_fuel_gauge_check_EOL_warrantee(drive_mgmt);
            
            fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
            /* No more log pages. Set it to 0xff. */
            fuel_gauge_info->current_supported_page_index = LOG_PAGE_NONE; 
            fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
            
            /* Set READ condition. */
            status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                            (fbe_base_object_t*)drive_mgmt, 
                                            FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DMO: %s can't set READ condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
        
        for ( i = fuel_gauge_info->current_supported_page_index; i < DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT; i++ )
        {
            fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
                  
            /* Fill out the fuel gauge operation paramsters. */
            fbe_drive_mgmt_fuel_gauge_fill_params(drive_mgmt, fuel_gauge_info->fuel_gauge_op, dmo_fuel_gauge_supported_pages[i].page_code);
            fuel_gauge_info->current_supported_page_index = (i + 1);  /* next log page to start search */
            fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
  
            /* Only handle the supported log page. */           
            if (dmo_fuel_gauge_supported_pages[i].supported)
            {
                /* Get fuel gauge info from the drive. */
                fbe_api_physical_drive_get_fuel_gauge_asynch(fuel_gauge_info->current_object, 
                        fuel_gauge_info->fuel_gauge_op);
                break;
            }
            else
            {
                /* Set READ condition. */
                status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                                (fbe_base_object_t*)drive_mgmt, 
                                                FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
                if (status != FBE_STATUS_OK) {
                    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "DMO: %s can't set READ condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
                break;
            }   
        }
    }
    else
    {
        /* No drive eligible. Cleanup. */
        fbe_drive_mgmt_fuel_gauge_cleanup(drive_mgmt);
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_read_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_fuel_gauge_write_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the fuel gauge write to write flash
 *  drive fuel gauge information to the drive.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  11/15/10:  Created. chenl6
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_fuel_gauge_write_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info; 
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u16_t           supported_page;
   
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);

    /* Handle all the log pages except log page 0. */
    supported_page = dmo_fuel_gauge_supported_pages[fuel_gauge_info->current_supported_page_index-1].page_code;
    
    if ((dmo_fuel_gauge_supported_pages[fuel_gauge_info->current_supported_page_index - 1].supported) &&
            (fuel_gauge_info->fuel_gauge_op->get_log_page.page_code == supported_page))
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s Writing log page %x to the file.\n",
                              __FUNCTION__, supported_page);
        
        status = fbe_drive_mgmt_fuel_gauge_write_to_file(drive_mgmt);
    }
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    /* Always set READ condition. */
    status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                    (fbe_base_object_t*)drive_mgmt, 
                                    FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't set READ condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_write_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond_function()
 ****************************************************************
 * @brief
 *  This function handles update dmo_fuel_gauge_supported_pages table 
 *                       with the information receive from log page 0.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/07/12:  Created. chianc1
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info; 
    fbe_u8_t            *byte_read_ptr = fuel_gauge_info->read_buffer;
 
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);
    
    /* Search supported log pages page to find the correct log page 0x30 or 0x31 for the drivr. */
    fbe_drive_mgmt_fuel_gauge_find_valid_log_page_code(byte_read_ptr, fuel_gauge_info->fuel_gauge_op);
            
    /* Continue to read */
    status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                        (fbe_base_object_t*)drive_mgmt, 
                                        FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
        
    if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s can't set READ condition in find_valid_log_page_code, status: 0x%X\n",
                                  __FUNCTION__, status);
    }
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_update_pages_tbl_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_updateEnclFaultLed()
 ****************************************************************
 * @brief
 *  This function checks the Drives' status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param drive_mgmt -
 * @param pLocation - The location of the PS.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  17-Feb-2011:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_updateEnclFaultLed(fbe_drive_mgmt_t *drive_mgmt,
                                  fbe_object_id_t drive_object_id)
{
    fbe_status_t                                status;
    fbe_drive_info_t                            *driveInfoPtr = NULL;
    fbe_base_enclosure_led_status_control_t     ledStatusControlInfo;
    fbe_u32_t                                   driveIndex;
    fbe_bool_t                                  driveFaultDetected = FALSE, enclDriveFaultDetected = FALSE;
    fbe_u32_t                                   port_number, enclosure_number, slot_number;
    fbe_object_id_t                             encl_object_id;
    fbe_device_physical_location_t phys_location = {0};
    fbe_base_enclosure_led_behavior_t           ledBehavior = FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF;
    fbe_physical_drive_information_t drive_info = {0};

    status = fbe_drive_mgmt_get_drive_phy_location_by_objectId(drive_mgmt,    
                                                                    drive_object_id,
                                                                    &phys_location);                    
    if (status == FBE_STATUS_OK)
    {
        /* find drive location. */
        port_number = phys_location.bus;
        enclosure_number = phys_location.enclosure;
        slot_number = phys_location.slot;

        /*
        * Determine if there are any faulted Drives in this enclosure
        */
        driveInfoPtr = drive_mgmt->drive_info_array;
        for (driveIndex = 0; driveIndex < drive_mgmt->max_supported_drives; driveIndex++)
        {
            /*
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "  %s, index %d, driveInfoPtr 0x%x, slot %d, state %d\n",
                              __FUNCTION__, 
                              driveIndex,
                              driveInfoPtr,
                              ((driveInfoPtr == NULL) ? 0 : driveInfoPtr->location.slot),
                              ((driveInfoPtr == NULL) ? 0 : driveInfoPtr->state));
            */
            
            /* end of array (no more drives), so exit loop */
            if (driveInfoPtr == NULL)
            {
                break;
            }

            /* only check the specified enclosure */
            if ((driveInfoPtr->location.bus == port_number) &&
                (driveInfoPtr->location.enclosure == enclosure_number))
            {
                /* check for failed enclosure and drive */
                if ( (driveInfoPtr->state == FBE_LIFECYCLE_STATE_FAIL) ||
                     (driveInfoPtr->state == FBE_LIFECYCLE_STATE_PENDING_FAIL) ||
                     ( (driveInfoPtr->state != FBE_LIFECYCLE_STATE_DESTROY) &&
                       ( (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK) ||
                         (driveInfoPtr->logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER)
                       )
                     )
                   )
                 {
                    /* Check for the fault enclosure */
                    enclDriveFaultDetected = TRUE;
                    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DMO: %s, encl %d_%d, fault Enclosure detected\n",
                                      __FUNCTION__, port_number, enclosure_number);
                    
                    /* check for the fault drive .*/
                    if (driveInfoPtr->location.slot == slot_number)
                    {
                        driveFaultDetected = TRUE;
                        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "DMO: %d_%d_%d %s, fault Drive detected\n",
                                          port_number, enclosure_number, slot_number, __FUNCTION__);
                    
                        break;
                    }
                }
            }
            driveInfoPtr++;
        }
    }
    else
    {
        /* If dmo can't get the drive information. (i.e. not found) maybe it failed before it reached ready state. Try to get the drive info from PDO itself*/                    
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s drive oid=0x%x isn't ready in DMO, get it from PDO. \n", __FUNCTION__, drive_object_id);
        
        status = fbe_api_physical_drive_get_drive_information(drive_object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s drive oid=0x%x can't get drive info\n", __FUNCTION__, drive_object_id);
            return status;
        }
        else
        {
            /* find drive location. */
            port_number = drive_info.port_number;
            enclosure_number = drive_info.enclosure_number;
            slot_number = drive_info.slot_number;
            driveFaultDetected = TRUE;
            enclDriveFaultDetected = TRUE;
        }
    }
    
    fbe_set_memory(&ledStatusControlInfo, 0, sizeof(fbe_base_enclosure_led_status_control_t));

    if ((drive_mgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
             (port_number == 0) && (enclosure_number == 0))
    {
        status = fbe_api_get_board_object_id(&encl_object_id);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s, failed to get board object, status: 0x%X.\n",
                                  __FUNCTION__,
                                  status);
            return status;
        }
    }
    else
    {
        status = fbe_api_get_enclosure_object_id_by_location(port_number, enclosure_number, &encl_object_id);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: Encl %d_%d error getting Encl ObjectId, status: 0x%X %s.\n",
                                  port_number, enclosure_number, status, __FUNCTION__);
            return status;
        }
    }

    /* Update enclosure led. */
    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)drive_mgmt,
                                                     encl_object_id,
                                                     (enclDriveFaultDetected ? TRUE : FALSE),
                                                     FBE_ENCL_FAULT_LED_DRIVE_FLT);
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: Encl %d_%d ObjId %d, FaultLed %s  %s\n",
                              port_number, enclosure_number, encl_object_id,
                              (enclDriveFaultDetected ? "ON" : "OFF"), __FUNCTION__);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: Encl %d_%d error turning %s , ObjId %d, FaultLed, status: 0x%X %s.\n",
                              port_number, enclosure_number, (enclDriveFaultDetected ? "ON" : "OFF"),
                              encl_object_id, status, __FUNCTION__);
        return status;
    }

    /* Update drive led. */
    ledBehavior = (driveFaultDetected ? FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON : FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF);
        
    status = fbe_api_enclosure_update_drive_fault_led(port_number, enclosure_number, slot_number, ledBehavior);
        
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d %s Drive FaultLed %s\n",
                              port_number, enclosure_number, slot_number, __FUNCTION__,
                              (driveFaultDetected ? "ON" : "OFF"));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %d_%d_%d %s Error turning Drive FaultLed %s status: 0x%X.\n",
                              port_number, enclosure_number, slot_number, __FUNCTION__,
                              (driveFaultDetected ? "ON" : "OFF"), status);
            return status;
     }
    
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_mgmt_updateEnclFaultLed() 
 ******************************************************/

/*!*************************************************************************
 *  fbe_drive_mgmt_get_drive_phy_location_by_objectId()
 ***************************************************************************
 * @brief
 *  This function gets the physical location of drive from given object ID..
 *
 * @param  drive_mgmt - The pointer to the fbe_drive_mgmt_t.
 * @param  object_id - Drive object ID.
 * @param  phys_location - Buffer to return physical location. 
 *
 * @return - FBE_STATUS_OK - If valid physical location found.
 * 
 * @author
 *  18-Feb-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t
fbe_drive_mgmt_get_drive_phy_location_by_objectId(fbe_drive_mgmt_t  *drive_mgmt,
                                                  fbe_object_id_t  object_id,
                                                  fbe_device_physical_location_t *phys_location)
{
    fbe_drive_info_t        *drive = NULL;

    dmo_drive_array_lock(drive_mgmt,"drive_mgmt_get_phy_loc_by_objId"); 
    drive = fbe_drive_mgmt_get_drive_p(drive_mgmt, object_id);  
    dmo_drive_array_unlock(drive_mgmt,"drive_mgmt_get_phy_loc_by_objId");

    if(drive == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,  
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: get_phy_loc_by_objId is not existed, objectId 0x%X.\n", object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *phys_location = drive->location;
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_drive_mgmt_get_drive_phy_location_by_objectId() 
 *********************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_get_drive_info_by_location()
 ****************************************************************
 * @brief
 *  This function gets the pointer to the drive info by the location.
 *  Drive info needs to have a valid obj id or a valid parent obj id.
 *  When obj id is valid parent obj id must also be valid.
 *
 * @param drive_mgmt - 
 * @param pLocation - Location of the drive.
 * @param pDriveInfoPtr (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  19-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_get_drive_info_by_location(fbe_drive_mgmt_t *drive_mgmt,
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_drive_info_t **pDriveInfoPtr)
{
    fbe_u32_t          driveIndex = 0;
    fbe_drive_info_t * pDriveInfo = NULL;

    *pDriveInfoPtr = NULL;
    pDriveInfo = drive_mgmt->drive_info_array;

    for (driveIndex = 0; driveIndex < drive_mgmt->max_supported_drives; driveIndex ++)
    {
        if((pDriveInfo->object_id != FBE_OBJECT_ID_INVALID) &&
           (pDriveInfo->parent_object_id == FBE_OBJECT_ID_INVALID))
        {
            // error case
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %d_%d_%d Drive Obj:%d Invalid Parent Obj Id, %s \n",
                                  pDriveInfo->location.bus, pDriveInfo->location.enclosure,
                                  pDriveInfo->location.slot, pDriveInfo->object_id, __FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if((pDriveInfo->object_id != FBE_OBJECT_ID_INVALID) ||
           (pDriveInfo->parent_object_id != FBE_OBJECT_ID_INVALID))
        {
            if((pDriveInfo->location.bus == pLocation->bus) &&
               (pDriveInfo->location.enclosure == pLocation->enclosure) &&
               (pDriveInfo->location.slot == pLocation->slot))
            {
                *pDriveInfoPtr = pDriveInfo;
                return FBE_STATUS_OK;
            }
        }

        pDriveInfo++;  
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

/*!**************************************************************
 * fbe_drive_mgmt_get_available_drive_info_entry()
 ****************************************************************
 * @brief
 *  This function finds a drive info entry that is not in use.
 *
 * @param drive_mgmt - 
 * @param pDriveInfoPtr (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  19-Jul-2011:  GB - Created. 
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_get_available_drive_info_entry(fbe_drive_mgmt_t *pDriveMgmt, 
                                                           fbe_drive_info_t **ppDriveInfo)
{
    fbe_u32_t           i;
    fbe_drive_info_t    *pDriveInfo = NULL;

    // get the first available drive info item
    pDriveInfo = pDriveMgmt->drive_info_array;
    dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);
    for (i = 0; i < pDriveMgmt->max_supported_drives; i++, pDriveInfo++)
    {
        if((pDriveInfo->object_id == FBE_OBJECT_ID_INVALID) &&
           (pDriveInfo->parent_object_id == FBE_OBJECT_ID_INVALID))
        {
            *ppDriveInfo = pDriveInfo;
            dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);
            return FBE_STATUS_OK;
            break;
        }
    }

    dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);

    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "DMO: %s Free handle not found\n",
                          __FUNCTION__);
    /* Free entry not found and so return an error */
    return FBE_STATUS_GENERIC_FAILURE;
    
} //fbe_drive_mgmt_get_available_drive_info_entry

/*!**************************************************************
 * fbe_drive_mgmt_get_drive_info_pointer()
 ****************************************************************
 * @brief
 *  First look for an entry that matches the location info. If not
 *  found look for an unused entry.
 *
 * @param drive_mgmt - 
 * @param pLocation - Location of the drive.
 * @param pDriveInfoPtr (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  19-Aug-2011:  GB - Created. 
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_get_drive_info_pointer(fbe_drive_mgmt_t *pDriveMgmt,
                                                   fbe_device_physical_location_t *pLocation,
                                                   fbe_drive_info_t **ppDriveInfo)
{
    fbe_status_t        status;

    status = fbe_drive_mgmt_get_drive_info_by_location(pDriveMgmt, pLocation, ppDriveInfo);
    if(*ppDriveInfo == NULL)
    {
        status = fbe_drive_mgmt_get_available_drive_info_entry(pDriveMgmt,
                                                               ppDriveInfo);
    }
    return status;
}

/*!**************************************************************
 * fbe_drive_mgmt_dieh_init_cond_function()
 ****************************************************************
 * @brief
 *  This function initializes DIEH (Drive Improved Error Handling).
 *
 * @param base_object - drive_mgmt.
 * @param packet - pointer to packet.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/15/12:  Wayne Garrett -- Created.
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_dieh_init_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t   *drive_mgmt = (fbe_drive_mgmt_t *) base_object;
    fbe_dieh_load_status_t  dieh_status;
    fbe_status_t       status = FBE_STATUS_INVALID;
    fbe_u32_t          system_normal_q_depth    = 0;
    fbe_u32_t          system_reduced_q_depth   = 0;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry\n",
                      __FUNCTION__);

    dieh_status = fbe_drive_mgmt_dieh_init(drive_mgmt, NULL);

    if (dieh_status == FBE_DMO_THRSH_NO_ERROR)
    {
        system_normal_q_depth = drive_mgmt->xml_parser->drv_config.drv_info_parser.system_normal_q_depth;
        if ((system_normal_q_depth != 0) &&
            (system_normal_q_depth <= drive_mgmt->xml_parser->drv_config.drv_info_parser.default_q_depth))
        {
            drive_mgmt_set_system_drive_normal_qdepth_global(drive_mgmt, system_normal_q_depth);
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: dieh_init: set sys drive q depth global to %d.\n",
                                  (int)system_normal_q_depth);

        }else{
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "DMO: dieh_init: sys q depth not found or invalid sys:0x%X default:0x%X \n", system_normal_q_depth,
                    drive_mgmt->xml_parser->drv_config.drv_info_parser.default_q_depth);
        }

        system_reduced_q_depth = drive_mgmt->xml_parser->drv_config.drv_info_parser.system_reduced_q_depth;
        if ((system_reduced_q_depth != 0) &&
            (system_reduced_q_depth <= drive_mgmt->xml_parser->drv_config.drv_info_parser.default_q_depth))
        {
            drive_mgmt_set_system_drive_reduced_qdepth_global(drive_mgmt, system_reduced_q_depth);
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: dieh_init: set sys drive reduced q depth global to %d.\n", 
                                  (int)system_reduced_q_depth);

        }else{
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "DMO: dieh_init: sys reduced q depth not found or invalid sys:0x%X default:0x%X \n", system_reduced_q_depth,
                    drive_mgmt->xml_parser->drv_config.drv_info_parser.default_q_depth);
        }
    }else{
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: dieh_init: Failed to init DIEH, load status: 0x%X\n", dieh_status);
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: dieh_init: can't clear current condition, status: 0x%X", status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_dieh_init_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_get_drive_log_cond_function()
 ****************************************************************
 * @brief
 *  This function handles drivegetlog command for drive paddlecard.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/25/10:  Created. chenl6
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_get_drive_log_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t        status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);

    status = fbe_drive_mgmt_get_drive_log(drive_mgmt);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_get_drive_log_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_write_drive_log_cond_function()
 ****************************************************************
 * @brief
 *  This function writes drivegetlog info to a file.
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/25/10:  Created. chenl6
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_drive_mgmt_write_drive_log_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_drive_mgmt_t *  drive_mgmt = (fbe_drive_mgmt_t*)base_object;
    fbe_status_t        status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);


    status = fbe_drive_mgmt_write_drive_log(drive_mgmt);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    /* Clean up. */
    fbe_drive_mgmt_drive_log_cleanup(drive_mgmt);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_write_drive_log_cond_function() 
 ******************************************************/





/*!**************************************************************
 * fbe_drive_mgmt_delayed_state_change_callback_register_cond_function()
 ****************************************************************
 * @brief
 *  This function carries out delayed registration for disk notification
 *  by DMO.
 *
 * @param base_object - This is the base object ptr for DMO
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the drive management's constant
 *                        lifecycle data.
 *
 * @author
 *  26-Dec-2012:  Arun Joseph - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_drive_mgmt_delayed_state_change_callback_register_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{    
    fbe_status_t status;

    fbe_base_object_trace(base_object,  
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);   


    /* Register with the base environment for the conditions we care
     * about.
     */
    status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                              (fbe_api_notification_callback_function_t)fbe_drive_mgmt_state_change_handler,
                                                              (FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY|FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED),
                                                              (FBE_DEVICE_TYPE_DRIVE),
                                                              base_object, 
                                                              (FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE|FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE|FBE_TOPOLOGY_OBJECT_TYPE_LCC),
                                                              FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s API callback init failed, status: 0x%X",
                                __FUNCTION__, status);
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "DMO: %s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_drive_mgmt_delayed_state_change_callback_register_cond_function() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_mgmt_is_system_drive()
 ****************************************************************
 * @brief
 *  This function checks whether the drive is a system drive.
 * 
 * @param pDriveMgmt - This is the object handle.
 * @param pDrivePhysicalLocation - The pointer to the physical location of the drive.
 * @param pIsOsDrive(OUTPUT) - The pointer to the value whether this is OS drive or not.
 *
 * @return fbe_status_t  - Always return FBE_STATUS_OK.
 *
 * @author
 *  2-April-2013: - Arun Joseph. Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_is_system_drive(fbe_drive_mgmt_t * pDriveMgmt, 
                                        fbe_device_physical_location_t * pDrivePhysicalLocation,
                                        fbe_bool_t * is_system_drive)
{   
    fbe_device_physical_location_t     enclLocationForOsDrives = {0};

    
    fbe_base_env_get_encl_location_for_os_drives((fbe_base_environment_t *) pDriveMgmt, &enclLocationForOsDrives); 

    if((pDrivePhysicalLocation->bus == enclLocationForOsDrives.bus) && 
       (pDrivePhysicalLocation->enclosure == enclLocationForOsDrives.enclosure) &&
       ((pDrivePhysicalLocation->slot >= 0) &&
        (pDrivePhysicalLocation->slot <= 3)))
    {
        *is_system_drive = FBE_TRUE;
    }
    else
    {
        *is_system_drive = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_mgmt_is_system_drive() 
 ******************************************************/

/*!**************************************************************
 * @fn drive_mgmt_set_system_drive_normal_qdepth()
 ****************************************************************
 * @brief
 *  This function sets the value that is to be used for setting system drive qdepth.
 * Change in value will not affect the drives till the time they are discovered/re-discovered
 * by DMO.
 * 
 * @param queue_depth   -  Queue depth value for system drives.
 *
 * @return fbe_status_t  - FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  04-April-2013: - Arun Joseph. Created.
 *
 ****************************************************************/
static fbe_status_t drive_mgmt_set_system_drive_normal_qdepth_global(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   queue_depth)
{
    if (drive_mgmt == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt->system_drive_normal_queue_depth = queue_depth;

    return FBE_STATUS_OK;
}
/******************************************************
 * end drive_mgmt_set_system_drive_normal_qdepth() 
 ******************************************************/

/*!**************************************************************
 * @fn drive_mgmt_get_system_drive_normal_qdepth()
 ****************************************************************
 * @brief
 *  This function gets the value that is being used for setting system drive qdepth.
 * 
 * @param queue_depth   -  Queue depth value for system drives.
 *
 * @return fbe_status_t  - FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  04-April-2013: - Arun Joseph. Created.
 *
 ****************************************************************/
static fbe_status_t drive_mgmt_get_system_drive_normal_qdepth(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   *queue_depth)
{
    if ((drive_mgmt == NULL)||(queue_depth == NULL)){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *queue_depth = drive_mgmt->system_drive_normal_queue_depth;

    return FBE_STATUS_OK;
}
/******************************************************
 * end drive_mgmt_get_system_drive_normal_qdepth() 
 ******************************************************/

/*!**************************************************************
 * @fn drive_mgmt_set_system_drive_reduced_qdepth()
 ****************************************************************
 * @brief
 *  This function sets the value that is to be used for setting system drive qdepth.
 * Change in value will not affect the drives till the time they are discovered/re-discovered
 * by DMO.
 * 
 * @param queue_depth   -  Queue depth value for system drives.
 *
 * @return fbe_status_t  - FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  04-April-2013: - Arun Joseph. Created.
 *
 ****************************************************************/
static fbe_status_t drive_mgmt_set_system_drive_reduced_qdepth_global(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   queue_depth)
{
    if (drive_mgmt == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt->system_drive_reduced_queue_depth = queue_depth;

    return FBE_STATUS_OK;
}
/******************************************************
 * end drive_mgmt_set_system_drive_reduced_qdepth() 
 ******************************************************/

/*!**************************************************************
 * @fn drive_mgmt_get_system_drive_reduced_qdepth()
 ****************************************************************
 * @brief
 *  This function gets the value that is being used for setting system drive qdepth.
 * 
 * @param queue_depth   -  Queue depth value for system drives.
 *
 * @return fbe_status_t  - FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  04-April-2013: - Arun Joseph. Created.
 *
 ****************************************************************/
static fbe_status_t drive_mgmt_get_system_drive_reduced_qdepth(fbe_drive_mgmt_t   *drive_mgmt, fbe_u32_t   *queue_depth)
{
    if ((drive_mgmt == NULL)||(queue_depth == NULL)){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *queue_depth = drive_mgmt->system_drive_reduced_queue_depth;

    return FBE_STATUS_OK;
}
/******************************************************
 * end drive_mgmt_get_system_drive_reduced_qdepth() 
 ******************************************************/

/*!**************************************************************
 * fbe_drive_mgmt_handle_null_drive_in_fail_state()
 ****************************************************************
 * @brief
 *  This function handles the DMO drive information is NULL in the fail state
 *  when a drive has never reached the ready state. Try to get the drive info
 *  from PDO itself, then copy to DMO drive information.     
 *
 * @param object_handle - This is the object handle, or in our
 * case the drive_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * @param object_id - Object ID of the drive that just got added
 *
 * @return fbe_drive_info_t *  - Pointer of DMO drive information
 *
 * @author
 *  3-April-2013:  Created. chianc1
 *
 ****************************************************************/
static fbe_drive_info_t * 
fbe_drive_mgmt_handle_null_drive_in_fail_state(fbe_drive_mgmt_t *drive_mgmt, fbe_object_id_t object_id)
{
    fbe_physical_drive_information_t *physical_drive_info = NULL;
    fbe_status_t status;
    fbe_u32_t port_number, drive_number, enclosure_number;
    fbe_device_physical_location_t location;
    fbe_drive_info_t *drive_info = NULL;
    fbe_lifecycle_state_t current_state;
    
    status = fbe_api_get_object_lifecycle_state(object_id, &current_state, FBE_PACKAGE_ID_PHYSICAL);
    if (current_state != FBE_LIFECYCLE_STATE_FAIL)
    {
        /* This function is only called in the fail state. */
        return NULL;
    }
    
    status = fbe_api_get_object_port_number (object_id, &port_number);
    if (status != FBE_STATUS_OK) {
        return NULL;
    }
    
    status = fbe_api_get_object_enclosure_number(object_id, &enclosure_number);
    if (status != FBE_STATUS_OK) {
        return NULL;
    }
    
    status = fbe_api_get_object_drive_number (object_id, &drive_number);
    if (status != FBE_STATUS_OK) {
        return NULL;
    }
 
    physical_drive_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_physical_drive_information_t));
    if(physical_drive_info == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Memory Allocation failed\n",
                              __FUNCTION__);
        return NULL;
    }
 
    status = fbe_api_physical_drive_get_drive_information(object_id, physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
        return NULL;
    }
    
    location.bus = port_number;
    location.enclosure = enclosure_number;
    location.slot = drive_number;
 
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt,  
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %d_%d_%d state: %d ID:%s %s \n",
                          location.bus, location.enclosure,location.slot, current_state,physical_drive_info->vendor_id, __FUNCTION__);
    
    /* Find a free entry and then update local state*/
    status = fbe_drive_mgmt_get_drive_info_pointer(drive_mgmt, &location, &drive_info);
    if(drive_info == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Free handle not found\n",
                              __FUNCTION__);
        /* Free entry not found and so return an error */
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
        return NULL;
    }
    
    /* It is in fail state, only copy the information is needed. */
    fbe_zero_memory(drive_info, sizeof(fbe_drive_info_t));
    
    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
    drive_info->object_id = object_id;
    drive_info->state = current_state;
    drive_info->location.bus = port_number;
    drive_info->location.enclosure = enclosure_number;
    drive_info->location.slot = drive_number;
    fbe_copy_memory(&drive_info->vendor_id, &physical_drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
    drive_info->vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE] = '\0';
    fbe_copy_memory(&drive_info->tla, &physical_drive_info->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE);
    drive_info->tla[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0';
    fbe_copy_memory(&drive_info->rev, &physical_drive_info->product_rev, physical_drive_info->product_rev_len);
    drive_info->rev[physical_drive_info->product_rev_len] = '\0';
    fbe_copy_memory(&drive_info->sn, &physical_drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    drive_info->sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0';
    fbe_copy_memory(&drive_info->drive_description_buff, &physical_drive_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE);
    drive_info->drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE] = '\0';
    fbe_copy_memory(&drive_info->bridge_hw_rev, &physical_drive_info->bridge_hw_rev, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE);
    drive_info->bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE] = '\0';
    fbe_copy_memory(&drive_info->product_id, &physical_drive_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    drive_info->product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE] = '\0';
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
 
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, physical_drive_info);
 
    return drive_info;
}
/******************************************************
 * end fbe_drive_mgmt_handle_null_drive_in_fail_state() 
 *****************************************************/       
