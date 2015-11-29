#ifndef FBE_API_COMMON_H
#define FBE_API_COMMON_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_common.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_common.
 * 
 * @ingroup fbe_api_system_package_interface_class_files
 * 
 * @version
 *   10/10/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_payload_control_operation.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_api_common_job_notification.h"
#include "fbe/fbe_job_service_interface.h"
#include "fbe/fbe_notification_lib.h"
#ifdef C4_INTEGRATED
#include "fbe/fbe_enclosure.h"
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */


#define FBE_API_TOTAL_PRE_ALLOCATED_PACKETS 200
#define FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS 1
#define FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK (FBE_API_TOTAL_PRE_ALLOCATED_PACKETS / FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS)
#define FBE_API_PRE_ALLOCATED_OS_PACKETS 100

//----------------------------------------------------------------
// Define the top level group for the FBE System APIs.
// This needs to be here because some of the functions define in
// the FBE API Discovery Interface are really used by the FBE API
// System. 
//----------------------------------------------------------------
/*! @defgroup fbe_system_package_class FBE System APIs 
 *  @brief 
 *    This is the set of definitions for FBE System APIs.
 * 
 *  @ingroup fbe_api_system_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API System Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface_usurper_interface FBE API System Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API System class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @struct update_object_msg_t 
 *  
 * @brief 
 *   This contains the data info for Update Object Message structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup update_object_msg
 **********************************************************************/
typedef struct update_object_msg_s {
    fbe_queue_element_t         queue_element;      /*!< Queue Element */
    fbe_object_id_t             object_id;          /*!< Object ID*/
	/* FBE30 temporary */
    fbe_topology_object_type_t  object_type;            
    fbe_s32_t                   port;
    fbe_s32_t                   encl;
    fbe_s32_t                   encl_addr;
    fbe_s32_t                   drive;
	/* FBE30 temporary */
    fbe_notification_info_t     notification_info;  /*!< Notification Info */
}update_object_msg_t;


static __forceinline update_object_msg_t * 
fbe_api_notification_queue_element_to_update_object_msg(fbe_queue_element_t * queue_element)
{
    update_object_msg_t * msg;
    msg = (update_object_msg_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((update_object_msg_t  *)0)->queue_element));
    return msg;
}


/*!********************************************************************* 
 * @typedef void (* event_notification_function_t)(void)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef void (* event_notification_function_t)(void);

/*!********************************************************************* 
 * @typedef void (* commmand_response_callback_function_t) (void * context)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef void (* commmand_response_callback_function_t) (void * context);
/*!********************************************************************* 
 * @typedef void (* fbe_api_notification_callback_function_t) (update_object_msg_t * update_object_msg, void * context)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef void (* fbe_api_notification_callback_function_t) (update_object_msg_t * update_object_msg, void * context);
/*!********************************************************************* 
 * @typedef fbe_status_t (* fbe_api_common_send_control_packet_func) (fbe_packet_t *packet)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef fbe_status_t (* fbe_api_common_send_control_packet_func) (fbe_packet_t *packet);
/*!********************************************************************* 
 * @typedef fbe_status_t (* fbe_api_common_send_io_packet_func) (fbe_packet_t *packet)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef fbe_status_t (* fbe_api_common_send_io_packet_func) (fbe_packet_t *packet);
/*!********************************************************************* 
 * @typedef void (* fbe_api_sleep_func)(fbe_u32_t msec)
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef void (* fbe_api_sleep_func)(fbe_u32_t msec);


/*!********************************************************************* 
 * @struct fbe_api_control_operation_status_info_t 
 *  
 * @brief 
 *   This contains the data info for Control Operation Status Info structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup fbe_api_control_operation_status_info
 **********************************************************************/
typedef struct fbe_api_control_operation_status_info_s{
    fbe_payload_control_status_t            control_operation_status;    /*!< Control Operation Status */
    fbe_payload_control_status_qualifier_t  control_operation_qualifier; /*!< Control Operation Qualifier */
    fbe_status_t                            packet_status;               /*!< Packet Status */
    fbe_u32_t                               packet_qualifier;            /*!< Packet Qualifier */
}fbe_api_control_operation_status_info_t;

/*!********************************************************************* 
 * @struct fbe_api_lifecycle_debug_trace_control_t 
 *  
 * @brief 
 *   This contains the data info for lifecycle debug trace Control structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup fbe_api_lifecycle_debug_trace_control
 **********************************************************************/
typedef struct fbe_api_lifecycle_debug_trace_control_s {
    fbe_lifecycle_state_debug_tracing_flags_t     trace_flag;   /*!< Trace Flag */
} fbe_api_lifecycle_debug_trace_control_t;

/*!********************************************************************* 
 * @struct fbe_api_trace_level_control_t 
 *  
 * @brief 
 *   This contains the data info for Trace Level Control structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup fbe_api_trace_level_control
 **********************************************************************/
typedef struct fbe_api_trace_level_control_s {
    fbe_trace_type_t      trace_type;   /*!< Trace Type */
    fbe_u32_t             fbe_id;       /*!< FBE ID */
    fbe_trace_level_t     trace_level;  /*!< Trace Level */
    fbe_trace_flags_t     trace_flag;   /*!< Trace Flag */
} fbe_api_trace_level_control_t;

/*!********************************************************************* 
 * @struct fbe_api_trace_error_counters_t 
 *  
 * @brief 
 *   This contains the data info for Trace Error Counters structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup fbe_api_trace_error_counters
 **********************************************************************/
typedef struct fbe_api_trace_error_counters_s {
    fbe_u32_t   trace_error_counter;          /*!< Trace Error Counter */
    fbe_u32_t   trace_critical_error_counter; /*!< Trace Critical Error Counter */
} fbe_api_trace_error_counters_t;


/*!*******************************************************************
 * @enum fbe_api_trace_error_limit_t
 *********************************************************************
 * @brief This describes the type of action to take when we
 *        reach a given limit of error counts.
 *
 *********************************************************************/
typedef enum fbe_api_trace_error_limit_action_e {
	FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID = 0,

    /*! We simply trace when we reach this limit.
     */
    FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,

    /*! Bring the system down if we see this limit.
     */
    FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
	FBE_API_TRACE_ERROR_LIMIT_ACTION_LAST
} fbe_api_trace_error_limit_action_t;

/*!*******************************************************************
 * @struct fbe_api_trace_get_error_limit_record_t
 *********************************************************************
 * @brief This structure is used as part of the 
 *        FBE_TRACE_CONTROL_CODE_GET_ERROR_limit usurper command
 *        to control what the system will do for a given trace level.
 *********************************************************************/
typedef struct fbe_api_trace_get_error_limit_record_s {

    /*! Action to take when we reach this limit.
     */
    fbe_api_trace_error_limit_action_t action;

    /*! The number of errors encountered at this level.
     */
    fbe_u32_t num_errors;

    /*! The number of errors needed to reach this limit.
     */
    fbe_u32_t error_limit;

} fbe_api_trace_get_error_limit_record_t;

/*!*******************************************************************
 * @struct fbe_api_trace_get_error_limit_t
 *********************************************************************
 * @brief This structure is used as part of the 
 *        FBE_TRACE_CONTROL_CODE_GET_ERROR_limit usurper command
 *        to control what the system will do for a given trace level.
 *********************************************************************/
typedef struct fbe_api_trace_get_error_limit_s {
    
    /*! The array of records, that describe the error limits 
     * for each trace level. 
     */ 
    fbe_api_trace_get_error_limit_record_t records[FBE_TRACE_LEVEL_LAST];

} fbe_api_trace_get_error_limit_t;

/*!********************************************************************* 
 * @struct fbe_api_libs_function_entries_t 
 *  
 * @brief 
 *   This contains the data info for Library Functions structure.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup fbe_api_libs_function_entries
 **********************************************************************/
typedef struct fbe_api_libs_function_entries_s{
    fbe_api_common_send_control_packet_func lib_control_entry; /*!< Send Control Packet Function */
    fbe_api_common_send_io_packet_func      lib_io_entry;      /*!< Send IO Packet Function */
    fbe_u32_t                               magic_number;      /*!< Magic Number */
}fbe_api_libs_function_entries_t;

/*! @} */ /* end of group fbe_api_system_interface_usurper_interface */

#ifdef C4_INTEGRATED
/* 
 * IOCTLs for FBE_CLI are used for interacting with ppfd.
 */
#if 0
#define FBE_CLI_IOCTL_SEND_PKT     0x40000008
#define FBE_CLI_IOCTL_SEND_PKT_SGL 0x40000009
#define FBE_CLI_IOCTL_GET_MAP      0x4000000a
#endif

#define FBE_CLI_MAP_INTERFACE_GET_PORT 0
#define FBE_CLI_MAP_INTERFACE_GET_ENCL 1
#define FBE_CLI_MAP_INTERFACE_GET_LDRV 2
#define FBE_CLI_MAP_INTERFACE_GET_PDRV 3
#define FBE_CLI_MAP_INTERFACE_GET_OBJL 4
#define FBE_CLI_MAP_INTERFACE_GET_BORD 5
#define FBE_CLI_MAP_INTERFACE_GET_TYPE 6
#define FBE_CLI_MAP_INTERFACE_GET_NUML 7
#define FBE_CLI_MAP_INTERFACE_GET_EADD 8
#define FBE_CLI_MAP_INTERFACE_GET_EPOS 9

typedef union {
        struct {
            fbe_u32_t       val32_1;
            fbe_u32_t       val32_2;
        };
        fbe_u64_t       val64;
} fbe_api_fbe_cli_rdevice_output_value_t;


fbe_status_t fbe_api_common_init_from_fbe_cli (csx_module_context_t csx_mcontext);
csx_status_e fbe_api_common_packet_from_fbe_cli (fbe_u32_t ioctl_code, csx_pvoid_t inBuffPtr, csx_pvoid_t outBuffPtr);
csx_status_e fbe_api_common_map_interface_get_from_fbe_cli (csx_pvoid_t inBuffPtr, csx_pvoid_t outBuffPtr);
fbe_status_t fbe_api_object_map_interface_get_for_fbe_cli (fbe_u32_t fbe_cli_map_interface_code, 
                                                           fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num,fbe_object_id_t object_id, fbe_u32_t encl_addr, fbe_api_fbe_cli_rdevice_output_value_t *requested_value);

CSX_EXTERN csx_p_rdevice_reference_t ppDeviceRef;
CSX_EXTERN int  call_from_fbe_cli_user;

typedef struct fbe_api_fbe_cli_rdevice_inbuff_s
{
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_u32_t                               fbe_cli_map_interface_code;
    fbe_u32_t                               port_num;
    fbe_u32_t                               encl_num;
    fbe_u32_t                               disk_num;
    fbe_u32_t                               encl_addr;
    fbe_object_id_t                         object_id;
    fbe_packet_attr_t                       attr;
    fbe_package_id_t                        package_id;
    fbe_payload_control_buffer_length_t     buffer_length;
    fbe_payload_control_buffer_length_t     sgl0_length;
} fbe_api_fbe_cli_rdevice_inbuff_t;

typedef struct fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_s
{
    fbe_status_t    fbe_status_rc;
    fbe_api_fbe_cli_rdevice_output_value_t requested_value;
} fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t;

typedef struct fbe_api_fbe_cli_rdevice_outbuff_s
{
    fbe_api_control_operation_status_info_t control_status_info;
} fbe_api_fbe_cli_rdevice_outbuff_t;

typedef struct fbe_api_fbe_cli_rdevice_read_buffer_s
{
    fbe_object_id_t              object_id;
    fbe_enclosure_mgmt_ctrl_op_t operation;
    fbe_base_enclosure_control_code_t control_code;
    fbe_enclosure_status_t       enclosure_status;
} fbe_api_fbe_cli_rdevice_read_buffer_t;
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */

const fbe_u8_t *
fbe_api_job_service_notification_error_code_to_string(fbe_job_service_error_type_t job_error_code);

/*use this macro and the closing one: FBE_API_CPP_EXPORT_END to export your functions to C++ applications*/
FBE_API_CPP_EXPORT_START


//----------------------------------------------------------------
// Define the group for the FBE API Common Kernel Interface.  
// This is where all the function prototypes for the Common Kernel API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_common_kernel_interface FBE API Common Kernel Interface
 *  @brief 
 *    This is the set of FBE API Common Kernel calls.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_common.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_common_init_kernel (void);
fbe_status_t FBE_API_CALL fbe_api_common_destroy_kernel (void);
/*! @} */ /* end of group fbe_api_common_kernel_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Common Sim Interface.  
// This is where all the function prototypes for the Common Sim API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_common_sim_interface FBE API Common Sim Interface
 *  @brief 
 *    This is the set of FBE API Common Sim calls.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_common.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_common_init_sim (void);
fbe_status_t FBE_API_CALL fbe_api_common_destroy_sim (void);
fbe_status_t FBE_API_CALL fbe_api_set_simulation_io_and_control_entries (fbe_package_id_t package_id,
                                                                         fbe_io_entry_function_t io_entry_function,
                                                                         fbe_service_control_entry_t control_entry);
/*! @} */ /* end of group fbe_api_common_sim_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Common User Interface.  
// This is where all the function prototypes for the Common User API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_common_user_interface FBE API Common User Interface
 *  @brief 
 *    This is the set of FBE API Common User calls.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_common.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_common_init_user (fbe_bool_t notification_init);
fbe_status_t FBE_API_CALL fbe_api_common_destroy_user (void);
fbe_status_t FBE_API_CALL fbe_api_common_set_function_entries (fbe_api_libs_function_entries_t * entries, fbe_package_id_t package_id);
fbe_queue_head_t * fbe_api_common_get_contiguous_packet_q_head(void);
fbe_spinlock_t * fbe_api_common_get_contiguous_packet_q_lock(void);
fbe_queue_head_t * fbe_api_common_get_outstanding_contiguous_packet_q_head(void);

fbe_status_t fbe_api_common_destroy_cluster_lock_user (void);

/*! @} */ /* end of group fbe_api_common_user_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Common Interface.  
// This is where all the function prototypes for the Common API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_common_interface FBE API Common Interface
 *  @brief 
 *    This is the set of FBE API system calls.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_common.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
void FBE_API_CALL fbe_api_trace(fbe_trace_level_t trace_level, const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,2,3)));
void FBE_API_CALL fbe_api_sleep (fbe_u32_t msec);
const fbe_u8_t * FBE_API_CALL fbe_api_state_to_string (fbe_lifecycle_state_t state);
const fbe_u8_t * FBE_API_CALL fbe_api_traffic_priority_to_string (fbe_traffic_priority_t traffic_priority_in);

fbe_status_t FBE_API_CALL fbe_api_trace_get_level (fbe_api_trace_level_control_t * p_level_control, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_set_level (fbe_api_trace_level_control_t * p_level_control, fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_trace_get_error_counter(fbe_api_trace_error_counters_t * p_error_counters, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_clear_error_counter(fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_trace_get_flags (fbe_api_trace_level_control_t * p_flag_control, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_set_flags (fbe_api_trace_level_control_t * p_flag_control, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_esp_getObjectId(fbe_class_id_t espClassId, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_trace_set_error_limit(fbe_trace_level_t trace_level,
                                                        fbe_api_trace_error_limit_action_t action,
                                                        fbe_u32_t num_errors,
                                                        fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_get_error_limit(fbe_package_id_t package_id,
                                                        fbe_api_trace_get_error_limit_t *error_limit_p);
fbe_status_t FBE_API_CALL fbe_api_traffic_trace_enable(fbe_u32_t object_tag,
                                                        fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_traffic_trace_disable(fbe_u32_t object_tag,
                                                        fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_common_package_entry_initialized (fbe_package_id_t package_id, fbe_bool_t *initialized);

fbe_status_t FBE_API_CALL fbe_api_lifecycle_set_debug_trace_flag (fbe_api_lifecycle_debug_trace_control_t* p_trace_control, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_command_to_ktrace_buff(fbe_u8_t *cmd, fbe_u32_t length); 
fbe_status_t FBE_API_CALL fbe_api_disable_object_lifecycle_debug_trace(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_enable_object_lifecycle_debug_trace(fbe_object_id_t object_id);

/*! @} */ /* end of group fbe_api_common_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Common Notification Interface.  
// This is where all the function prototypes for the Common API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_common_notification_interface FBE API Common Notification Interface
 *  @brief 
 *    This is the set of FBE API Notification calls.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_common.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_notification_interface_init (void);
fbe_status_t FBE_API_CALL fbe_api_notification_interface_destroy (void);
fbe_status_t FBE_API_CALL fbe_api_notification_interface_register_notification(fbe_notification_type_t state_mask,
                                                                                fbe_package_notification_id_mask_t package_mask,
                                                                                fbe_topology_object_type_t object_type_mask,
                                                                                fbe_api_notification_callback_function_t callback_func,
                                                                                void * context,
																			    fbe_notification_registration_id_t * registration_id);

fbe_status_t FBE_API_CALL fbe_api_notification_interface_unregister_notification(fbe_api_notification_callback_function_t callback_func,
																				 fbe_notification_registration_id_t user_registration_id);

#if 0 /*depracated*/
fbe_package_id_t FBE_API_CALL fbe_api_notification_translate_api_package_to_fbe_package (fbe_package_notification_id_mask_t package_id);
#endif /*depracated*/

/*! @} */ /* end of group fbe_api_common_notification_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for the FBE API System Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_package_interface_class_files FBE System APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE System Package Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------


#endif /*FBE_API_COMMON_H*/
