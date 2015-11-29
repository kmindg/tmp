#ifndef FBE_DRIVE_CONFIGURATION_INTERFACE_H
#define FBE_DRIVE_CONFIGURATION_INTERFACE_H

#include "fbe/fbe_types.h"
/*
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_stat.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_payload_cdb_fis.h"
#include "fbe/fbe_port.h"
*/

#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_payload_cdb_fis.h"



#define FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE -1
#define MAX_THRESHOLD_TABLE_RECORDS                     50
#define MAX_EXCEPTION_CODES                             10
#define MAX_PORT_ENTRIES                                10
#define DCS_MAX_MS_TABLE_ENTRIES 64
#define DCS_MS_INVALID_ENTRY     -1
#define MAX_QUEUING_TABLE_ENTRIES                       100
#define ENHANCED_QUEUING_TIMER_MS_INVALID               0xFFFFFFFF
/* Tunable Parameters special values. */
#define FBE_DCS_PARAM_DISABLE       -1
#define FBE_DCS_PARAM_USE_DEFAULT   -2    /* change value to default */
#define FBE_DCS_PARAM_USE_CURRENT   -3    /* keep value currently set */


typedef enum fbe_drive_configuration_control_code_e {
    FBE_DRIVE_CONFIGURATION_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_DRIVE_CONFIGURATION),
    FBE_DRIVE_CONFIGURATION_CONTROL_ADD_RECORD,
    FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_THRESHOLDS,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_HANDLES_LIST,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_DRIVE_RECORD,
    FBE_DRIVE_CONFIGURATION_CONTROL_START_TABLE_UPDATE,
    FBE_DRIVE_CONFIGURATION_CONTROL_END_TABLE_UPDATE,
    FBE_DRIVE_CONFIGURATION_CONTROL_ADD_PORT_RECORD,
    FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_PORT_THRESHOLD,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_HANDLES_LIST,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_RECORD,
    FBE_DRIVE_CONFIGURATION_CONTROL_DOWNLOAD_FIRMWARE,
    FBE_DRIVE_CONFIGURATION_CONTROL_ABORT_DOWNLOAD,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_PROCESS,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_DRIVE,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_MAX_DRIVE_COUNT,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_ALL_DOWNLOAD_DRIVES,
    FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_FORCE_CLEAR_UPDATE,
    FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_GET_STATUS,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_TUNABLE_PARAMETERS,
    FBE_DRIVE_CONFIGURATION_CONTROL_SET_TUNABLE_PARAMETERS,
    FBE_DRIVE_CONFIGURATION_CONTROL_GET_MODE_PAGE_OVERRIDES,
    FBE_DRIVE_CONFIGURATION_CONTROL_SET_MODE_PAGE_BYTE,
    FBE_DRIVE_CONFIGURATION_CONTROL_MODE_PAGE_ADDL_OVERRIDE_CLEAR,
    FBE_DRIVE_CONFIGURATION_CONTROL_RESET_QUEUING_TABLE,
    FBE_DRIVE_CONFIGURATION_CONTROL_ADD_QUEUING_TABLE_ENTRY,
    FBE_DRIVE_CONFIGURATION_CONTROL_ACTIVATE_QUEUING_TABLE,

    FBE_DRIVE_CONFIGURATION_CONTROL_CODE_LAST
} fbe_drive_configuration_control_code_t;


/*!*****************************************************************************
 *  @struct fbe_dcs_control_flags_t
 *  
 *  @brief Flags to tune Physical Drive Object behavior.
 */

/*! enable health check drive action */                                       
#define  FBE_DCS_HEALTH_CHECK_ENABLED                   (0x1ULL)        /*0x0001*/
/*! enable retrying for failed remaps */
#define  FBE_DCS_REMAP_RETRIES_ENABLED                  (0x1ULL << 1)   /*0x0002*/
/*! do health check during remap handling if
   non-retryable error is returned. */
#define  FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE             (0x1ULL << 2)   /*0x0004*/
/*! fail drive when fw dl retries hit */
#define  FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES            (0x1ULL << 3)   /*0x0008*/
/*! kill drives that don't support enhance queuing */
#define  FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES        (0x1ULL << 4)   /*0x0010*/
/*! enable drive lockup error handling*/
#define  FBE_DCS_DRIVE_LOCKUP_RECOVERY                  (0x1ULL << 5)   /*0x0020*/  
/*! enable drive timeout quiesce handling*/
#define  FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING         (0x1ULL << 6)   /*0x0040*/
/*! enable PFA handling. */
#define FBE_DCS_PFA_HANDLING                            (0x1ULL << 7)   /*0x0080*/
/*! enable Mode Select of override table during 
    drive init */
#define FBE_DCS_MODE_SELECT_CAPABILITY                  (0x1ULL << 8)   /*0x0100*/ 
/*! break up fw image for download */
#define FBE_DCS_BREAK_UP_FW_IMAGE                       (0x1ULL << 9)   /*0x0200*/
/*! Allow 4K (4160) formated drives */
#define FBE_DCS_4K_ENABLED                              (0x1ULL << 10)  /*0x0400*/
/*! Don't kill drives that have invalid identity */
#define FBE_DCS_IGNORE_INVALID_IDENTITY                 (0x1ULL << 11)  /*0x0800*/

typedef fbe_u64_t fbe_dcs_control_flags_t;   
/*****************************************************************************/

/*!********************************************************************* 
 * @struct fbe_dcs_tunable_params_s 
 *  
 * @brief 
 *   Contains drive configuration tunable parameters for all PDOs
 *
 **********************************************************************/
typedef struct fbe_dcs_tunable_params_s{
    fbe_dcs_control_flags_t         control_flags;
    fbe_time_t                      service_time_limit_ms; /* Expected service time limit in milliseconds */
    fbe_time_t                      emeh_service_time_limit_ms;
    fbe_time_t                      remap_service_time_limit_ms;
    fbe_u32_t                       fw_image_chunk_size; /*byte size for each sg_element*/
}fbe_dcs_tunable_params_t;


/*!********************************************************************* 
 * @struct fbe_dcs_mode_page_byte_s 
 *  
 * @brief 
 *   Contains drive configuration mode page offset value
 *
 **********************************************************************/
typedef struct fbe_dcs_mode_page_byte_s{
    fbe_u8_t page;
    fbe_u8_t byte_offset;
    fbe_u8_t mask;    
    fbe_u8_t value;
}fbe_dcs_mode_page_byte_t;


typedef enum fbe_drive_configuration_mode_page_override_table_id_e
{
    FBE_DCS_MP_OVERRIDE_TABLE_ID_DEFAULT = 0,
    FBE_DCS_MP_OVERRIDE_TABLE_ID_SAMSUNG,
    FBE_DCS_MP_OVERRIDE_TABLE_ID_MINIMAL,
    FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL,
} fbe_drive_configuration_mode_page_override_table_id_t;

typedef struct fbe_dcs_set_mode_page_override_table_entry_s{
    fbe_drive_configuration_mode_page_override_table_id_t table_id;
    fbe_dcs_mode_page_byte_t table_entry;
} fbe_dcs_set_mode_page_override_table_entry_t;

/*!********************************************************************* 
 * @struct fbe_dcs_mode_select_override_info_s 
 *  
 * @brief 
 *   Contains drive configuration structure to hold mode select
 *   override table.
 *
 **********************************************************************/
typedef struct fbe_dcs_mode_select_override_info_s{
    fbe_drive_configuration_mode_page_override_table_id_t table_id;
    fbe_u32_t                 num_entries;
    fbe_dcs_mode_page_byte_t  table[DCS_MAX_MS_TABLE_ENTRIES];
}fbe_dcs_mode_select_override_info_t;


/*!********************************************************************* 
 * @enum fbe_dcs_dieh_status_qualifier_t 
 *  
 * @brief 
 * The Drive Configuration Service DIEH status
 *         
 **********************************************************************/
typedef enum fbe_dcs_dieh_status_e{
    FBE_DCS_DIEH_STATUS_OK,
    FBE_DCS_DIEH_STATUS_FAILED,
    FBE_DCS_DIEH_STATUS_FAILED_UPDATE_IN_PROGRESS,
    
    FBE_DCS_DIEH_STATUS_LAST
}fbe_dcs_dieh_status_t;


typedef struct fbe_drive_configuration_drive_info_s{
    fbe_drive_type_t        drive_type;
    fbe_drive_vendor_id_t   drive_vendor;
    fbe_u8_t                part_number[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1];
    fbe_u8_t                fw_revision[FBE_SCSI_INQUIRY_REVISION_SIZE + 1];
    fbe_u8_t                serial_number_start[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
    fbe_u8_t                serial_number_end[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
}fbe_drive_configuration_drive_info_t;

typedef struct fbe_drive_config_scsi_execption_code_s{
    fbe_u8_t                            sense_key;
    fbe_u8_t                            asc_range_start;
    fbe_u8_t                            asc_range_end;
    fbe_u8_t                            ascq_range_start;
    fbe_u8_t                            ascq_range_end;
}fbe_drive_config_scsi_sense_code_t;

typedef struct fbe_drive_config_fis_code_s{
 fbe_u32_t       fis_status;
}fbe_drive_config_fis_code_t;

typedef struct fbe_drive_config_scsi_fis_exception_code_s{
 union{
  fbe_drive_config_scsi_sense_code_t    scsi_code;
  fbe_drive_config_fis_code_t           fis_code;
 }scsi_fis_union;
 
 fbe_payload_cdb_fis_io_status_t    status;
 fbe_payload_cdb_fis_error_flags_t  type_and_action;
}fbe_drive_config_scsi_fis_exception_code_t;

typedef struct fbe_drive_threshold_and_exceptions_s
{
    fbe_drive_stat_t                            threshold_info;
    fbe_drive_config_scsi_fis_exception_code_t  category_exception_codes[MAX_EXCEPTION_CODES]; 
} fbe_drive_threshold_and_exceptions_t;

typedef struct fbe_drive_configuration_record_s{
    fbe_drive_configuration_drive_info_t        drive_info;
    fbe_drive_stat_t                            threshold_info;
    fbe_drive_config_scsi_fis_exception_code_t  category_exception_codes[MAX_EXCEPTION_CODES]; 
}fbe_drive_configuration_record_t;

typedef struct fbe_drive_configuration_port_info_s{
    fbe_port_type_t     port_type;
}fbe_drive_configuration_port_info_t;

typedef struct fbe_drive_configuration_port_record_s{
    fbe_drive_configuration_port_info_t     port_info;
    fbe_port_stat_t                         threshold_info;
}fbe_drive_configuration_port_record_t;

typedef struct fbe_drive_configuration_registration_info_s{
    fbe_drive_type_t        drive_type;
    fbe_drive_vendor_id_t   drive_vendor;
    fbe_u8_t                part_number[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1];
    fbe_u8_t                fw_revision[FBE_SCSI_INQUIRY_REVISION_SIZE + 1];
    fbe_u8_t                serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
}fbe_drive_configuration_registration_info_t;


typedef struct fbe_vendor_page_s {
    fbe_u8_t page;
    fbe_u8_t offset;
    fbe_u8_t mask;
    fbe_u8_t value;
} fbe_vendor_page_t;


/*USURPER strucuters*/
/*FBE_DRIVE_CONFIGURATION_CONTROL_ADD_RECORD*/
typedef struct fbe_drive_configuration_control_add_record_s{
    fbe_drive_configuration_record_t        new_record;
    fbe_drive_configuration_handle_t        handle;
}fbe_drive_configuration_control_add_record_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_THRESHOLDS*/
typedef struct fbe_drive_configuration_control_change_thresholds_s{
    fbe_drive_stat_t                            new_threshold;
    fbe_drive_configuration_handle_t            handle;
}fbe_drive_configuration_control_change_thresholds_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_HANDLES_LIST*/
typedef struct fbe_drive_configuration_control_get_handles_list_s{
    fbe_drive_configuration_handle_t    handles_list[MAX_THRESHOLD_TABLE_RECORDS];
    fbe_u32_t                           total_count;
}fbe_drive_configuration_control_get_handles_list_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_DRIVE_RECORD*/
typedef struct fbe_drive_configuration_control_get_drive_record_s{
    fbe_drive_configuration_handle_t    handle;
    fbe_drive_configuration_record_t    record;
}fbe_drive_configuration_control_get_drive_record_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_ADD_PORT_RECORD*/
typedef struct fbe_drive_configuration_control_add_port_record_s{
    fbe_drive_configuration_port_record_t   new_record;
    fbe_drive_configuration_handle_t        handle;
}fbe_drive_configuration_control_add_port_record_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_PORT_THRESHOLD*/
typedef struct fbe_drive_configuration_control_change_port_threshold_s{
    fbe_port_stat_t                         new_threshold;
    fbe_drive_configuration_handle_t        handle;
}fbe_drive_configuration_control_change_port_threshold_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_RECORD*/
typedef struct fbe_drive_configuration_control_get_port_record_s{
    fbe_drive_configuration_handle_t        handle;
    fbe_drive_configuration_port_record_t   record;
}fbe_drive_configuration_control_get_port_record_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_ALL_DOWNLOAD_DRIVES*/
typedef struct fbe_drive_configuration_control_get_all_download_drive_s{
    fbe_u32_t   actual_returned;
}fbe_drive_configuration_control_get_all_download_drive_t;

typedef struct fbe_drive_configuration_queuing_record_s{
    fbe_drive_configuration_drive_info_t    drive_info;
    fbe_u32_t                               lpq_timer;
    fbe_u32_t                               hpq_timer;
}fbe_drive_configuration_queuing_record_t;

typedef struct fbe_drive_configuration_control_add_queuing_entry_s{
    fbe_drive_configuration_queuing_record_t    queue_entry;
}fbe_drive_configuration_control_add_queuing_entry_t;



/*APIS*/
fbe_status_t fbe_drive_configuration_register_drive (fbe_drive_configuration_registration_info_t *registration_info,
                                                     fbe_drive_configuration_handle_t *handle);

fbe_status_t fbe_drive_configuration_get_threshold_info (fbe_drive_configuration_handle_t handle, fbe_drive_threshold_and_exceptions_t  *threshold_rec);
fbe_status_t fbe_drive_configuration_get_threshold_info_ptr (fbe_drive_configuration_handle_t handle, fbe_drive_configuration_record_t **threshold_rec_pp);

fbe_status_t fbe_drive_configuration_get_scsi_exception_action (fbe_drive_configuration_record_t *threshold_rec,
                                                                fbe_payload_cdb_operation_t *cdb_operation, 
                                                                fbe_payload_cdb_fis_io_status_t *status,
                                                                fbe_payload_cdb_fis_error_t *type_and_action);

fbe_status_t fbe_drive_configuration_get_fis_exception_action (fbe_drive_configuration_handle_t handle,
                                                                fbe_payload_fis_operation_t *fis_operation, 
                                                                fbe_payload_cdb_fis_io_status_t *io_status,
                                                                fbe_payload_cdb_fis_error_flags_t *type_and_action);


fbe_status_t fbe_drive_configuration_unregister_drive (fbe_drive_configuration_handle_t handle);

fbe_status_t fbe_drive_configuration_did_drive_handle_change(fbe_drive_configuration_registration_info_t *registration_info,
                                                             fbe_drive_configuration_handle_t handle,
                                                             fbe_bool_t *changed);

fbe_status_t fbe_drive_configuration_register_port (fbe_drive_configuration_port_info_t *registration_info,
                                                    fbe_drive_configuration_handle_t *handle);

fbe_status_t fbe_drive_configuration_unregister_port (fbe_drive_configuration_handle_t handle);

fbe_status_t fbe_drive_configuration_get_port_threshold_info (fbe_drive_configuration_handle_t handle, fbe_port_stat_t  *threshold_info);

fbe_status_t fbe_drive_configuration_get_queuing_info(fbe_drive_configuration_registration_info_t *registration_info,
                                                                fbe_u32_t *lpq_timer, fbe_u32_t *hpq_timer);



/*Parameter Interface*/

fbe_bool_t fbe_dcs_param_is_enabled(fbe_dcs_control_flags_t flag);
fbe_bool_t fbe_dcs_is_healthcheck_enabled(void);
fbe_bool_t fbe_dcs_is_remap_retries_enabled(void);
fbe_bool_t fbe_dcs_use_hc_for_remap_non_retryable(void);
fbe_time_t fbe_dcs_get_svc_time_limit(void);
fbe_time_t fbe_dcs_get_remap_svc_time_limit(void);
fbe_time_t fbe_dcs_get_emeh_svc_time_limit(void);
fbe_u32_t  fbe_dcs_get_fw_image_chunk_size(void);



#endif /* FBE_DRIVE_CONFIGURATION_INTERFACE_H */

