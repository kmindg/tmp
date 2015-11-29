#ifndef FBE_TERMINATOR_FILE_API_H
#define FBE_TERMINATOR_FILE_API_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_terminator_api.h"

extern fbe_u32_t terminator_access_mode;
extern fbe_u8_t terminator_disk_storage_dir[FBE_MAX_PATH];

#define ENUM_TO_STR(_enum) #_enum

#define MAX_LINE_SIZE                   1024
#define MAX_GENERIC_NUMBER_CHARS        18
#define MAX_GENERIC_STRING_CHARS        50
#define MAX_SN_CHARS                    20
#define MAX_UID_CHARS                   FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE
#define MAX_CONFIG_FILE_NAME            100

/* read 100-byte lines during XML format detection */
#define XML_CONFIGURATION_STRING_LEN 100

typedef enum terminator_configuration_e {
    TC_MISSING,
    TC_INVALID_XML,
    TC_OLD_XML,
    TC_NEW_XML
} terminator_configuration_t;


typedef enum term_device_type_e{
    BOARD_DEVICE,
    SAS_PORT_DEVICE,
    FC_PORT_DEVICE,
    ISCSI_PORT_DEVICE,
    SAS_ENCLOSURE_DEVICE,
    SAS_DRIVE_DEVICE,
    SATA_DRIVE_DEVICE,
    FCOE_PORT_DEVICE
}term_device_type_t;

typedef struct term_device_info_s{
    fbe_queue_element_t         queue_element;/*must be first*/
    fbe_terminator_api_device_handle_t  device_id;
    fbe_u32_t               port_number;
    fbe_terminator_device_ptr_t     handle;
    fbe_u8_t                sn[MAX_SN_CHARS];/*used only in the xml file to identify the devices*/
    term_device_type_t          device_type;
    fbe_u32_t               slot;
    union {
    fbe_terminator_sas_drive_info_t sas_drive_info;
    fbe_terminator_sas_encl_info_t  sas_encl_info;
    fbe_terminator_sas_port_info_t  sas_port_info;
    fbe_terminator_sata_drive_info_t    sata_drive_info;
    }type_info;
}term_device_info_t;

typedef struct board_str_to_enum_s{
    fbe_board_type_t        board_type;
    fbe_u8_t *          board_str;
}board_str_to_enum_t;

typedef struct platform_str_to_enum_s{
    SPID_HW_TYPE     platform_type;
    fbe_u8_t *       platform_str;
}platform_str_to_enum_t;

typedef struct port_str_to_enum_s{
    fbe_port_type_t         port_type;
    fbe_u8_t *              port_str;
}port_str_to_enum_t;

typedef struct encl_str_to_enum_s{
    fbe_sas_enclosure_type_t    encl_type;
    fbe_u8_t *          encl_str;
}encl_str_to_enum_t;

typedef struct sas_drive_str_to_enum_s{
    fbe_sas_drive_type_t        drive_type;
    fbe_u8_t *              drive_str;
}sas_drive_str_to_enum_t;

typedef struct sata_drive_str_to_enum_s{
    fbe_sata_drive_type_t       drive_type;
    fbe_u8_t *                  drive_str;
}sata_drive_str_to_enum_t;


fbe_status_t terminator_file_api_init(void);
fbe_status_t terminator_file_api_destroy(void);
fbe_status_t terminator_file_api_init_new(void);
fbe_status_t terminator_file_api_destroy_new(void);
/* function that loads new XML format */
fbe_status_t terminator_file_api_load_new_configuration(void);
/* function used to determine what XML format we are going to use - old or new */
terminator_configuration_t terminator_file_api_get_configuration_format(fbe_u8_t *file_dir, const char * config_file_name);

const char * terminator_file_api_get_disk_storage_dir(void);
const fbe_u32_t terminator_file_api_get_access_mode(void);

#endif /* FBE_TERMINATOR_FILE_API_H */
