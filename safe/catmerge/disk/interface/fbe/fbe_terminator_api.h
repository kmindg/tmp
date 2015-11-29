#ifndef FBE_TERMINATOR_API_H
#define FBE_TERMINATOR_API_H

/* Includes*/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_board_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_physical_drive.h"
#include "../fbe/interface/fbe_pmc_shim.h"
#include "fbe_cpd_shim.h"

#include "fbe/fbe_eses.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_sata_interface.h" 
#include "fbe/fbe_api_sim_transport.h" 
#include "fbe/fbe_api_terminator_drive_interface.h" 
#include "specl_sfi_types.h"
#include "specl_interface.h"
#include "flare_sgl.h"
#include "fbe_cms.h"

/*Terminator defines*/
#define TERMINATOR_SCSI_INQUIRY_DATA_SIZE           FBE_SCSI_INQUIRY_DATA_SIZE
#define TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE  FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE
#define TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_OFFSET FBE_SCSI_INQUIRY_SERIAL_NUMBER_OFFSET
#define TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE  FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1
#define TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_OFFSET FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET
#define TERMINATOR_SCSI_INQUIRY_REVISION_OFFSET     FBE_SCSI_INQUIRY_REVISION_OFFSET
#define TERMINATOR_SCSI_INQUIRY_REVISION_SIZE       FBE_SCSI_INQUIRY_REVISION_SIZE
#define TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xF3_SIZE  16
#define TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xB2_SIZE  12
#define TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xC0_SIZE  21
#define TERMINATOR_SCSI_READ_CAPACITY_DATA_SIZE     FBE_SCSI_READ_CAPACITY_DATA_SIZE
#define TERMINATOR_SCSI_READ_CAPACITY_DATA_SIZE_16  FBE_SCSI_READ_CAPACITY_DATA_SIZE_16
#define TERMINATOR_SCSI_MODE_PAGE_SIZE  176
#define TERMINATOR_SCSI_MODE_PAGE_10_BYTE_SIZE  236 /* Size of MODE PAGE */
#define TERMINATOR_SCSI_MODE_PAGE_0x19_SIZE  108 /* Size of MODE PAGE 0x19 long format*/
#define TERMINATOR_SCSI_MODE_PAGE_0x04_SIZE  28 /* Size of MODE PAGE 0x04 long format*/        
#define TERMINATOR_SCSI_DIAG_0x82_SIZE  162
#define TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE  148 /* Max Size of LOG PAGE 31 */

#define TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_SIZE    FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE
#define TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_OFFSET  FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET

#define TERMINATOR_SATA_SERIAL_NUMBER_SIZE  FBE_SATA_SERIAL_NUMBER_SIZE
#define TERMINATOR_SATA_SERIAL_NUMBER_OFFSET FBE_SATA_SERIAL_NUMBER_OFFSET

#define TERMINATOR_DISK_NAME_FORMAT_STRING "disk%d_%d_%d"
#define TERMINATOR_DISK_NAME_FORMAT_STRING_SIZE 15

#define TERMINATOR_DISK_FULLPATH_FORMAT_STRING "%sdisk%d_%d_%d"
#define TERMINATOR_DISK_FULLPATH_STRING_SIZE FBE_MAX_PATH + TERMINATOR_DISK_NAME_FORMAT_STRING_SIZE

/* A valid product ID would be like "ST1VMSIMCLAR3000" (refer to drive_table
 *  in terminator_drive_sas_plugin_main.c), so the leading offset would
 *  be the string length of "ST1VMSIM". */
#define TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_LEADING_OFFSET   8

#define TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_SIZE    FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE
#define TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_OFFSET  FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET

#define TERMINATOR_DISK_FILE_NAME_FORMAT             "disk%d_%d_%d"
#define TERMINATOR_DISK_FILE_NAME_LEN                15

#define TERMINATOR_DISK_FILE_FULL_PATH_FORMAT        "%sdisk%d_%d_%d"
#define TERMINATOR_DISK_FILE_FULL_PATH_LEN           FBE_MAX_PATH + TERMINATOR_DISK_FILE_NAME_LEN

#define TERMINATOR_MAX_ENCLOSURE 8
#define TERMINATOR_MAX_SLOT 15

/*Terminator API types*/
typedef fbe_u64_t fbe_terminator_api_device_handle_t;
/* TCM class handle */
typedef void* fbe_terminator_api_device_class_handle_t;

typedef void * fbe_terminator_device_ptr_t;
/* Device reset interface */
typedef fbe_status_t (* fbe_terminator_device_reset_function_t) (fbe_terminator_device_ptr_t device_handle);


/*!****************************************************************************
 * @struct zeus_ready_test_params_s
 ******************************************************************************
 *
 * @brief
 *  This structure is used to hold the number of devices enumerated in the 
 * terminator.  Filled in by 
 *
 *****************************************************************************/
typedef struct fbe_terminator_device_count_s {
    fbe_u32_t total_devices;
    fbe_u32_t num_boards;
    fbe_u32_t num_ports;
    fbe_u32_t num_enclosures;
    fbe_u32_t num_drives;
} fbe_terminator_device_count_t;


/*Structure for loading user info*/
typedef struct fbe_terminator_board_info_s {
    fbe_board_type_t        board_type;
    SPID_HW_TYPE            platform_type;
    /* this queue holds SPECL SFI mask data which are passed in before SFI is ready */
    fbe_queue_head_t       specl_sfi_request_queue_head;
}fbe_terminator_board_info_t;

typedef struct fbe_terminator_sas_port_info_s{
    fbe_sas_address_t       sas_address;
    fbe_port_type_t         port_type;
    fbe_u32_t               io_port_number;
    fbe_u32_t               portal_number;
    fbe_u32_t               backend_number;
    fbe_cpd_shim_port_role_t          port_role;
    fbe_cpd_shim_connect_class_t port_connect_class;

}fbe_terminator_sas_port_info_t;

typedef struct fbe_terminator_fc_port_info_s{
    fbe_diplex_address_t    diplex_address;
    fbe_port_type_t         port_type;
    fbe_u32_t               io_port_number;
    fbe_u32_t               portal_number;
    fbe_u32_t               backend_number;

    fbe_cpd_shim_port_role_t          port_role;
    fbe_cpd_shim_connect_class_t port_connect_class;
}fbe_terminator_fc_port_info_t;

typedef struct fbe_terminator_iscsi_port_info_s{
    //fbe_diplex_address_t    diplex_address; //TODO: IP address
    fbe_port_type_t         port_type;
    fbe_u32_t               io_port_number;
    fbe_u32_t               portal_number;
    fbe_u32_t               backend_number;

    fbe_cpd_shim_port_role_t          port_role;
    fbe_cpd_shim_connect_class_t port_connect_class;
}fbe_terminator_iscsi_port_info_t;

typedef struct fbe_terminator_FCOE_port_info_s{
    //fbe_diplex_address_t    diplex_address; //TODO: IP address
    fbe_port_type_t         port_type;
    fbe_u32_t               io_port_number;
    fbe_u32_t               portal_number;
    fbe_u32_t               backend_number;

    fbe_cpd_shim_port_role_t          port_role;
    fbe_cpd_shim_connect_class_t port_connect_class;
}fbe_terminator_fcoe_port_info_t;

typedef struct fbe_terminator_sas_encl_info_s{
    fbe_sas_address_t           sas_address;
    fbe_sas_enclosure_type_t    encl_type;
    fbe_u8_t                    uid[TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_SIZE];
    /* New field added */
    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;
    fbe_u32_t                   connector_id;
}fbe_terminator_sas_encl_info_t;

typedef struct fbe_terminator_fc_encl_info_s{
    fbe_diplex_address_t           diplex_address;
    fbe_fc_enclosure_type_t    encl_type;
    fbe_u8_t                    uid[TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_SIZE];
    /* New field added */
    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;
}fbe_terminator_fc_encl_info_t;

#define MAX_POSSIBLE_CONNECTOR_ID_COUNT 8

typedef struct fbe_term_encl_connector_list_s
{
    fbe_u8_t num_connector_ids;  // Indicates how many entries in the list are valid.
    fbe_u8_t list[MAX_POSSIBLE_CONNECTOR_ID_COUNT];
} fbe_term_encl_connector_list_t;


/* default info based on drive type*/
typedef struct fbe_terminator_sas_drive_type_default_info_s
{
    fbe_u8_t inquiry[TERMINATOR_SCSI_INQUIRY_DATA_SIZE];
    fbe_u8_t vpd_inquiry_f3[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xF3_SIZE];
    fbe_u8_t vpd_inquiry_b2[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xB2_SIZE];
    fbe_u8_t vpd_inquiry_c0[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xC0_SIZE];
    /* terminator maintains multiple copies of mode pages; for Mode_Sense_6 and Mode_Sense_10
       and various pages.  There should only be one copy. This makes testing very difficult. */
    fbe_u8_t mode_page_for_mode_sense_6[TERMINATOR_SCSI_MODE_PAGE_SIZE];
    fbe_u8_t mode_page_for_mode_sense_10[TERMINATOR_SCSI_MODE_PAGE_10_BYTE_SIZE];
    fbe_u8_t mode_page_0x04[TERMINATOR_SCSI_MODE_PAGE_0x04_SIZE];
    fbe_u8_t mode_page_0x19[TERMINATOR_SCSI_MODE_PAGE_0x19_SIZE]; /* use by mode_sense_6*/

    fbe_u8_t diag_page_0x82[TERMINATOR_SCSI_DIAG_0x82_SIZE];
    fbe_u8_t log_page_31[TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE]; /* wear levelling data */
}fbe_terminator_sas_drive_type_default_info_t;

typedef enum fbe_terminator_drive_default_field_s
{
    FBE_DRIVE_INQUIRY_FIELD_INVALID = 0, 
    FBE_DRIVE_INQUIRY_FIELD_VENDOR,
    FBE_DRIVE_INQUIRY_FIELD_PRODUCT_ID,
    FBE_DRIVE_INQUIRY_FIELD_SERIAL_NUMBER,
    FBE_DRIVE_INQUIRY_FIELD_PART_NUMBER,
    FBE_DRIVE_INQUIRY_FIELD_TLA,        
    FBE_DRIVE_INQUIRY_FIELD_REV,
    FBE_DRIVE_INQUIRY_FIELD_ALL,    /* entire inquiry data buffer*/
    FBE_DRIVE_VPD_INQUIRY_F3_FIELD_ALL,  /* all of vpd f3 */
    FBE_DRIVE_VPD_INQUIRY_B2_FIELD_ALL,
}fbe_terminator_drive_default_field_t;

typedef enum fbe_sas_drive_type_s{
    FBE_SAS_DRIVE_INVALID,

    FBE_SAS_DRIVE_CHEETA_15K,

    FBE_SAS_DRIVE_UNICORN_512,  /* A mythical 512 bytes per block SAS drive.*/
    FBE_SAS_DRIVE_UNICORN_4096, /* A mythical 4096 bytes per block SAS drive.*/
    FBE_SAS_DRIVE_UNICORN_4160, /* A mythical 4160 bytes per block SAS drive.*/

    FBE_SAS_DRIVE_SIM_520,     
	FBE_SAS_NL_DRIVE_SIM_520, 
    FBE_SAS_DRIVE_SIM_512,      

    FBE_SAS_DRIVE_SIM_520_FLASH_HE,    /* Simulation EFD (HE) SSD */

    FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP, /* deprecated by FBE_SAS_DRIVE_SIM_520_FLASH_ME */
    FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP,
    FBE_SAS_DRIVE_SIM_520_UNMAP, 
    FBE_SAS_DRIVE_UNICORN_4160_UNMAP, 

    FBE_SAS_DRIVE_COBRA_E_10K,         /* Cobra E 10K HDD */

    FBE_SAS_DRIVE_SIM_520_12G,         /* Simulation for HDD 12Gb */

    FBE_SAS_DRIVE_SIM_520_FLASH_ME,    /* Simulation (ME) Flash VP */
    FBE_SAS_DRIVE_SIM_520_FLASH_LE,    /* Simulation (LE) Flash VP */
    FBE_SAS_DRIVE_SIM_520_FLASH_RI,    /* Simulation (RI) Flash VP */

    FBE_SAS_DRIVE_LAST
}fbe_sas_drive_type_t;

/* For sake of testing we took below FC drives
   need to update with actual FC drives */
typedef enum fbe_fc_drive_type_s{
        FBE_FC_DRIVE_INVALID,
        FBE_FC_DRIVE_CHEETA_15K,
        FBE_FC_DRIVE_UNICORN_512,  
        FBE_FC_DRIVE_UNICORN_4096, 
        FBE_FC_DRIVE_UNICORN_4160, 
        FBE_FC_DRIVE_SIM_520,   
        FBE_FC_DRIVE_SIM_512,   
        FBE_FC_DRIVE_LAST
}fbe_fc_drive_type_t;

/* This defines the structure for a drive fw image, which is passed down as
   part of a fw upgrade.
*/
typedef struct fbe_terminator_sas_drive_fw_image_s {
    fbe_u8_t inquiry[TERMINATOR_SCSI_INQUIRY_DATA_SIZE];
    fbe_u8_t vpd_inquiry_f3[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xF3_SIZE];
} fbe_terminator_sas_drive_fw_image_t;


typedef struct fbe_terminator_fc_drive_info_s{
    fbe_diplex_address_t        diplex_address;
    fbe_fc_drive_type_t         drive_type;
    fbe_u8_t                    drive_serial_number[TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];

    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;  
    fbe_u32_t                   slot_number;

    fbe_lba_t                   capacity;
    fbe_block_size_t            block_size;
    fbe_u8_t product_id[TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE];
}fbe_terminator_fc_drive_info_t;


typedef struct fbe_terminator_sas_drive_info_s{
    fbe_sas_address_t           sas_address;
    fbe_sas_drive_type_t        drive_type;
    fbe_u8_t                    drive_serial_number[TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];
    /* New fields added */
    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;
    fbe_u32_t                   slot_number;
    fbe_lba_t					capacity;   /* num blocks*/
    fbe_block_size_t			block_size;
    fbe_u8_t product_id[TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE];
}fbe_terminator_sas_drive_info_t; 

typedef enum fbe_sata_drive_type_s{
        FBE_SATA_DRIVE_INVALID,
        FBE_SATA_DRIVE_HITACHI_HUA,
        FBE_SATA_DRIVE_SIM_512,         
        FBE_SATA_DRIVE_SIM_512_FLASH,   
        FBE_SATA_DRIVE_LAST
}fbe_sata_drive_type_t;

typedef struct fbe_terminator_sata_drive_info_s{
    fbe_sas_address_t           sas_address;
    fbe_sata_drive_type_t       drive_type;
    fbe_u8_t                    drive_serial_number[TERMINATOR_SATA_SERIAL_NUMBER_SIZE];
    /* New fields added */
    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;
    fbe_u32_t                   slot_number;
    fbe_lba_t					capacity;
    fbe_block_size_t			block_size;
    fbe_u8_t product_id[TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE];
}fbe_terminator_sata_drive_info_t;

typedef enum terminator_sas_drive_state_e {
    TERMINATOR_SAS_DRIVE_STATE_OK,
    TERMINATOR_SAS_DRIVE_SELECT_TIMEOUT
}terminator_sas_drive_state_t;

/* This allow applications to create drives without a configuration entry. */
typedef struct fbe_terminator_create_sas_drive_info_s{
    fbe_sas_address_t           sas_address;
    fbe_sas_drive_type_t        drive_type;
    fbe_u8_t                    drive_serial_number[TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];
    /* New fields added */
    fbe_u32_t                   backend_number;
    fbe_u32_t                   encl_number;
    fbe_u32_t                   slot_number;
    fbe_terminator_api_device_handle_t drive_handle;
    fbe_lba_t					capacity;
    fbe_block_size_t			block_size;
}fbe_terminator_create_sas_drive_info_t;

/* this is the io file mode defines */
typedef enum fbe_terminator_io_mode_e{
    FBE_TERMINATOR_IO_MODE_ENABLED,
    FBE_TERMINATOR_IO_MODE_DISABLED
}fbe_terminator_io_mode_t;

/* POWER SUPPLY Related structures */

// Each power supply (PSA, PSB--etc) in an encl. can be
// addressed by using these enums
typedef enum terminator_eses_ps_id
{
    PS_0 = 0,
    PS_1 = 1,
    PS_2 = 2,
    PS_3 = 3,
}terminator_eses_ps_id;
// Each Cooling element in an encl. can be
// addressed by using these enums
typedef enum terminator_eses_cooling_id
{
    COOLING_0 = 0,
    COOLING_1 = 1,
    COOLING_2 = 2,
    COOLING_3 = 3,
    COOLING_4 = 4,
    COOLING_5 = 5,
    COOLING_6 = 6,
    COOLING_7 = 7,
    COOLING_8 = 8,
    COOLING_9 = 9,
}terminator_eses_cooling_id;
/* End of POWER SUPPLY related strcutures */

// Each Connector element in an encl. can be
// addressed by using these enums
typedef enum terminator_eses_sas_conn_id
{
    LOCAL_ENTIRE_CONNECTOR_0 = 0,
    LOCAL_ENTIRE_CONNECTOR_1 = 5,
    PEER_ENTIRE_CONNECTOR_0 = 10,
    PEER_ENTIRE_CONNECTOR_1 = 15,
}terminator_eses_sas_conn_id;

// Each temperature sensor in LCC can be
// addressed by using these enums
typedef enum terminator_eses_temp_sensor_id
{
    TEMP_SENSOR_0 = 0,
    TEMP_SENSOR_1 = 1,
    TEMP_SENSOR_2 = 2,
    TEMP_SENSOR_3 = 3,
    TEMP_SENSOR_4 = 4,
    TEMP_SENSOR_5 = 5
   
     
}terminator_eses_temp_sensor_id;

typedef enum terminator_eses_display_character_id
{
    DISPLAY_CHARACTER_0 = 0,
    DISPLAY_CHARACTER_1,
    DISPLAY_CHARACTER_2,
    MAX_DISPLAY_CHARACTERS
}terminator_eses_display_character_id;

typedef enum terminator_eses_resume_prom_id_e
{
    LCC_A_RESUME_PROM,
    LCC_B_RESUME_PROM,
    PS_A_RESUME_PROM,
    PS_B_RESUME_PROM,
    CHASSIS_RESUME_PROM,
    EE_LCC_A_RESUME_PROM,
    EE_LCC_B_RESUME_PROM,
    FAN_1_RESUME_PROM,
    FAN_2_RESUME_PROM,
    FAN_3_RESUME_PROM
}terminator_eses_resume_prom_id_t;

 /**********************************************************************
 * Structure for a NEIT error injection drive
 **********************************************************************/
typedef struct neit_drive_s{
    /* Disk information that specifies a unique drive.
     * Note: This will soom change to port / enclosure / slot
     */
    fbe_u32_t port_number;
    fbe_u32_t enclosure_number;
    fbe_u32_t slot_number;
}neit_drive_t;

/**********************************************************************
* Structure for NEIT error parameters.
*********************************************************************/
typedef struct fbe_terminator_neit_error_record_s{
    /* Disk information for which the error should be inserted.
     */
    neit_drive_t drive;
    /* lba_start and lba_end define the LBA range within
     * which the error should be inserted.
     */
    fbe_lba_t lba_start;
    fbe_lba_t lba_end;
    /* This is the opcode for which the error should be inserted.
     */
    fbe_u8_t opcode;
    /* This is the error the user wants to insert. They are
     * represented here as three values representing the
     * sk/asc/ascq.
     */
    fbe_u8_t error_sk;
    fbe_u8_t error_asc;
    fbe_u8_t error_ascq;
    /* Specifies if the error intended here is a port or regular scsi
     * error insertion. This variable serves as the flag to check
     * if the error is a port error. If it is FBE_TRUE then it is a
     * port error.
     */
    fbe_bool_t is_port_error;
    /*the status the port would return if it's a port error*/
    fbe_port_request_status_t port_status;
    fbe_payload_cdb_scsi_status_t port_force_scsi_status;
    /* Number of times the error should be inserted.
     */
    fbe_u32_t num_of_times_to_insert;
    /* Timestamp when this request was made inactive by being hit
     * num_of_times_to_insert times.
     */
    fbe_time_t max_times_hit_timestamp;
    /* Number of seconds that a record can remain inactive after
     * getting injected num_of_times_to_insert times.
     */
    fbe_u32_t secs_to_reactivate;
    /* This field controls the number of times that the record can be
     * reactivated.
     */
    fbe_u32_t num_of_times_to_reactivate;
    /* Number of times the error is inserted.
     */
    fbe_u32_t times_inserted;
    /* How many times we reset the times_inserted field due to a
     * reactivation.
     */
    fbe_u32_t times_reset;
}fbe_terminator_neit_error_record_t;

typedef enum terminator_simulated_drive_type_e
{
    TERMINATOR_SIMULATED_DRIVE_TYPE_INVALID = 0,

    TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY,
    TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY,
    /* These two enums are mainly used in VM Simulator, if you want to change them
     * please also update "$terminator_parm_key->{"/SimDriveType"}" in
     * NTEnv\k10_install\support\SimSetPeerInfo.pl
     */
    TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE,
    TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE,

    TERMINATOR_SIMULATED_DRIVE_TYPE_LAST,
} terminator_simulated_drive_type_t;

/*! @todo Change simulated drive type to remote memory 
 */
#define TERMINATOR_DEFAULT_SIMULATED_DRIVE_TYPE (TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY)

typedef enum terminator_sp_id_e{
    TERMINATOR_SP_A,
    TERMINATOR_SP_B
}terminator_sp_id_t;

//SFI control entry type
typedef SPECL_STATUS (*speclSFIEntryType)(PSPECL_SFI_MASK_DATA psfiMaskData);

extern fbe_bool_t zero_valid_buffer;

fbe_status_t fbe_terminator_api_set_simulated_drive_type(terminator_simulated_drive_type_t simulated_drive_type);
fbe_status_t fbe_terminator_api_get_simulated_drive_type(terminator_simulated_drive_type_t *simulated_drive_type);

fbe_status_t fbe_terminator_api_disable_zero_valid_buffer(void);

fbe_status_t fbe_terminator_api_init(void);
fbe_status_t fbe_terminator_api_destroy(void);

fbe_status_t fbe_terminator_api_load_config_file (fbe_u8_t *file_dir, fbe_u8_t *file_name);
fbe_status_t fbe_terminator_api_unload_config_file(void);

fbe_status_t fbe_terminator_api_load_disk_storage_dir(fbe_u8_t *disk_storage_dir);
fbe_status_t fbe_terminator_api_load_access_mode(fbe_u32_t access_mode);

fbe_status_t fbe_terminator_api_set_io_mode(fbe_terminator_io_mode_t io_mode);
fbe_status_t fbe_terminator_api_get_io_mode(fbe_terminator_io_mode_t *io_mode);

fbe_status_t fbe_terminator_api_set_io_completion_irql(fbe_bool_t b_is_io_completion_at_dpc);

fbe_status_t fbe_terminator_api_set_io_global_completion_delay(fbe_u32_t global_completion_delay);

fbe_status_t fbe_terminator_api_insert_board (fbe_terminator_board_info_t * board_info);
fbe_status_t fbe_terminator_api_get_board_info(fbe_terminator_board_info_t *board_info);
fbe_status_t fbe_terminator_api_get_board_type(fbe_board_type_t *board_type);
fbe_status_t fbe_terminator_api_get_specl_sfi_entry_func(speclSFIEntryType *function);
fbe_status_t fbe_terminator_api_set_specl_sfi_entry_func(speclSFIEntryType function);
fbe_status_t fbe_terminator_api_send_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA sfi_mask_data);
fbe_status_t fbe_terminator_api_process_specl_sfi_mask_data_queue(void);

typedef fbe_status_t (* fbe_terminator_api_get_board_info_function_t)(fbe_terminator_board_info_t *board_info);
typedef fbe_status_t (* fbe_terminator_api_process_specl_sfi_mask_data_queue_function_t)(void);

fbe_status_t fbe_terminator_api_insert_sas_port (fbe_terminator_sas_port_info_t *port_info, fbe_terminator_api_device_handle_t *port_handle);
fbe_status_t fbe_terminator_api_insert_fc_port (fbe_terminator_fc_port_info_t *port_info, fbe_terminator_api_device_handle_t *port_handle);
fbe_status_t fbe_terminator_api_insert_iscsi_port (fbe_terminator_iscsi_port_info_t *port_info, fbe_terminator_api_device_handle_t *port_handle);
fbe_status_t fbe_terminator_api_insert_fcoe_port (fbe_terminator_fcoe_port_info_t *port_info, fbe_terminator_api_device_handle_t *port_handle);
fbe_status_t fbe_terminator_api_start_port_reset (fbe_terminator_api_device_handle_t port_handle);
fbe_status_t fbe_terminator_api_complete_port_reset (fbe_terminator_api_device_handle_t port_handle);
fbe_status_t fbe_terminator_api_auto_port_reset (fbe_terminator_api_device_handle_t port_handle);
fbe_status_t fbe_terminator_api_wait_port_logout_complete(fbe_terminator_api_device_handle_t port_handle);
fbe_status_t fbe_terminator_api_wait_on_port_reset_clear(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_driver_reset_event_type_t reset_event);
fbe_status_t fbe_terminator_api_logout_all_devices_on_port(fbe_terminator_api_device_handle_t port_handle);
fbe_status_t fbe_terminator_api_login_all_devices_on_port(fbe_terminator_api_device_handle_t port_handle);

/*Drive*/
fbe_status_t fbe_terminator_api_insert_sas_drive (fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_sas_drive_info_t *drive_info,
                                                  fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_pull_drive (fbe_terminator_api_device_handle_t drive_handle);
fbe_status_t fbe_terminator_api_reinsert_drive (fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_api_device_handle_t drive_handle);
fbe_status_t fbe_terminator_api_remove_sas_drive (fbe_terminator_api_device_handle_t drive_handle);
fbe_status_t fbe_terminator_api_force_create_sas_drive (fbe_terminator_sas_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_get_sas_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info);
fbe_status_t fbe_terminator_api_set_sas_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info);

fbe_status_t fbe_terminator_api_get_drive_error_count(fbe_terminator_api_device_handle_t handle, fbe_u32_t  *const error_count_p);
fbe_status_t fbe_terminator_api_clear_drive_error_count(fbe_terminator_api_device_handle_t handle);

fbe_status_t fbe_terminator_api_insert_fc_enclosure (fbe_terminator_api_device_handle_t port_handle,
                                                      fbe_terminator_fc_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *enclosure_handle);
fbe_status_t fbe_terminator_api_insert_fc_drive (fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_fc_drive_info_t *drive_info, 
                                                  fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_force_create_fc_drive (fbe_u32_t slot_number, fbe_terminator_fc_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_set_log_page_31(fbe_terminator_api_device_handle_t drive_handle, 
                                                fbe_u8_t * log_page_31,
                                                fbe_u32_t buffer_length);
fbe_status_t fbe_terminator_api_get_log_page_31(fbe_terminator_api_device_handle_t drive_handle, 
                                                fbe_u8_t * log_page_31,
                                                fbe_u32_t * buffer_length);

/* SATA Drive*/
fbe_status_t fbe_terminator_api_insert_sata_drive(fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_sata_drive_info_t *drive_info,
                                                  fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_remove_sata_drive (fbe_terminator_api_device_handle_t drive_handle);
fbe_status_t fbe_terminator_api_force_create_sata_drive (fbe_terminator_sata_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_terminator_api_get_sata_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info);
fbe_status_t fbe_terminator_api_set_sata_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info);

fbe_status_t fbe_terminator_api_drive_get_state (fbe_terminator_api_device_handle_t drive_handle,
                                                 terminator_sas_drive_state_t * drive_state);
fbe_status_t fbe_terminator_api_drive_set_state (fbe_terminator_api_device_handle_t drive_handle,
                                                 terminator_sas_drive_state_t  drive_state);
fbe_status_t fbe_terminator_api_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t fbe_terminator_api_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t fbe_terminator_api_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, fbe_u8_t *data, fbe_u32_t size);
fbe_status_t fbe_terminator_api_set_drive_product_id(fbe_terminator_api_device_handle_t drive_handle, const fbe_u8_t * product_id);
fbe_status_t fbe_terminator_api_get_drive_product_id(fbe_terminator_api_device_handle_t drive_handle, fbe_u8_t * product_id);
/*Device_common api*/
fbe_status_t fbe_terminator_api_remove_device (fbe_terminator_api_device_handle_t device_handle);
fbe_status_t fbe_terminator_api_unmount_device (fbe_terminator_api_device_handle_t device_handle);
fbe_status_t fbe_terminator_api_force_login_device (fbe_terminator_api_device_handle_t device_handle);
fbe_status_t fbe_terminator_api_force_logout_device (fbe_terminator_api_device_handle_t device_handle);
fbe_status_t fbe_terminator_api_set_device_reset_delay (fbe_terminator_api_device_handle_t device_handle, fbe_u32_t delay_in_ms);
fbe_status_t fbe_terminator_api_register_device_reset_function (fbe_terminator_api_device_handle_t device_handle,
                                                                fbe_terminator_device_reset_function_t reset_function);
/* Interface for the new cpd_device_id */
fbe_status_t fbe_terminator_api_get_device_cpd_device_id(const fbe_terminator_api_device_handle_t device_handle, fbe_miniport_device_id_t *cpd_device_id);
fbe_status_t fbe_terminator_api_reserve_miniport_sas_device_table_index(const fbe_u32_t port_number, const fbe_miniport_device_id_t cpd_device_id);
fbe_status_t fbe_terminator_api_miniport_sas_device_table_force_add(const fbe_terminator_api_device_handle_t device_handle, const fbe_miniport_device_id_t cpd_device_id);
fbe_status_t fbe_terminator_api_miniport_sas_device_table_force_remove(const fbe_terminator_api_device_handle_t device_handle);



/* Enclosure*/
fbe_status_t fbe_terminator_api_insert_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info,
                                                      fbe_terminator_api_device_handle_t *enclosure_handle);
fbe_status_t fbe_terminator_api_remove_sas_enclosure (fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t fbe_terminator_api_pull_sas_enclosure (fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t fbe_terminator_api_reinsert_sas_enclosure (fbe_terminator_api_device_handle_t port_handle,
                                                      fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t fbe_terminator_api_get_sas_enclosure_info(fbe_terminator_api_device_handle_t enclosure_handle, fbe_terminator_sas_encl_info_t *encl_info);
fbe_status_t fbe_terminator_api_set_sas_enclosure_info(fbe_terminator_api_device_handle_t enclosure_handle, fbe_terminator_sas_encl_info_t  encl_info);
fbe_status_t fbe_terminator_api_set_need_update_enclosure_firmware_rev(fbe_bool_t  update_rev);
fbe_status_t fbe_terminator_api_get_need_update_enclosure_firmware_rev(fbe_bool_t  *update_rev);
fbe_status_t fbe_terminator_api_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t need_update_checksum);
fbe_status_t fbe_terminator_api_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t *need_update_checksum);

/*ESES*/
fbe_status_t fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                         fbe_u32_t slot_number,
                                                                         ses_stat_elem_array_dev_slot_struct drive_slot_stat);
fbe_status_t fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                         fbe_u32_t slot_number,
                                                                         ses_stat_elem_array_dev_slot_struct *drive_slot_stat);
fbe_status_t fbe_terminator_api_set_sas_enclosure_phy_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                  fbe_u32_t phy_number,
                                                                  ses_stat_elem_exp_phy_struct phy_stat);
fbe_status_t fbe_terminator_api_get_sas_enclosure_phy_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                  fbe_u32_t phy_number,
                                                                  ses_stat_elem_exp_phy_struct *phy_stat);

fbe_status_t fbe_terminator_api_get_ps_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct *ps_stat);
fbe_status_t fbe_terminator_api_set_ps_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct ps_stat);

fbe_status_t fbe_terminator_api_get_sas_conn_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                         terminator_eses_sas_conn_id sas_conn_id,
                                                        ses_stat_elem_sas_conn_struct *sas_conn_stat);

fbe_status_t fbe_terminator_api_set_sas_conn_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct sas_conn_stat);

fbe_status_t fbe_terminator_api_get_lcc_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        ses_stat_elem_encl_struct *lcc_stat);
fbe_status_t fbe_terminator_api_set_lcc_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        ses_stat_elem_encl_struct lcc_stat);

fbe_status_t fbe_terminator_api_get_cooling_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct *cooling_stat);
fbe_status_t fbe_terminator_api_set_cooling_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct cooling_stat);

fbe_status_t fbe_terminator_api_set_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t fbe_terminator_api_get_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct *temp_sensor_stat);

fbe_status_t fbe_terminator_api_set_overall_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                    terminator_eses_temp_sensor_id temp_sensor_id,
                                                                    ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t fbe_terminator_api_get_overall_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                    terminator_eses_temp_sensor_id temp_sensor_id,
                                                                    ses_stat_elem_temp_sensor_struct *temp_sensor_stat);
fbe_status_t fbe_terminator_api_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);
fbe_status_t fbe_terminator_api_enclosure_bypass_drive_slot (fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t slot_number);
fbe_status_t fbe_terminator_api_enclosure_unbypass_drive_slot (fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t slot_number);
fbe_status_t fbe_terminator_api_enclosure_getEmcEnclStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                            ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t fbe_terminator_api_enclosure_setEmcEnclStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                            ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t fbe_terminator_api_enclosure_getEmcPsInfoStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                              ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t fbe_terminator_api_enclosure_setEmcPsInfoStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                              ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t fbe_terminator_api_eses_increment_config_page_gen_code(fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t fbe_terminator_api_set_unit_attention(fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t fbe_terminator_api_eses_set_ver_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t  side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct ver_desc);
fbe_status_t fbe_terminator_api_eses_get_ver_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct *ver_desc);
fbe_status_t fbe_terminator_api_eses_get_subencl_id(fbe_terminator_api_device_handle_t enclosure_handle,
                                                    ses_subencl_type_enum subencl_type,
                                                    fbe_u8_t side,
                                                    fbe_u8_t *subencl_id);
fbe_status_t fbe_terminator_api_eses_set_download_microcode_stat_page_stat_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                                fbe_download_status_desc_t download_stat_desc);
fbe_status_t fbe_terminator_api_set_resume_prom_info(fbe_terminator_api_device_handle_t enclosure_handle,
                                                     terminator_eses_resume_prom_id_t resume_prom_id,
                                                     fbe_u8_t *buf,
                                                     fbe_u32_t len);
fbe_status_t fbe_terminator_api_set_buf(fbe_terminator_api_device_handle_t enclosure_handle,
                                        ses_subencl_type_enum subencl_type,
                                        fbe_u8_t side,
                                        ses_buf_type_enum buf_type,
                                        fbe_u8_t *buf,
                                        fbe_u32_t len);
fbe_status_t fbe_terminator_api_set_buf_by_buf_id(fbe_terminator_api_device_handle_t enclosure_handle,
                                                fbe_u8_t buf_id,
                                                fbe_u8_t *buf,
                                                fbe_u32_t len);
fbe_status_t fbe_terminator_api_set_enclosure_firmware_activate_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t time_interval);
fbe_status_t fbe_terminator_api_get_enclosure_firmware_activate_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t *time_interval);
fbe_status_t fbe_terminator_api_set_enclosure_firmware_reset_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t time_interval);
fbe_status_t fbe_terminator_api_get_enclosure_firmware_reset_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t *time_interval);

/* Driver error injection interfaces */
fbe_status_t fbe_terminator_api_drive_error_injection_init(void);
fbe_status_t fbe_terminator_api_drive_error_injection_destroy(void);
fbe_status_t fbe_terminator_api_drive_error_injection_start(void);
fbe_status_t fbe_terminator_api_drive_error_injection_stop(void);
fbe_status_t fbe_terminator_api_drive_error_injection_add_error(fbe_terminator_neit_error_record_t record);
fbe_status_t fbe_terminator_api_drive_error_injection_init_error(fbe_terminator_neit_error_record_t* record);
fbe_status_t fbe_terminator_api_drive_payload_insert_error(fbe_payload_cdb_operation_t  *payload_cdb_operation,
                                                           fbe_terminator_api_device_handle_t drive_handle);

/*!**************************************************************************
 * @file fbe_terminator_api.h
 ***************************************************************************
 *
 * @brief
 *  This file contains public interface for the Terminator.
 *
 * @date 11/07/2008
 * @author VG
 *
 ***************************************************************************/

/* Class management */
/*! @fn fbe_status_t fbe_terminator_api_find_device_class(
 *                      const char * device_class_name,
 *                      fbe_terminator_api_device_class_handle_t * device_class_handle)
 *
 *  @brief Terminator Class Management function that finds TCM class by given name
 *  @param device_class_name - name of device class (Drive, Enclosure, etc.)
 *  @param device_class_handle - receives TCM handle of this class in case of success
 *  @return status of the call
 */
fbe_status_t fbe_terminator_api_find_device_class(
                        const char * device_class_name,
                        fbe_terminator_api_device_class_handle_t * device_class_handle);

/*! @fn fbe_status_t fbe_terminator_api_create_device_class_instance(
 *                      fbe_terminator_api_device_class_handle_t device_class_handle,
 *                      const char * device_type,
 *                      fbe_terminator_device_ptr_t * device_handle)
 *
 * @brief function used to create instance of given device class
 * @param device_class_handle - handle to device class we are going to instantiate
 * @param device_type - parameter used to specify type of device, if there are
 *                      more than one type of these devices supported (SAS and
 *                      SATA drives, for instance)
 * @param device_handle - receives handle to new instance of device
 * @return status of the call
 */
fbe_status_t fbe_terminator_api_create_device_class_instance(
                        fbe_terminator_api_device_class_handle_t device_class_handle,
                        const char * device_type,
                        fbe_terminator_api_device_handle_t * device_handle);

/*! @fn fbe_status_t fbe_terminator_api_set_device_attribute(
 *                                  fbe_terminator_api_device_handle_t device_handle,
 *                                  const char * attribute_name,
 *                                  const char * attribute_value)
 *
 *  @brief Convenient way to invoke an accessor
 *  @param device_handle - handle to the device which attribute will be modified
 *  @param attribute_name - name of attribute
 *  @param attribute_value - string representing new value of attribute
 *  @return status of the call
 *
 * This function does following:
 *  - for given Terminator device handle device_handle it looks for its type;
 *  - it looks for TCM class for this type of device;
 *  - it tries to find an accessor (string_string_setter) for attribute with given name (parameter attribute_name) using TCM;
 *  - it tries to set value of this attribute (parameter attribute_value) invoking this accessor.
 */
fbe_status_t fbe_terminator_api_set_device_attribute(fbe_terminator_api_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);



/*! @fn fbe_status_t fbe_terminator_api_get_device_attribute(fbe_terminator_api_device_handle_t device_handle,
 *                                                           const char * attribute_name,
 *                                                           char * attribute_value_buffer,
 *                                                           fbe_u32_t buffer_length)
 *
 *  @brief Convenient way to invoke an accessor
 *  @param device_handle - handle to the device which attribute will be obtained
 *  @param attribute_name - name of attribute
 *  @param attribute_value_buffer - buffer that will receive 0-terminated
 *                                  string representing value of attribute
 *  @param buffer_length - size of attribute_value_buffer
 *  @return status of the call
 *
 * This function does following:
 *  - for given Terminator device handle device_handle it looks for its type;
 *  - it looks for TCM class for this type of device;
 *  - it tries to find an accessor (string_string_getter) for attribute with given name (parameter attribute_name) using TCM;
 *  - it tries to get value of this attribute invoking this accessor.
 */
fbe_status_t fbe_terminator_api_get_device_attribute(fbe_terminator_api_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length);

/* Device management */
/*! @fn fbe_status_t fbe_terminator_api_create_device(
 *                      fbe_terminator_api_device_type_t device_type,
 *                      fbe_terminator_api_device_handle_t *device_handle)
 *
 *  @brief Creation function for the Terminator object.
 *  @param device_type - the type of device to create. IN
 *  @param *device_handle - the device handle. OUT
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_create_device(fbe_terminator_api_device_type_t device_type,
//                                              fbe_terminator_api_device_handle_t *device_handle);

/*! @fn fbe_status_t fbe_terminator_api_destroy_device(
 *                      fbe_terminator_api_device_handle_t device_handle)
 *
 *  @brief Destroy function for the Terminator object.
 *  @param device_handle - the handle to the device to be destroyed. IN
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_destroy_device(fbe_terminator_api_device_handle_t device_handle);

/*! @fn fbe_u32_t fbe_terminator_api_get_devices_count(void)
 *
 *  @brief Function that return number of devices that are in the Terminator.
 *  @return number of devices in Terminator (0, if Terminator is not initialized)
 *
 */
fbe_u32_t fbe_terminator_api_get_devices_count(void);

/*! @fn fbe_status_t fbe_terminator_api_get_devices_count_by_type_name(const fbe_u8_t * device_type_name, fbe_u32_t  *const device_count)
 *
 *  @brief Function that return number of devices that are in the Terminator.
 *  @device_type_name - type of devices to count. IN
 *  @param device_count - number of devices in Terminator (0, if Terminator is not initialized). OUT
 *  @return the status of the call
 *
 */
fbe_status_t fbe_terminator_api_get_devices_count_by_type_name(const fbe_u8_t *device_type_name, fbe_u32_t  *const device_count);
/*! @fn fbe_status_t fbe_terminator_api_enumerate_devices(
 *                      fbe_terminator_api_device_handle_t device_handle_array[],
 *                      fbe_u32_t number_of_devices)
 *
 *  @brief Function that enumerate all the devices that are in the Terminator.
 *  @param device_handle_array - array which would be used to store handles of devices
 *  @param number_of_devices - size of this array (use fbe_terminator_api_get_devices_count() to determine its minimal size)
 *  @return the status of the call.
 */
fbe_status_t fbe_terminator_api_enumerate_devices(fbe_terminator_api_device_handle_t device_handle_array[],
                                                  fbe_u32_t number_of_devices);

/*! @fn fbe_status_t fbe_terminator_api_get_device_type(
 *                      fbe_terminator_api_device_handle_t device_handle,
 *                      fbe_terminator_api_device_type_t * device_type)
 *
 *  @brief Function that gets the device type.
 *  @param device_handle - the handle to the device to be looked up. IN
 *  @param device_type - the type of device. OUT
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_get_device_type(fbe_terminator_api_device_handle_t device_handle,
//                                                fbe_terminator_api_device_type_t * device_type);


/*! @fn fbe_status_t fbe_terminator_api_set_device_attributes(
 *                      fbe_terminator_device_ptr_t device_handle,
 *                      fbe_terminator_api_device_attributes_t * device_attributes)
 *
 *  @brief Function that sets the device with given attributes.
 *  @param device_handle - the handle to the device to be looked up. IN
 *  @param device_attributes - the attributes to be set. IN
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_set_device_attributes(fbe_terminator_device_ptr_t device_handle,
//                                                    fbe_terminator_api_device_attributes_t * device_attributes);

/*! @fn fbe_status_t fbe_terminator_api_get_device_attributes(
 *                      fbe_terminator_device_ptr_t device_handle,
 *                      fbe_terminator_api_device_attributes_t * device_attributes)
 *
 *  @brief Function that gets the attributes of given device.
 *  @param device_handle - the handle of the device to be looked up. IN
 *  @param device_attributes - the attributes to be filled in. OUT
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_get_device_attributes(fbe_terminator_device_ptr_t device_handle,
//                                                      fbe_terminator_api_device_attributes_t * device_attributes);


/* Topology Management */
/*! @fn fbe_status_t fbe_terminator_api_insert_device(fbe_terminator_device_ptr_t parent_device,
 *                                             fbe_terminator_device_ptr_t child_device)
 *
 *  @brief Function that insert a device to the child list of the parent device.
 *  @param parent_device - the handle of the parent. IN
 *  @param child_device - the handle of the child. OUT
 *  @return the status of the call.
 */
fbe_status_t fbe_terminator_api_insert_device(fbe_terminator_api_device_handle_t parent_device,
                                              fbe_terminator_api_device_handle_t child_device);

/*! @fn fbe_status_t fbe_terminator_api_remove_device(fbe_terminator_device_ptr_t device_handle)
 *
 *  @brief Function that remove the device and all it's children from the topology.
 *  @param parent_device - the handle of the device. IN
 *  @return the status of the call.
 */
//fbe_status_t fbe_terminator_api_remove_device(fbe_terminator_device_ptr_t device_handle);

/*! @fn fbe_status_t fbe_terminator_api_activate_device(fbe_terminator_device_ptr_t device_handle)
 *
 *  @brief Function that active a device so it is visable to miniport API.
 *  @param device_handle - the handle of the device. IN
 *  @return the status of the call.
 */
fbe_status_t fbe_terminator_api_activate_device(fbe_terminator_api_device_handle_t device_handle);

/*! @fn fbe_status_t fbe_terminator_api_complete_forced_device_creation(fbe_terminator_device_ptr_t device_handle)
 *
 *  @brief Function that finishes forced creation of device (used instead of activation function in case of forced creation of device)
 *  @param device_handle - the handle of the device. IN
 *  @return the status of the call
 */
//fbe_status_t fbe_terminator_api_complete_forced_device_creation(fbe_terminator_api_device_handle_t device_handle);

/* obtain ID of device */
fbe_status_t fbe_terminator_api_get_device_base_component_id(fbe_terminator_api_device_handle_t device_handle,
                                                             fbe_miniport_device_id_t * device_id);

/* obtain the board handle */
fbe_status_t fbe_terminator_api_get_board_handle(fbe_terminator_api_device_handle_t * board_handle);

/* obtain port handle by its number */
fbe_status_t fbe_terminator_api_get_port_handle(fbe_u32_t port_number, fbe_terminator_api_device_handle_t *port_handle);

/* obtain enclosure handle by port number and number of enclosure */
fbe_status_t fbe_terminator_api_get_enclosure_handle(fbe_u32_t port_number,
                                                     fbe_u32_t enclosure_number,
                                                     fbe_terminator_api_device_handle_t * enclosure_handle);

/* obtain enclosure handle by port number and number of enclosure -- TODO */
fbe_status_t fbe_terminator_api_get_enclosure_handle_by_chain_depth(fbe_u32_t port_number,
                                                     fbe_u8_t enclosure_chain_depth,
                                                     fbe_terminator_api_device_handle_t * enclosure_handle);

/* obtain drive handle by port number, number of enclosure and slot number*/
fbe_status_t fbe_terminator_api_get_drive_handle(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_terminator_api_device_handle_t * drive_handle);


/* obtain drive handle by port number, enclosure chain depth and slot number -- TODO */
fbe_status_t fbe_terminator_api_get_drive_handle_by_chain_depth_and_slot(fbe_u32_t port_number,
                                                 fbe_u8_t enclosure_chain_depth,
                                                 fbe_u32_t slot_number,
                                                 fbe_terminator_api_device_handle_t * drive_handle);

typedef fbe_status_t (* fbe_terminator_api_get_sp_id_function_t)(terminator_sp_id_t *sp_id);
typedef fbe_status_t (* fbe_terminator_api_is_single_sp_system_function_t)(fbe_bool_t *is_single);

fbe_status_t fbe_terminator_api_init_with_sp_id(terminator_sp_id_t sp_id);
fbe_status_t fbe_terminator_api_set_sp_id(terminator_sp_id_t sp_id);
fbe_status_t fbe_terminator_api_get_sp_id(terminator_sp_id_t *sp_id);
fbe_status_t fbe_terminator_api_set_single_sp(fbe_bool_t is_single);
fbe_status_t fbe_terminator_api_is_single_sp_system(fbe_bool_t *is_single);


typedef fbe_status_t (* fbe_terminator_api_get_cmi_port_base_function_t)(fbe_u16_t *port_base);

fbe_status_t fbe_terminator_api_set_cmi_port_base(fbe_u16_t port_base);
fbe_status_t fbe_terminator_api_get_cmi_port_base(fbe_u16_t *port_base);
fbe_status_t fbe_terminator_api_set_simulated_drive_server_port(fbe_u16_t port);
fbe_status_t fbe_terminator_api_get_simulated_drive_server_port(fbe_u16_t *port);
fbe_status_t fbe_terminator_api_set_simulated_drive_server_name(const char* server_name);
fbe_status_t fbe_terminator_api_get_simulated_drive_server_name(char **simulated_server_name_pp);
fbe_status_t fbe_terminator_api_set_simulated_drive_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                                fbe_u32_t first_term_drive_index,
                                                                fbe_u32_t last_term_drive_index,
                                                                fbe_u32_t backend_bus_number,
                                                                fbe_u32_t encl_number,
                                                                fbe_u32_t slot_number,
                                                                fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags);
fbe_status_t fbe_terminator_api_get_hardware_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t fbe_terminator_api_set_hardware_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t fbe_terminator_api_get_sfp_media_interface_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t fbe_terminator_api_set_sfp_media_interface_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t fbe_terminator_api_get_port_link_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_lane_info);
fbe_status_t fbe_terminator_api_set_port_link_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_lane_info);
fbe_status_t fbe_terminator_api_get_encryption_key(fbe_terminator_api_device_handle_t port_handle, 
                                                   fbe_key_handle_t key_handle,
                                                   fbe_u8_t *key_buffer);
fbe_status_t fbe_terminator_api_get_port_address(fbe_terminator_api_device_handle_t port_handle,
                                                 fbe_address_t *address);
    
fbe_status_t fbe_terminator_api_enclosure_find_slot_parent (fbe_terminator_api_device_handle_t *enclosure_handle, 
                                                 fbe_u32_t *slot_number);
fbe_status_t fbe_terminator_api_get_connector_id_list_for_enclosure (fbe_terminator_api_device_handle_t enclosure_handle,
                                                        fbe_term_encl_connector_list_t *connector_ids);

fbe_status_t fbe_terminator_api_count_terminator_devices(fbe_terminator_device_count_t *dev_counts);

fbe_status_t fbe_terminator_persistent_memory_set_persistence_request(fbe_bool_t);
fbe_status_t fbe_terminator_persistent_memory_get_persistence_request(fbe_bool_t *);
fbe_status_t fbe_terminator_persistent_memory_set_persistence_status(fbe_cms_memory_persist_status_t);
fbe_status_t fbe_terminator_persistent_memory_get_persistence_status(fbe_cms_memory_persist_status_t *);
fbe_status_t fbe_terminator_persistent_memory_get_sgl(SGL *);
fbe_status_t fbe_terminator_persistent_memory_zero_memory(void);

#endif /*FBE_TERMINATOR_API_H*/
