#ifndef FBE_TERMINATOR_SERVICE_H
#define FBE_TERMINATOR_SERVICE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_terminator_drive_interface.h"
#include "../../fbe/src/services/cluster_memory/interface/memory/fbe_cms_memory.h"

typedef enum fbe_terminator_control_code_e
{
    FBE_TERMINATOR_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_TERMINATOR),

    FBE_TERMINATOR_CONTROL_CODE_INIT,
    FBE_TERMINATOR_CONTROL_CODE_DESTROY,

    FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGIN_DEVICE,
    FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGOUT_DEVICE,

    FBE_TERMINATOR_CONTROL_CODE_FIND_DEVICE_CLASS,

    FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_ATTRIBUTE,
    FBE_TERMINATOR_CONTROL_CODE_GET_PORT_HANDLE,
    FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_HANDLE,
    FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_HANDLE,
    FBE_TERMINATOR_CONTROL_CODE_GET_TEMP_SENSOR_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_COOLING_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_PS_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_SAS_CONN_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_LCC_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_SAS_DRIVE_INFO,    
    FBE_TERMINATOR_CONTROL_CODE_GET_SATA_DRIVE_INFO,
    FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_PRODUCT_ID,
    FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SAS_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SATA_DRIVE,

    FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_ERROR_COUNT,
    FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_CPD_DEVICE_ID,
    FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM,
    FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV,
    FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL,
    FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL,
    FBE_TERMINATOR_CONTROL_CODE_GET_SIMULATED_DRIVE_TYPE,

    FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_TYPE,
    FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_NAME,
    FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_PORT,
    FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_DEBUG_FLAGS,
    FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_COMPRESSION_DISABLE,
    FBE_TERMINATOR_CONTROL_CODE_SET_DEVICE_ATTRIBUTE,
    FBE_TERMINATOR_CONTROL_CODE_SET_SAS_ENCLOSURE_DRIVE_SLOT_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_TEMP_SENSOR_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_COOLING_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_PS_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_SAS_CONN_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_LCC_ESES_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_SET_BUF,
    FBE_TERMINATOR_CONTROL_CODE_SET_BUF_BY_BUF_ID,
    FBE_TERMINATOR_CONTROL_CODE_SET_RESUME_PROM_INFO,
    FBE_TERMINATOR_CONTROL_CODE_SET_IO_MODE,
    FBE_TERMINATOR_CONTROL_CODE_SET_IO_COMPLETION_IRQL,
    FBE_TERMINATOR_CONTROL_CODE_SET_IO_GLOBAL_COMPLETION_DELAY,
    FBE_TERMINATOR_CONTROL_CODE_SET_DRIVE_PRODUCT_ID,
    FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM,
    FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV,
    FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL,
    FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL,

    FBE_TERMINATOR_CONTROL_CODE_CREATE_DEVICE_CLASS_INSTANCE,

    FBE_TERMINATOR_CONTROL_CODE_INSERT_DEVICE,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_BOARD,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_PORT,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_ENCLOSURE,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_SATA_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_REINSERT_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_REINSERT_ENCLOSURE,

    FBE_TERMINATOR_CONTROL_CODE_INSERT_FC_PORT,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_ISCSI_PORT,
    FBE_TERMINATOR_CONTROL_CODE_INSERT_FCOE_PORT,


    FBE_TERMINATOR_CONTROL_CODE_REMOVE_DEVICE,
    FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_ENCLOSURE,
    FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_REMOVE_SATA_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_PULL_DRIVE,
    FBE_TERMINATOR_CONTROL_CODE_PULL_ENCLOSURE,

    FBE_TERMINATOR_CONTROL_CODE_ACTIVATE_DEVICE,

    FBE_TERMINATOR_CONTROL_CODE_START_PORT_RESET,
    FBE_TERMINATOR_CONTROL_CODE_COMPLETE_PORT_RESET,

    FBE_TERMINATOR_CONTROL_CODE_GET_HARDWARE_INFO,
    FBE_TERMINATOR_CONTROL_CODE_SET_HARDWARE_INFO,
	FBE_TERMINATOR_CONTROL_CODE_GET_SFP_MEDIA_INTERFACE_INFO,
    FBE_TERMINATOR_CONTROL_CODE_SET_SFP_MEDIA_INTERFACE_INFO,


    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_BYPASS_DRIVE_SLOT,
    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_UNBYPASS_DRIVE_SLOT,
    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_ENCL_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_ENCL_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_PS_INFO_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_PS_INFO_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_ESES_INCREMENT_CONFIG_PAGE_GEN_CODE,
    FBE_TERMINATOR_CONTROL_CODE_ESES_GET_VER_DESC,
    FBE_TERMINATOR_CONTROL_CODE_ESES_SET_DOWNLOAD_MICROCODE_STAT_PAGE_STAT_DESC,
    FBE_TERMINATOR_CONTROL_CODE_ESES_SET_VER_DESC,
    FBE_TERMINATOR_CONTROL_CODE_ESES_SET_UNIT_ATTENTION,
    FBE_TERMINATOR_CONTROL_CODE_MARK_ESES_PAGE_UNSUPPORTED,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT_ERROR,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_ADD_ERROR,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_START,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_STOP,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_DESTROY,

    FBE_TERMINATOR_CONTROL_CODE_RESERVE_MINIPORT_SAS_DEVICE_TABLE_INDEX,
    FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_ADD,
    FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_REMOVE,
	FBE_TERMINATOR_CONTROL_CODE_SET_SP_ID,
	FBE_TERMINATOR_CONTROL_CODE_GET_SP_ID,
    FBE_TERMINATOR_CONTROL_CODE_SET_SINGLE_SP,
	FBE_TERMINATOR_CONTROL_CODE_IS_SINGLE_SP_SYSTEM,
	FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_STATE,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_GET_DEFAULT_PAGE_INFO,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_PAGE_INFO,
    FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_FIELD,
	FBE_TERMINATOR_CONTROL_CODE_GET_DEVICES_COUNT_BY_TYPE_NAME,
	FBE_TERMINATOR_CONTROL_CODE_SEND_SPECL_SFI_MASK_DATA,

	FBE_TERMINATOR_CONTROL_CODE_GET_PORT_LINK_INFO,
    FBE_TERMINATOR_CONTROL_CODE_SET_PORT_LINK_INFO,

    FBE_TERMINATOR_CONTROL_CODE_GET_CONNECTOR_ID_LIST_FOR_ENCLOSURE,
    FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_SLOT_PARENT,
    FBE_TERMINATOR_CONTROL_CODE_GET_TERMINATOR_DEVICE_COUNT,

    FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_REQUEST,
    FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_REQUEST,
    FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_STATUS,
    FBE_TERMINATOR_CONTROL_CODE_ZERO_PERSISTENT_MEMORY,

    FBE_TERMINATOR_CONTROL_CODE_GET_ENCRYPTION_KEY,
    FBE_TERMINATOR_CONTROL_CODE_PORT_ADDRESS,
    FBE_TERMINATOR_CONTROL_CODE_SET_LOG_PAGE_31,
    FBE_TERMINATOR_CONTROL_CODE_GET_LOG_PAGE_31,
    FBE_TERMINATOR_CONTROL_CODE_LAST
} fbe_terminator_control_code_t;

#define PACKAGE_COMMON_STRING_LEN 32
#define PACKAGE_COMMON_BUFFER_LEN 4096

typedef struct fbe_terminator_force_login_device_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_force_login_device_ioctl_t;

typedef struct fbe_terminator_force_logout_device_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_force_logout_device_ioctl_t;

typedef struct fbe_terminator_find_device_class_ioctl_s
{
    char device_class_name[PACKAGE_COMMON_STRING_LEN];
    fbe_terminator_api_device_class_handle_t device_class_handle;
}fbe_terminator_find_device_class_ioctl_t;

typedef struct fbe_terminator_insert_device_ioctl_s
{
    fbe_terminator_api_device_handle_t parent_device;
    fbe_terminator_api_device_handle_t child_device;
}fbe_terminator_insert_device_ioctl_t;

typedef struct fbe_terminator_insert_board_ioctl_s
{
    fbe_terminator_board_info_t board_info;
}fbe_terminator_insert_board_ioctl_t;

typedef struct fbe_terminator_insert_sas_port_ioctl_s
{
    fbe_terminator_sas_port_info_t port_info;
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_insert_sas_port_ioctl_t;

typedef struct fbe_terminator_insert_fc_port_ioctl_s
{
    fbe_terminator_fc_port_info_t port_info;
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_insert_fc_port_ioctl_t;

typedef struct fbe_terminator_insert_iscsi_port_ioctl_s
{
    fbe_terminator_iscsi_port_info_t port_info;
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_insert_iscsi_port_ioctl_t;

typedef struct fbe_terminator_insert_fcoe_port_ioctl_s
{
    fbe_terminator_fcoe_port_info_t port_info;
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_insert_fcoe_port_ioctl_t;

typedef struct fbe_terminator_get_device_attribute_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    char attribute_name[PACKAGE_COMMON_STRING_LEN];
    char attribute_value_buffer[PACKAGE_COMMON_STRING_LEN];
    fbe_u32_t buffer_length;
}fbe_terminator_get_device_attribute_ioctl_t;

typedef struct fbe_terminator_get_port_handle_ioctl_s
{
    fbe_u32_t backend_number;
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_get_port_handle_ioctl_t;

typedef struct fbe_terminator_get_enclosure_handle_ioctl_s
{
    fbe_u32_t port_number;
    fbe_u32_t enclosure_number;
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_get_enclosure_handle_ioctl_t;

typedef struct fbe_terminator_get_temp_sensor_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_temp_sensor_id temp_sensor_id;
    ses_stat_elem_temp_sensor_struct temp_sensor_stat;
}fbe_terminator_get_temp_sensor_eses_status_ioctl_t;

typedef struct fbe_terminator_get_cooling_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_cooling_id cooling_id;
    ses_stat_elem_cooling_struct cooling_stat;
}fbe_terminator_get_cooling_eses_status_ioctl_t;

typedef struct fbe_terminator_get_ps_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_ps_id ps_id;
    ses_stat_elem_ps_struct ps_stat;
}fbe_terminator_get_ps_eses_status_ioctl_t;

typedef struct fbe_terminator_get_sas_conn_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_sas_conn_id sas_conn_id;
    ses_stat_elem_sas_conn_struct sas_conn_stat;
}fbe_terminator_get_sas_conn_eses_status_ioctl_t;

typedef struct fbe_terminator_get_lcc_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    ses_stat_elem_encl_struct lcc_stat;
}fbe_terminator_get_lcc_eses_status_ioctl_t;

typedef struct fbe_terminator_get_drive_handle_ioctl_s
{
    fbe_u32_t port_number;
    fbe_u32_t enclosure_number;
    fbe_u32_t slot_number;
    fbe_terminator_api_device_handle_t drive_handle;
}fbe_terminator_get_drive_handle_ioctl_t;

typedef struct fbe_terminator_get_sas_drive_info_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
    fbe_terminator_sas_drive_info_t drive_info;
}fbe_terminator_get_sas_drive_info_ioctl_t;

typedef struct fbe_terminator_get_sata_drive_info_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
    fbe_terminator_sata_drive_info_t drive_info;
}fbe_terminator_get_sata_drive_info_ioctl_t;

typedef struct fbe_terminator_get_drive_product_id_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
    fbe_u8_t product_id[PACKAGE_COMMON_STRING_LEN];
}fbe_terminator_get_drive_product_id_ioctl_t;

typedef struct fbe_terminator_get_drive_error_count_ioctl_s
{
    fbe_terminator_api_device_handle_t handle; 
    fbe_u32_t  error_count_p;
}fbe_terminator_get_drive_error_count_ioctl_t;

typedef struct fbe_terminator_get_device_cpd_device_id_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    fbe_miniport_device_id_t cpd_device_id;
}fbe_terminator_get_device_cpd_device_id_ioctl_t;

typedef struct fbe_terminator_set_simulated_drive_type_ioctl_s
{
    terminator_simulated_drive_type_t simulated_drive_type;
}fbe_terminator_set_simulated_drive_type_ioctl_t;

typedef struct fbe_terminator_get_simulated_drive_type_ioctl_s
{
    terminator_simulated_drive_type_t simulated_drive_type;
}fbe_terminator_get_simulated_drive_type_ioctl_t;

typedef struct fbe_terminator_set_simulated_drive_server_name_ioctl_s
{
    char server_name[PACKAGE_COMMON_STRING_LEN];
}fbe_terminator_set_simulated_drive_server_name_ioctl_t;

typedef struct fbe_terminator_set_simulated_drive_server_port_ioctl_s
{
    fbe_u16_t server_port;
}fbe_terminator_set_simulated_drive_server_port_ioctl_t;

typedef struct fbe_terminator_set_simulated_drive_debug_flags_ioctl_s
{
    fbe_terminator_drive_select_type_t  drive_select_type;
    fbe_u32_t                           first_term_drive_index;
    fbe_u32_t                           last_term_drive_index;
    fbe_u32_t                           backend_bus_number;
    fbe_u32_t                           encl_number;
    fbe_u32_t                           slot_number;
    fbe_terminator_drive_debug_flags_t  terminator_drive_debug_flags;
}fbe_terminator_set_simulated_drive_debug_flags_ioctl_t;

typedef struct fbe_terminator_log_page_31_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    fbe_u8_t log_page_31[PACKAGE_COMMON_BUFFER_LEN];
    fbe_u32_t buffer_length;
}fbe_terminator_log_page_31_ioctl_t;

typedef struct fbe_terminator_set_device_attribute_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    char attribute_name[PACKAGE_COMMON_STRING_LEN];
    char attribute_value[PACKAGE_COMMON_STRING_LEN];
}fbe_terminator_set_device_attribute_ioctl_t;

typedef struct fbe_terminator_set_drive_product_id_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
    fbe_u8_t product_id[PACKAGE_COMMON_STRING_LEN];
}fbe_terminator_set_drive_product_id_ioctl_t;

typedef struct fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t slot_number;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
}fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_t;

typedef struct fbe_terminator_set_temp_sensor_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_temp_sensor_id temp_sensor_id;
    ses_stat_elem_temp_sensor_struct temp_sensor_stat;
}fbe_terminator_set_temp_sensor_eses_status_ioctl_t;

typedef struct fbe_terminator_set_cooling_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_cooling_id cooling_id;
    ses_stat_elem_cooling_struct cooling_stat;
}fbe_terminator_set_cooling_eses_status_ioctl_t;

typedef struct fbe_terminator_set_ps_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_ps_id ps_id;
    ses_stat_elem_ps_struct ps_stat;
}fbe_terminator_set_ps_eses_status_ioctl_t;

typedef struct fbe_terminator_set_sas_conn_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_sas_conn_id sas_conn_id;
    ses_stat_elem_sas_conn_struct sas_conn_stat;
}fbe_terminator_set_sas_conn_eses_status_ioctl_t;

typedef struct fbe_terminator_set_lcc_eses_status_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    ses_stat_elem_encl_struct lcc_stat;
}fbe_terminator_set_lcc_eses_status_ioctl_t;

typedef struct fbe_terminator_set_buf_by_buf_id_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u8_t buf_id;
    fbe_u8_t buf[PACKAGE_COMMON_BUFFER_LEN];
    fbe_u32_t len;
}fbe_terminator_set_buf_by_buf_id_ioctl_t;

typedef struct fbe_terminator_set_buf_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    ses_subencl_type_enum subencl_type;
    fbe_u8_t side;
    ses_buf_type_enum buf_type;
    fbe_u8_t buf[PACKAGE_COMMON_BUFFER_LEN];
    fbe_u32_t len;
}fbe_terminator_set_buf_ioctl_t;

typedef struct fbe_terminator_set_resume_prom_info_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    terminator_eses_resume_prom_id_t resume_prom_id;
    fbe_u8_t buf[PACKAGE_COMMON_BUFFER_LEN];
    fbe_u32_t len;
}fbe_terminator_set_resume_prom_info_ioctl_t;

typedef struct fbe_terminator_set_io_mode_ioctl_s
{
    fbe_terminator_io_mode_t io_mode;
}fbe_terminator_set_io_mode_ioctl_t;

typedef struct fbe_terminator_set_io_completion_irql_ioctl_s
{
    fbe_bool_t  b_should_completion_be_at_dpc;
}fbe_terminator_set_io_completion_irql_ioctl_t;

typedef struct fbe_terminator_set_io_global_completion_delay_ioctl_s
{
    fbe_u32_t global_completion_delay;
}fbe_terminator_set_io_global_completion_delay_ioctl_t;


typedef struct fbe_terminator_create_device_class_instance_ioctl_s
{
    fbe_terminator_api_device_class_handle_t device_class_handle;
    char device_type[PACKAGE_COMMON_STRING_LEN];
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_create_device_class_instance_ioctl_t;

typedef struct fbe_terminator_insert_sas_encl_ioctl_s
{
    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_sas_encl_info_t      encl_info;
    fbe_terminator_api_device_handle_t  enclosure_handle;
}fbe_terminator_insert_sas_encl_ioctl_t;

typedef struct fbe_terminator_insert_sas_drive_ioctl_s
{
    fbe_terminator_api_device_handle_t  enclosure_handle;
    fbe_u32_t slot_number;
    fbe_terminator_sas_drive_info_t     drive_info;
    fbe_terminator_api_device_handle_t  drive_handle;
}fbe_terminator_insert_sas_drive_ioctl_t;

typedef struct fbe_terminator_insert_sata_drive_ioctl_s
{
    fbe_terminator_api_device_handle_t  enclosure_handle;
    fbe_u32_t slot_number;
    fbe_terminator_sata_drive_info_t     drive_info;
    fbe_terminator_api_device_handle_t  drive_handle;
}fbe_terminator_insert_sata_drive_ioctl_t;

typedef struct fbe_terminator_reinsert_drive_ioctl_s
{
	fbe_terminator_api_device_handle_t enclosure_handle;
	fbe_u32_t slot_number;
    fbe_terminator_api_device_handle_t drive_handle;
}fbe_terminator_reinsert_drive_ioctl_t;

typedef struct fbe_terminator_reinsert_enclosure_ioctl_s
{
    fbe_terminator_api_device_handle_t port_handle;
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_reinsert_enclosure_ioctl_t;

typedef struct fbe_terminator_force_create_sas_drive_ioctl_s
{
    fbe_terminator_sas_drive_info_t     drive_info;
    fbe_terminator_api_device_handle_t  drive_handle;
}fbe_terminator_force_create_sas_drive_ioctl_t;

typedef struct fbe_terminator_force_create_sata_drive_ioctl_s
{
    fbe_terminator_sata_drive_info_t     drive_info;
    fbe_terminator_api_device_handle_t   drive_handle;
}fbe_terminator_force_create_sata_drive_ioctl_t;

typedef struct fbe_terminator_remove_device_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_remove_device_ioctl_t;

typedef struct fbe_terminator_remove_sas_enclosure_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_remove_sas_enclosure_ioctl_t;

typedef struct fbe_terminator_remove_sas_drive_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
}fbe_terminator_remove_sas_drive_ioctl_t;

typedef struct fbe_terminator_remove_sata_drive_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
}fbe_terminator_remove_sata_drive_ioctl_t;

typedef struct fbe_terminator_pull_drive_ioctl_s
{
    fbe_terminator_api_device_handle_t drive_handle;
}fbe_terminator_pull_drive_ioctl_t;

typedef struct fbe_terminator_pull_enclosure_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_pull_enclosure_ioctl_t;

typedef struct fbe_terminator_activate_device_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_activate_device_ioctl_t;

typedef struct fbe_terminator_start_port_reset_ioctl_s
{
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_start_port_reset_ioctl_t;

typedef struct fbe_terminator_complete_port_reset_ioctl_s
{
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_complete_port_reset_ioctl_t;

typedef struct fbe_terminator_get_hardware_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_hardware_info_t		hdw_info;
}fbe_terminator_get_hardware_info_ioctl_t;

typedef struct fbe_terminator_set_hardware_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_hardware_info_t		hdw_info;
}fbe_terminator_set_hardware_info_ioctl_t;

typedef struct fbe_terminator_get_sfp_media_interface_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_sfp_media_interface_info_t		sfp_info;
}fbe_terminator_get_sfp_media_interface_info_ioctl_t;

typedef struct fbe_terminator_set_sfp_media_interface_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_sfp_media_interface_info_t		sfp_info;
}fbe_terminator_set_sfp_media_interface_info_ioctl_t;

typedef struct fbe_terminator_get_port_link_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_port_lane_info_t		port_link_info;
}fbe_terminator_get_port_link_info_ioctl_t;

typedef struct fbe_terminator_set_port_link_info_ioctl_s
{
    fbe_terminator_api_device_handle_t	port_handle;
	fbe_cpd_shim_port_lane_info_t		port_link_info;
}fbe_terminator_set_port_link_info_ioctl_t;

/* enclosure */
typedef struct fbe_terminator_enclosure_bypass_drive_slot_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t slot_number;
}fbe_terminator_enclosure_bypass_drive_slot_ioctl_t;

typedef struct fbe_terminator_enclosure_unbypass_drive_slot_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t slot_number;
}fbe_terminator_enclosure_unbypass_drive_slot_ioctl_t;

typedef struct fbe_terminator_enclosure_emc_encl_status_ioctl_s
{
    fbe_terminator_api_device_handle_t  enclosure_handle;
    ses_pg_emc_encl_stat_struct         emcEnclStatus;
}fbe_terminator_enclosure_emc_encl_status_ioctl_t;

typedef struct fbe_terminator_enclosure_emc_ps_info_status_ioctl_s
{
    fbe_terminator_api_device_handle_t  enclosure_handle;
    terminator_eses_ps_id               psIndex;
    ses_ps_info_elem_struct             emcPsInfoStatus;
}fbe_terminator_enclosure_emc_ps_info_status_ioctl_t;

typedef struct fbe_terminator_eses_increment_config_page_gen_code_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_eses_increment_config_page_gen_code_ioctl_t;

typedef struct fbe_terminator_eses_set_unit_attention_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
}fbe_terminator_eses_set_unit_attention_ioctl_t;

typedef struct fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_download_status_desc_t download_stat_desc;
}fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_t;

typedef struct fbe_terminator_eses_get_ver_desc_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    ses_subencl_type_enum subencl_type;
    fbe_u8_t side;
    fbe_u8_t  comp_type;
    ses_ver_desc_struct ver_desc;
}fbe_terminator_eses_get_ver_desc_ioctl_t;

typedef struct fbe_terminator_eses_set_ver_desc_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    ses_subencl_type_enum subencl_type;
    fbe_u8_t  side;
    fbe_u8_t  comp_type;
    ses_ver_desc_struct ver_desc;
}fbe_terminator_eses_set_ver_desc_ioctl_t;

typedef struct fbe_terminator_mark_eses_page_unsupported_ioctl_s
{
    fbe_u8_t cdb_opcode;
    fbe_u8_t diag_page_code;
}fbe_terminator_mark_eses_page_unsupported_ioctl_t;

typedef struct fbe_terminator_drive_error_injection_init_error_ioctl_s
{
    fbe_terminator_neit_error_record_t record;
}fbe_terminator_drive_error_injection_init_error_ioctl_t;

typedef struct fbe_terminator_drive_error_injection_add_error_ioctl_s
{
    fbe_terminator_neit_error_record_t record;
}fbe_terminator_drive_error_injection_add_error_ioctl_t;

typedef struct fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_s
{
    fbe_u32_t port_number;
    fbe_miniport_device_id_t cpd_device_id;
}fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_t;

typedef struct fbe_terminator_miniport_sas_device_table_force_add_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    fbe_miniport_device_id_t cpd_device_id;
}fbe_terminator_miniport_sas_device_table_force_add_ioctl_t;

typedef struct fbe_terminator_miniport_sas_device_table_force_remove_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
}fbe_terminator_miniport_sas_device_table_force_remove_ioctl_t;

typedef struct fbe_terminator_get_devices_count_by_type_name_ioctl_s
{
    fbe_u8_t device_type_name[PACKAGE_COMMON_STRING_LEN];
    fbe_u32_t  device_count;
}fbe_terminator_get_devices_count_by_type_name_ioctl_t;
/*FBE_TERMINATOR_CONTROL_CODE_SET_SP_ID*/
/*FBE_TERMINATOR_CONTROL_CODE_GET_SP_ID*/
typedef struct fbe_terminator_miniport_sp_id_ioctl_s
{
    terminator_sp_id_t sp_id;
}fbe_terminator_miniport_sp_id_ioctl_t;

typedef struct fbe_terminator_sp_single_or_dual_ioctl_s
{
    fbe_bool_t is_single;
}fbe_terminator_sp_single_or_dual_ioctl_t;

typedef struct fbe_terminator_send_specl_sfi_mask_data_ioctl_s
{
    SPECL_SFI_MASK_DATA specl_sfi_mask_data;
}fbe_terminator_send_specl_sfi_mask_data_ioctl_t;

typedef struct fbe_terminator_drive_set_state_ioctl_s
{
    fbe_terminator_api_device_handle_t device_handle;
    terminator_sas_drive_state_t drive_state;
}fbe_terminator_drive_set_state_ioctl_t;

typedef struct fbe_terminator_drive_type_default_page_info_ioctl_s
{
    fbe_sas_drive_type_t drive_type;
    fbe_terminator_sas_drive_type_default_info_t default_info;
}fbe_terminator_drive_type_default_page_info_ioctl_t;

typedef struct fbe_terminator_drive_default_ioctl_s
{
    fbe_sas_drive_type_t drive_type;
    fbe_terminator_drive_default_field_t field;
}fbe_terminator_drive_default_ioctl_t;

typedef struct fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_s
{
    fbe_bool_t need_update_checksum;
}fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_t;

typedef struct fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_s
{
    fbe_bool_t need_update_checksum;
}fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_t;

typedef struct fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_s
{
    fbe_bool_t need_update_rev;
}fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_t;

typedef struct fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_s
{
    fbe_bool_t need_update_rev;
}fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_t;

typedef struct fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t time_interval;
}fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_t;

typedef struct fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t time_interval;
}fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_t;

typedef struct fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t time_interval;
}fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_t;

typedef struct fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t time_interval;
}fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_t;

typedef struct fbe_terminator_get_connector_id_list_for_enclosure_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_term_encl_connector_list_t connector_ids;
}fbe_terminator_get_connector_id_list_for_enclosure_ioctl_t;

typedef struct fbe_terminator_get_drive_slot_parent_ioctl_s
{
    fbe_terminator_api_device_handle_t enclosure_handle;
    fbe_u32_t slot_number;
}fbe_terminator_get_drive_slot_parent_ioctl_t;

typedef struct fbe_terminator_get_terminator_device_count_ioctl_s
{
    fbe_terminator_device_count_t dev_counts;
}fbe_terminator_get_terminator_device_count_ioctl_t;

typedef struct fbe_terminator_set_persistence_request_ioctl_s
{
    fbe_bool_t request;
}fbe_terminator_set_persistence_request_ioctl_t;

typedef struct fbe_terminator_get_persistence_request_ioctl_s
{
    fbe_bool_t request;
}fbe_terminator_get_persistence_request_ioctl_t;

typedef struct fbe_terminator_set_persistence_status_ioctl_s
{
    fbe_cms_memory_persist_status_t status;
}fbe_terminator_set_persistence_status_ioctl_t;

typedef struct fbe_terminator_get_persistence_status_ioctl_s
{
    fbe_cms_memory_persist_status_t status;
}fbe_terminator_get_persistence_status_ioctl_t;

typedef struct fbe_terminator_zero_persistent_memory_ioctl_s
{
    fbe_u32_t unised;
}fbe_terminator_zero_persistent_memory_ioctl_t;

typedef struct fbe_terminator_get_encryption_key_s{
    fbe_key_handle_t key_handle;
    fbe_u8_t         key_buffer[FBE_ENCRYPTION_KEY_SIZE];
    fbe_terminator_api_device_handle_t port_handle;
}fbe_terminator_get_encryption_key_t;

typedef struct fbe_terminator_get_port_address_s{
    fbe_terminator_api_device_handle_t port_handle;
    fbe_address_t address;
}fbe_terminator_get_port_address_t;


fbe_status_t fbe_terminator_init(fbe_packet_t * packet);
fbe_status_t fbe_terminator_destroy(fbe_packet_t * packet);
fbe_status_t fbe_terminator_service_send_packet(fbe_packet_t *packet);


#endif /* FBE_TERMINATOR_SERVICE_H */
