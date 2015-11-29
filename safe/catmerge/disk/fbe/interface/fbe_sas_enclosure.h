#ifndef FBE_SAS_ENCLOSURE_H
#define FBE_SAS_ENCLOSURE_H

#include "fbe_base_enclosure.h"
#include "fbe_sas.h"
#include "fbe/fbe_enclosure.h"

#define SAS_ENCLOSURE_MAX_SES_CDB_SIZE 6
#define SAS_ENCLOSURE_MAX_SES_RESPONSE_SIZE 512
#define SAS_ENCLOSURE_MAX_SES_CMD_SIZE 300

/* control codes */
typedef enum fbe_sas_enclosure_control_code_e {
	FBE_SAS_ENCLOSURE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_SAS_ENCLOSURE),
    FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PARENT_SAS_ELEMENT,
    FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PHY_TABLE,
#if 0
    FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PRODUCT_TYPE, /* AT - Should be a BASE_OBJECT_CONTROL CODE?? */
    FBE_SAS_ENCLOSURE_CONTROL_CODE_SEND_SCSI_COMMAND,
#endif
	FBE_SAS_ENCLOSURE_CONTROL_CODE_LAST
}fbe_sas_enclosure_control_code_t;

/* FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PARENT_SAS_ELEMENT */
typedef struct fbe_sas_enclosure_mgmt_get_parent_sas_element_s {
    fbe_object_id_t		parent_id;
	fbe_sas_element_t	sas_element;
}fbe_sas_enclosure_mgmt_get_parent_sas_element_t;

enum fbe_sas_enclosure_max_phy_number_e {
	FBE_SAS_ENCLOSURE_MAX_PHY_NUMBER_PER_DEVICE = 30
};

/* FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PHY_TABLE */
typedef struct fbe_sas_enclosure_mgmt_get_phy_table_s {
    fbe_sas_address_t		sas_address;
	fbe_sas_element_t	phy_table[FBE_SAS_ENCLOSURE_MAX_PHY_NUMBER_PER_DEVICE];
}fbe_sas_enclosure_mgmt_get_phy_table_t;

#if 0
/* FBE_SAS_ENCLOSURE_CONTROL_CODE_GET_PRODUCT_TYPE */
typedef struct fbe_sas_enclosure_mgmt_get_product_type_s {
    fbe_object_id_t parent_id;
	fbe_sas_device_type_t product_type;
}fbe_sas_enclosure_mgmt_get_product_type_t;
#endif

typedef struct fbe_sas_enclosure_mgmt_ses_command_s {
    /* AT - Should all these three elements be part of another struct? */
	fbe_u32_t bus_id;
    fbe_u32_t target_id;
    fbe_u32_t lun_id;
    fbe_u8_t cdb[SAS_ENCLOSURE_MAX_SES_CDB_SIZE];
    fbe_u8_t cdb_size;
    fbe_u8_t resp_buffer[SAS_ENCLOSURE_MAX_SES_RESPONSE_SIZE];
    fbe_u32_t resp_buffer_size;
    fbe_u8_t cmd[SAS_ENCLOSURE_MAX_SES_CMD_SIZE];
    fbe_u32_t cmd_size;
}fbe_sas_enclosure_mgmt_ses_command_t;
#endif /* FBE_SAS_ENCLOSURE_H */
