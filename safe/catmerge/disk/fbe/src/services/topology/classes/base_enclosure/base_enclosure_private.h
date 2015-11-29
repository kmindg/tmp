#ifndef BASE_ENCLOSURE_PRIVATE_H
#define BASE_ENCLOSURE_PRIVATE_H

#include "fbe_base_enclosure.h"
#include "fbe/fbe_physical_drive.h"
#include "base_discovering_private.h"
#include "fbe_enclosure_data_access_public.h"
#include "edal_base_enclosure_data.h"
#include "fbe_notification_interface.h"
#include "fbe_notification.h"

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_enclosure);


/*
 * See ARS: 423962 for context on this problem.
 *
 * The following is used to conditionally compile the base object so
 * that it enables or disables the edal cache pointer.  There was an
 * optimization made that uses a simple cache to help optimize
 * accesses to the edal data block.  However this cache is not
 * protected by a lock (which is considered too expensive).  Some
 * problems have been seen where this data is overwritten by one
 * client while another client is using it.  The result is that the
 * wrong data can be read or written via edal.  The case in point
 * caused the enclosure to become faulted on one side. 
 * 
 * There is some concern that removal of this optimization will result
 * in poor performance especially since some of these calls occur in
 * DPC context.  For that reason this is being removed using a compile
 * time directive.  Setting the following define to 0 will disable the
 * cache optimization and setting it to 1 will re-enable the
 * optimization.
 * 
 */
#define ENABLE_EDAL_CACHE_POINTER   0

#define FBE_TICK_PER_SECOND    1000
// This define is used for log_header and must be big enough to store the
// strings generated.  For example: Enc:0_1.3
// Currently this is sufficient as long as we have single digits for
// bus, encl and connector_id.
#define FBE_BASE_ENCL_HEADER   10

/* FBE_CMD_FAIL returns string if failed or retry
 */
#define FBE_CMD_FAILED(val) ((val == FBE_CMD_FAIL) ? "FAILED" : "RETRY")

// Command Retry Enum
typedef enum
{
    FBE_CMD_NO_ACTION = 0,
    FBE_CMD_RETRY,
    FBE_CMD_FAIL
} fbe_cmd_retry_code_t;

/* These are the lifecycle condition ids for a base enclosure object. 
 * This order must be the same as in FBE_LIFECYCLE_BASE_COND_ARRAY(base_enclosure)
 */
typedef enum fbe_base_enclosure_lifecycle_cond_id_e {
    /* specialize rotary conditions */
    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_SERVER_INFO_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_ENCLOSURE),

    /* activate rotary conditions */
    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_INIT_RIDE_THROUGH,
    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_RIDE_THROUGH_TICK,

    /* Ready rotary conditions */
    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD,

    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_DRIVE_SPINUP_CTRL_NEEDED,

    FBE_BASE_ENCLOSURE_LIFECYCLE_COND_LAST /* must be last */
} fbe_base_enclosure_lifecycle_cond_id_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_component_accessed_data_t
 *  
 * @brief 
 *   describes the component accessed data associated with an enclosure. 
 *   More elements may get added to this structure.
 **********************************************************************/
typedef struct fbe_enclosure_component_accessed_data_s {
     fbe_enclosure_component_types_t component;
     fbe_u32_t index;
     fbe_edal_component_data_handle_t specificCompPtr;     
} fbe_enclosure_component_accessed_data_t;

typedef fbe_status_t (* fbe_base_enclosure_lifecycle_set_cond_function_t)(fbe_lifecycle_const_t * p_class_const,struct fbe_base_object_s * p_object,fbe_lifecycle_state_t new_state);
typedef struct fbe_base_enclosure_s {
    fbe_base_discovering_t              base_discovering;
    fbe_enclosure_type_t                enclosure_type;
    fbe_u64_t                   lcc_device_type;
    fbe_enclosure_number_t              enclosure_number; /* enclosure number is for logging only */
    fbe_address_t                       server_address;   /* address for the server component */
    fbe_enclosure_position_t            my_position;      /* the position assigned to me by my upstream parent */ 
    fbe_port_number_t                   port_number;      /* port number is for logging only */
    fbe_enclosure_component_id_t        component_id;     /* component id, 0 for primary component */
    fbe_number_of_slots_t               number_of_slots;
    fbe_number_of_slots_t               number_of_slots_per_bank;
    fbe_edge_index_t                    first_slot_index;
    fbe_edge_index_t                    first_expansion_port_index;
    fbe_u32_t                           number_of_expansion_ports;
    fbe_u32_t                           number_of_drives_spinningup;
    fbe_u32_t                           max_number_of_drives_spinningup;
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_ENCLOSURE_LIFECYCLE_COND_LAST));

    // Component Information for enclosure (defined by Base Enclosure, but filled in by specific enclosure
//    fbe_u32_t                           numberOfComponentTypes;
    fbe_enclosure_component_block_t     *enclosureComponentTypeBlock;    // pointer to enclosure Component Type Info
    fbe_enclosure_component_block_t     *enclosureComponentTypeBlock_backup; //pointer to Backup enclosure component Type Info 
    fbe_bool_t                                        active_edal_valid;                        //to confirm which copy EDAL to use
//    fbe_encl_component_general_info_t   *enclosureComponentData;        // pointer to enclosure Component Info

    /* the following four is needed for riding through enclosure reset
     * It tracks when ride through first detected and how long
     * before the enclosure should come back.
     */
    fbe_time_t          time_ride_through_start;
    fbe_u32_t           expected_ride_through_period;
    fbe_u32_t           default_ride_through_period;  /* filled in by leaf class at init time.
                                                       * If someone changes this after enclosure
                                                       * being specialized, and there's a potential
                                                       * of ongoing ride through, he needs to set 
                                                       * base_enclosure_init_ride_through_cond to
                                                       * get the timer adjusted.
                                                       */
    fbe_u32_t           condition_retry_count;
    fbe_bool_t                  power_save_supported;
    fbe_u8_t                    logHeader[FBE_BASE_ENCL_HEADER];

    /* the following three data elements and function pointer are needed for
     * error handling of EDAL errors, SCSI errors
     */
    fbe_enclosure_fault_t                               enclosure_faults;
    fbe_base_enclosure_lifecycle_set_cond_function_t    lifecycle_set_condition_routine;
    fbe_enclosure_scsi_error_info_t                     scsi_error_info;    
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t             enclosure_component_access_data;
#endif
    fbe_payload_discovery_opcode_t discovery_opcode;	
    fbe_payload_control_operation_opcode_t control_code;
}fbe_base_enclosure_t;

/*
 * Structure for accessing Enclosure Object data
 */
typedef enum fbe_base_enclosure_data_types_e
{
    ATTRIBUTE_BOOLEAN,
    ATTRIBUTE_U8,
    ATTRIBUTE_U16,
    ATTRIBUTE_U32,
    ATTRIBUTE_U64,
    ATTRIBUTE_STRING
} fbe_base_enclosure_data_types_t;

typedef struct fbe_base_enclosure_data_s
{
//    fbe_base_enclosure_attributes_t     attribute;
    fbe_base_enclosure_data_types_t     dataType;
    fbe_enclosure_component_types_t     componentType;
} fbe_base_enclosure_data_t;


/* Methods */
fbe_status_t fbe_base_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_base_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_enclosure_monitor_load_verify(void);

fbe_status_t fbe_base_enclosure_set_my_position(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_number_t enclosure_number);
fbe_status_t fbe_base_enclosure_get_my_position(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_number_t * enclosure_number);

// No set enclosure_number because it is handled internally
fbe_status_t fbe_base_enclosure_get_enclosure_number(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_number_t * enclosure_number);

fbe_status_t fbe_base_enclosure_set_port_number(fbe_base_enclosure_t * base_enclosure, fbe_port_number_t port_number);
fbe_status_t fbe_base_enclosure_get_port_number(fbe_base_enclosure_t * base_enclosure, fbe_port_number_t * port_number);

fbe_status_t fbe_base_enclosure_set_activeEdalValid(fbe_base_enclosure_t * base_enclosure, fbe_bool_t active_edal_valid);
fbe_status_t fbe_base_enclosure_get_activeEdalValid(fbe_base_enclosure_t * base_enclosure, fbe_bool_t * active_edal_valid);
fbe_status_t fbe_base_enclosure_set_logheader(fbe_base_enclosure_t * base_enclosure);
fbe_u8_t * fbe_base_enclosure_get_logheader(fbe_base_enclosure_t * base_enclosure);

fbe_status_t fbe_base_enclosure_set_server_address(fbe_base_enclosure_t * base_enclosure, fbe_address_t server_address);
fbe_status_t fbe_base_enclosure_get_server_address(fbe_base_enclosure_t * base_enclosure, fbe_address_t * server_address);

fbe_status_t fbe_base_enclosure_get_first_slot_index(fbe_base_enclosure_t * base_enclosure, fbe_edge_index_t * first_slot_index);
fbe_status_t fbe_base_enclosure_set_first_slot_index(fbe_base_enclosure_t * base_enclosure, fbe_edge_index_t first_slot_index);

fbe_status_t fbe_base_enclosure_get_number_of_expansion_ports(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * number_of_expansion_ports);
fbe_status_t fbe_base_enclosure_set_number_of_expansion_ports(fbe_base_enclosure_t * base_enclosure, fbe_u32_t number_of_expansion_ports);

fbe_status_t fbe_base_enclosure_get_first_expansion_port_index(fbe_base_enclosure_t * base_enclosure, fbe_edge_index_t * first_expansion_port_index);
fbe_status_t fbe_base_enclosure_set_first_expansion_port_index(fbe_base_enclosure_t * base_enclosure, fbe_edge_index_t first_expansion_port_index);

fbe_status_t fbe_base_enclosure_get_default_ride_through_period(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * default_ride_through_period);
fbe_status_t fbe_base_enclosure_set_default_ride_through_period(fbe_base_enclosure_t * base_enclosure, fbe_u32_t default_ride_through_period);

fbe_status_t fbe_base_enclosure_get_expected_ride_through_period(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * expected_ride_through_period);
fbe_status_t fbe_base_enclosure_set_expected_ride_through_period(fbe_base_enclosure_t * base_enclosure, fbe_u32_t expected_ride_through_period);

fbe_status_t fbe_base_enclosure_get_time_ride_through_start(fbe_base_enclosure_t * base_enclosure, fbe_time_t * time_ride_through_start);
fbe_status_t fbe_base_enclosure_set_time_ride_through_start(fbe_base_enclosure_t * base_enclosure, fbe_time_t time_ride_through_start);
fbe_status_t fbe_base_enclosure_record_control_opcode(fbe_base_enclosure_t * base_enclosure, fbe_payload_control_operation_opcode_t control_code);
fbe_status_t fbe_base_enclosure_record_discovery_transport_opcode(fbe_base_enclosure_t * base_enclosure,fbe_payload_discovery_opcode_t discovery_opcode);
fbe_status_t fbe_base_enclosure_clear_condition_retry_count(fbe_base_enclosure_t * base_enclosure);
fbe_u32_t    fbe_base_enclosure_increment_condition_retry_count(fbe_base_enclosure_t * base_enclosure);

fbe_status_t fbe_base_enclosure_increment_ride_through_count(fbe_base_enclosure_t * base_enclosure);
fbe_status_t fbe_base_enclosure_get_ride_through_count(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * ride_through_count);
fbe_status_t fbe_base_enclosure_set_ride_through_count(fbe_base_enclosure_t * base_enclosure, fbe_u32_t ride_through_count);

fbe_status_t fbe_base_enclosure_set_component_block_ptr(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_component_block_t *encl_component_block);
fbe_status_t fbe_base_enclosure_set_component_block_backup_ptr(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_component_block_t *encl_component_block);    
fbe_status_t fbe_base_enclosure_get_component_block_ptr(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_component_block_t **encl_component_block);
fbe_status_t fbe_base_enclosure_get_component_block_backup_ptr(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_component_block_t **encl_component_block_backup);

fbe_edal_status_t
fbe_base_enclosure_copy_edal_to_backup(fbe_base_enclosure_t * base_enclosure);

fbe_enclosure_status_t fbe_base_enclosure_set_lifecycle_set_cond_routine(fbe_base_enclosure_t * base_enclosure, 
                                   fbe_base_enclosure_lifecycle_set_cond_function_t lifecycle_set_cond_routine);
fbe_enclosure_status_t fbe_base_enclosure_set_lifecycle_cond(fbe_lifecycle_const_t * p_class_const,
                                                   fbe_base_enclosure_t * base_enclosure,
                                                   fbe_lifecycle_state_t new_state);

fbe_enclosure_status_t fbe_base_enclosure_handle_errors(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_u32_t component,
                                                    fbe_u32_t index,
                                                    fbe_u32_t fault_symptom,
                                                    fbe_u32_t addl_status);

fbe_enclosure_status_t fbe_base_enclosure_get_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_enclosure_fault_t * encl_fault_info);

fbe_enclosure_status_t fbe_base_enclosure_set_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_u32_t component,
                                                    fbe_u32_t index,
                                                    fbe_u32_t fault_symptom,
                                                    fbe_u32_t addl_status);

fbe_enclosure_status_t fbe_base_enclosure_get_mgmt_basic_info(fbe_base_enclosure_t * base_enclosure,
                                      fbe_base_object_mgmt_get_enclosure_basic_info_t  *mgmt_basic_info);

fbe_enclosure_status_t fbe_base_enclosure_get_scsi_error_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_enclosure_scsi_error_info_t  *scsi_error_info);

fbe_enclosure_status_t fbe_base_enclosure_set_scsi_error_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_u32_t scsi_error_code,
                                                    fbe_u32_t scsi_status,
                                                    fbe_u32_t payload_request_status,
                                                    fbe_u8_t * sense_data);
fbe_enclosure_status_t fbe_base_enclosure_get_edal_locale(fbe_base_enclosure_t * base_enclosure, 
                                                    fbe_u8_t *edalLocalPtr);

fbe_status_t fbe_base_enclosure_access_specific_component(fbe_base_enclosure_t *enclosurePtr,
                                             fbe_enclosure_component_types_t componentType,
                                             fbe_u8_t index,
                                             void **encl_component);

fbe_status_t
fbe_base_enclosure_get_number_of_components(fbe_base_enclosure_t * base_enclosure,
                                             fbe_enclosure_component_types_t component_type,
                                             fbe_u8_t * number_of_components);
fbe_u32_t 
    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(fbe_enclosure_status_t encl_stat);

fbe_lifecycle_status_t base_enclosure_init_ride_through_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);


fbe_enclosure_status_t fbe_base_enclosure_translate_edal_status(fbe_edal_status_t edal_status);
/* regular edal access routine 
 */
fbe_enclosure_status_t
fbe_base_enclosure_edal_getOverallStatus(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_component_types_t component,
                                         fbe_u32_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setOverallStatus(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_component_types_t component,
                                         fbe_u32_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getBool(fbe_base_enclosure_t * base_enclosure,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t component,
                                fbe_u32_t index,
                                fbe_bool_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setBool(fbe_base_enclosure_t * base_enclosure,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t component,
                                fbe_u32_t index,
                                fbe_bool_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU8(fbe_base_enclosure_t * base_enclosure,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t index,
                              fbe_u8_t *returnValuePtr);

fbe_enclosure_status_t
fbe_base_enclosure_edal_incrementStateChangeCount(fbe_base_enclosure_t * base_enclosure);

fbe_enclosure_status_t
fbe_base_enclosure_edal_incrementGenerationCount(fbe_base_enclosure_t * base_enclosure);

fbe_enclosure_status_t
fbe_base_enclosure_edal_get_generationCount(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * generation_count);

fbe_enclosure_status_t
fbe_base_enclosure_edal_setU8(fbe_base_enclosure_t * base_enclosure,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t index,
                              fbe_u8_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU16(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u16_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU16(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u16_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU32(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU32(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU64(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u64_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU64(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u64_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getStr(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t length,
                               char *returnStringPtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setStr(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t length,
                               char *setString);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getOverallStateChangeCount(fbe_base_enclosure_t * base_enclosure,
                                                   fbe_u32_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getEnclosureSide(fbe_base_enclosure_t * base_enclosure,
                                         fbe_u8_t *returnValuePtr);

fbe_enclosure_status_t
fbe_base_enclosure_edal_getEnclosureType(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_types_t *returnValuePtr);

fbe_enclosure_status_t
fbe_base_enclosure_edal_printUpdatedComponent(fbe_base_enclosure_t * base_enclosure,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t index);

fbe_enclosure_status_t
fbe_base_enclosure_edal_getTempSensorBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                                      fbe_base_enclosure_attributes_t attribute,
                                                      fbe_u32_t index,
                                                      fbe_bool_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getTempSensorU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                                     fbe_base_enclosure_attributes_t attribute,
                                                     fbe_u32_t index,
                                                     fbe_u16_t *returnValuePtr);

// these function should be used by usurper functions to avoid data corruption
fbe_enclosure_status_t
fbe_base_enclosure_edal_getBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t index,
                                            fbe_bool_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t index,
                                            fbe_bool_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                          fbe_base_enclosure_attributes_t attribute,
                                          fbe_enclosure_component_types_t component,
                                          fbe_u32_t index,
                                          fbe_u8_t *returnValuePtr);

fbe_enclosure_status_t
fbe_base_enclosure_edal_setU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                          fbe_base_enclosure_attributes_t attribute,
                                          fbe_enclosure_component_types_t component,
                                          fbe_u32_t index,
                                          fbe_u8_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u16_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u16_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u64_t *returnValuePtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u64_t setValue);
fbe_enclosure_status_t
fbe_base_enclosure_edal_getStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t length,
                                           char *returnStringPtr);
fbe_enclosure_status_t
fbe_base_enclosure_edal_setStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t length,
                                           char *setString);

/* edal search utilities
 */
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_Bool(fbe_base_enclosure_t * base_enclosure,
                                              fbe_base_enclosure_attributes_t attribute,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t start_index,
                                              fbe_bool_t check_value);
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U8(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t start_index,
                                            fbe_u8_t check_value);
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U16(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u16_t check_value);

fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U32(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u32_t check_value);
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U64(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u64_t check_value);

fbe_status_t fbe_base_enclosure_get_fw_bytes_sent(fbe_base_enclosure_t *base_enclosure, fbe_u32_t *bytes_sent);
fbe_status_t fbe_base_enclosure_set_fw_bytes_sent(fbe_base_enclosure_t *base_enclosure, fbe_u32_t bytes_sent);

fbe_status_t fbe_base_enclosure_get_fw_image_size(fbe_base_enclosure_t *base_enclosure, fbe_u32_t *image_size);
fbe_status_t fbe_base_enclosure_get_fw_image_pointer(fbe_base_enclosure_t *base_enclosure, fbe_u64_t *image_p);

fbe_status_t fbe_base_enclosure_init(fbe_base_enclosure_t * base_enclosure);

fbe_status_t fbe_base_enclosure_get_packet_payload_control_data( fbe_packet_t * packetPtr,
                                               fbe_base_enclosure_t *baseEncltPtr,
                                               BOOL use_current_operation,
                                               fbe_payload_control_buffer_t  *bufferPtr,
                                               fbe_u32_t  user_request_length);

fbe_status_t fbe_base_enclosure_set_packet_payload_status( fbe_packet_t * packetPtr,
                                                          fbe_enclosure_status_t encl_stat);


fbe_status_t fbe_base_enclosure_process_state_change(fbe_base_enclosure_t * pBaseEncl);

void fbe_base_enclosure_send_data_change_notification(fbe_base_enclosure_t *pBaseEncl,
                                                      fbe_notification_data_changed_info_t * pDataChangeInfo);

/* Executer functions */
fbe_status_t fbe_base_enclosure_discovery_transport_entry(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_base_enclosure_send_get_server_info_command(fbe_base_enclosure_t * sas_viper_enclosure, 
                                                             fbe_packet_t *packet);
                
char * fbe_base_enclosure_getFaultSymptomString(fbe_u32_t fault_symptom);
char * fbe_base_enclosure_getControlCodeString(fbe_u32_t controlCode);

static __forceinline 
fbe_u64_t fbe_base_enclosure_get_lcc_device_type(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->lcc_device_type);
}

static __forceinline 
fbe_status_t fbe_base_enclosure_set_lcc_device_type(fbe_base_enclosure_t * base_enclosure, fbe_u64_t lcc_device_type)
{
    base_enclosure->lcc_device_type = lcc_device_type;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_enclosure_type_t fbe_base_enclosure_get_enclosure_type(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->enclosure_type);
}

static __forceinline 
fbe_status_t fbe_base_enclosure_set_enclosure_type(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_type_t type)
{
    base_enclosure->enclosure_type = type;
    return FBE_STATUS_OK;
}

static __forceinline  
fbe_status_t fbe_base_enclosure_set_power_save_supported(fbe_base_enclosure_t * base_enclosure,
                                                         fbe_bool_t power_save_supported)
{
    base_enclosure->power_save_supported = power_save_supported;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_base_enclosure_get_power_save_supported(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->power_save_supported);
}

static __forceinline 
fbe_status_t fbe_base_enclosure_get_enclosure_fault_symptom(fbe_base_enclosure_t * base_enclosure)
{
     return (base_enclosure->enclosure_faults.faultSymptom);
}

static __forceinline 
fbe_u32_t fbe_base_enclosure_get_number_of_slots(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->number_of_slots);
}

static __forceinline
fbe_status_t fbe_base_enclosure_set_number_of_slots(fbe_base_enclosure_t * base_enclosure, fbe_u32_t number_of_slots)
{
    base_enclosure->number_of_slots = number_of_slots;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_status_t fbe_base_enclosure_set_number_of_slots_per_bank(fbe_base_enclosure_t * base_enclosure, fbe_u32_t number_of_slots_per_bank)
{
    base_enclosure->number_of_slots_per_bank = number_of_slots_per_bank;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_u32_t fbe_base_enclosure_get_number_of_slots_per_bank(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->number_of_slots_per_bank);
}


static __forceinline 
fbe_u32_t fbe_base_enclosure_get_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->number_of_drives_spinningup);
}

static __forceinline
fbe_status_t fbe_base_enclosure_set_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure, fbe_u32_t number_of_drives_spinningup)
{
    base_enclosure->number_of_drives_spinningup = number_of_drives_spinningup;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_status_t fbe_base_enclosure_increment_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure)
{
    base_enclosure->number_of_drives_spinningup ++;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_status_t fbe_base_enclosure_decrement_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure)
{
    base_enclosure->number_of_drives_spinningup --;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u32_t fbe_base_enclosure_get_max_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->max_number_of_drives_spinningup);
}

static __forceinline
fbe_status_t fbe_base_enclosure_set_max_number_of_drives_spinningup(fbe_base_enclosure_t * base_enclosure, fbe_u32_t max_number_of_drives_spinningup)
{
    base_enclosure->max_number_of_drives_spinningup = max_number_of_drives_spinningup;
    return FBE_STATUS_OK;
}

void * fbe_base_enclosure_allocate_buffer(void);

fbe_enclosure_component_block_t   
*fbe_base_enclosure_allocate_encl_data_block(fbe_enclosure_types_t componentType,
                                             fbe_u8_t componentId,
                                             fbe_u8_t maxNumberComponentTypes);
fbe_enclosure_component_id_t fbe_base_enclosure_get_component_id(fbe_base_enclosure_t * base_enclosure);


#endif /* FBE_BASE_ENCLOSURE_PRIVATE_H */
