#ifndef FBE_SES_INTERNAL_H
#define FBE_SES_INTERNAL_H

/* Command Opcodes */
#define FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_OPCODE 0x1C
#define FBE_SES_SEND_DIAGNOSTIC_RESULTS_OPCODE    0x1D

/* Receive Diagnostic Page Codes */
enum fbe_ses_pages_e{
	FBE_SES_SUPPORTED_DIAGNOSTIC_PAGE         = 0x00,
	FBE_SES_ENCLOSURE_CONFIGURATION_PAGE      = 0x01,
	FBE_SES_ENCLOSURE_STATUS_PAGE             = 0x02,
	FBE_SES_STRING_IN_PAGE                    = 0x04,
	FBE_SES_ADDITIONAL_ELEMENT_STATUS_PAGE    = 0x0A
};

/* Send Diagnostic Page Codes */
#define FBE_SES_ENCLOSURE_CONTROL_PAGE             0x02

/* Bit to indicate that the foll. data is valid control data that expander 
 * needs to act on i.e. Any time the host want to write control data for
 * a particular device, this bit should be set. 
 */
#define FBE_SES_ENCLOSURE_CONTROL_SELECT_BIT       0x80

/* Byte Offset in the control page for each device */
#define FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_OFFSET 12
#define FBE_SES_ENCLOSURE_CONTROL_ENCLOSURE_CONTROL_OFFSET 132

/* Byte Offset in the staus page for each device */
#define FBE_SES_ENCLOSURE_STATUS_DEVICE_STATUS_OFFSET 8

/* Byte Offset in the Additional Status page for Device info */
#define FBE_SES_ADDITIONAL_ELEMENT_DEVICE_STATUS_OFFSET 8

typedef struct fbe_ses_receive_diagnostic_results_cdb_s
{
    /* Byte 0 */
    fbe_u8_t operation_code;
    /* Byte 1 */
    fbe_u8_t page_code_valid :1;
    fbe_u8_t rsvd : 7;
    /* Byte 2 */
    fbe_u8_t page_code;
    /* Byte 3 */
    fbe_u8_t alloc_length_msbyte;
    /* Byte 4 */
    fbe_u8_t alloc_length_lsbyte;
    /* Byte 5 */
    fbe_u8_t control;
}fbe_ses_receive_diagnostic_results_cdb_t;

typedef struct fbe_ses_send_diagnostic_results_cdb_s
{
    /* Byte 0 */
    fbe_u8_t operation_code;
    /* Byte 1 */
    fbe_u8_t unit_offline :1;
    fbe_u8_t device_offline :1;
    fbe_u8_t self_test :1;
    fbe_u8_t rsvd :1;
    fbe_u8_t page_format :1;
    fbe_u8_t self_test_code : 3;
    /* Byte 2 */
    fbe_u8_t rsvd1;
    /* Byte 3 */
    fbe_u8_t param_list_length_msbyte;
    /* Byte 4 */
    fbe_u8_t param_list_length_lsbyte;
    /* Byte 5 */
    fbe_u8_t control;
}fbe_ses_send_diagnostic_results_cdb_t;

typedef struct fbe_ses_send_diagnostic_page_header_s
{
    fbe_u8_t page_code;
    fbe_u8_t rsvd;
    fbe_u8_t page_length_msbyte;
    fbe_u8_t page_length_lsbyte;
    fbe_u32_t generation_code;
}fbe_ses_send_diagnostic_page_header_t;

/* Control word for device(i.e. Drive) */
typedef struct fbe_ses_enclosure_control_device_control_word_s
{
    /* Byte 0 */
    fbe_u8_t rsvd :4;
    fbe_u8_t rst_swap :1;
    fbe_u8_t rsvd1 :2 ;
    fbe_u8_t select_bit: 1;
    /* Byte 1 */
    fbe_u8_t rsvd2: 8;
    /* Byte 2 */
    fbe_u8_t rsvd3: 1;
    fbe_u8_t rqst_ident:1;
    fbe_u8_t rsvd4: 6;
    /* byte 3 */
    fbe_u8_t rsvd5 :4;
    fbe_u8_t rqst_onoff:1;
    fbe_u8_t rqst_flt:1;
    fbe_u8_t rsvd6:2;
}fbe_ses_enclosure_control_device_control_word_t;

#define FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_WORD_LENGTH sizeof(fbe_ses_enclosure_control_device_control_word_t)
/* Control word for enclosure */
typedef struct fbe_ses_enclosure_control_enclosure_control_word_s
{
    /* Byte 0 */
    fbe_u8_t rsvd :7;
    fbe_u8_t select_bit: 1;
    /* Byte 1 */
    fbe_u8_t rsvd1 :7;
    fbe_u8_t rqst_ident: 1;
    /* Byte 2 */
    fbe_u8_t encl_id:3;
    fbe_u8_t encl_id_change_flag: 1;
    fbe_u8_t rsvd2: 4;
    /* byte 3 */
    fbe_u8_t rqst_reset:1;
    fbe_u8_t rqst_flt:1;
    fbe_u8_t rsvd3:6;
}fbe_ses_enclosure_control_enclosure_control_word_t;

typedef struct fbe_ses_additional_status_device_info_s
{
    /* Byte 0 */
    fbe_u8_t protocol_id :4;
    fbe_u8_t element_index_present : 1;
    fbe_u8_t rsvd : 2;
    fbe_u8_t invalid : 1;
    /* Byte 1 */
    fbe_u8_t length;
    /* Byte 2 */
    fbe_u8_t num_phy_desc;
    /* Byte 3 */
    fbe_u8_t not_all_phy :3;
    fbe_u8_t rsvd1 : 3;
    fbe_u8_t desc_type : 2;
    /* Byte 4 */
    fbe_u8_t rsvd2 : 4;
    fbe_u8_t device_type: 3;
    fbe_u8_t rsvd3 : 1;
    /* Byte 5 */
    fbe_u8_t rsvd4;
    /* Byte 6 */
    fbe_u8_t rsvd5 : 1;
    fbe_u8_t smp_init_port : 1;
    fbe_u8_t stp_init_port : 1;
    fbe_u8_t ssp_init_port : 1;
    fbe_u8_t rsvd6 : 4;
    /* Byte 7 */
    fbe_u8_t sata_device : 1;
    fbe_u8_t smp_target_port : 1;
    fbe_u8_t stp_target_port : 1;
    fbe_u8_t ssp_target_port : 1;
    fbe_u8_t rsvd7 : 3;
    fbe_u8_t port_selector : 1;
    /* Byte 8 -15 */
    fbe_sas_address_t attached_sas_address;
    /* Byte 16 -23 */
    fbe_sas_address_t sas_address;
    /* Byte 24 */
    fbe_u8_t phy_identifier;
    fbe_u8_t rsvd8;
} fbe_ses_additional_status_device_info_t;
#endif /* FBE_SES_INTERNAL_H */