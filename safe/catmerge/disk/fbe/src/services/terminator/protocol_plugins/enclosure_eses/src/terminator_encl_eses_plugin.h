#ifndef TERMINATOR_ENCL_ESES_PLUGIN_H
#define TERMINATOR_ENCL_ESES_PLUGIN_H

#include "terminator_sas_io_api.h"

//GJF Changed as part of rebase: #define TERMINATOR_ESES_LCC_POWER_CYCLE_DELAY 10000 // 10 seconds
#define TERMINATOR_ESES_LCC_POWER_CYCLE_DELAY 4000 // 4 seconds

#define TERMINATOR_ESES_DRIVE_POWER_CYCLE_DELAY 5000 // 5 seconds

/* sense bufffer related */
void build_sense_info_buffer(
    fbe_u8_t *sense_info_buffer, 
    fbe_scsi_sense_key_t sense_key, 
    fbe_scsi_additional_sense_code_t ASC,
    fbe_scsi_additional_sense_code_qualifier_t ASCQ);
void ema_not_ready(fbe_u8_t *sense_info_buffer);
/* End of sense buffer related */

/* Inquiry page related routines protoypes*/
fbe_status_t fbe_terminator_sas_enclosure_get_eses_version_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                fbe_u16_t * eses_version_desc_ptr);
/* End of Inquiry page related routines protoypes*/

/*configuration page related routines protoypes */
fbe_status_t config_page_parse_given_config_page(fbe_u8_t *config_page, 
                                                ses_subencl_desc_struct subencl_desc[],
                                                fbe_u8_t * subencl_slot,
                                                ses_type_desc_hdr_struct type_desc_hdr[],
                                                fbe_u8_t  elem_index_of_first_elem[],
                                                fbe_u16_t offset_of_first_elem[],
                                                ses_ver_desc_struct ver_desc_array[],
                                                terminator_eses_subencl_buf_desc_info_t subencl_buf_desc_info[],
                                                fbe_u8_t *num_subencls,
                                                fbe_u8_t *num_type_desc_headers,
                                                fbe_u8_t *num_ver_descs,
                                                fbe_u8_t *num_buf_descs,
                                                fbe_u16_t *encl_stat_diag_page_size);
fbe_status_t config_page_init_type_desc_hdr_structs(void);

fbe_status_t config_page_get_start_elem_offset_in_stat_page(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            ses_elem_type_enum elem_type,
                                                            fbe_bool_t ignore_num_possible_elems,
                                                            fbe_u8_t num_possible_elems,
                                                            fbe_bool_t consider_sub_encl_id,
                                                            fbe_u8_t sub_encl_id,
                                                            fbe_bool_t ignore_type_desc_text,
                                                            fbe_u8_t *type_desc_text,
                                                            fbe_u16_t *offset);
fbe_status_t config_page_get_start_elem_index_in_stat_page(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            ses_elem_type_enum elem_type,
                                                            fbe_bool_t ignore_num_possible_elems,
                                                            fbe_u8_t num_possible_elems,
                                                            fbe_bool_t ignore_type_desc_text,
                                                            fbe_u8_t *type_desc_text,
                                                            fbe_u8_t *index);

fbe_status_t config_page_get_encl_stat_diag_page_size(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                      fbe_u16_t *page_size);
fbe_status_t config_page_get_current_config_page_copy(fbe_u8_t *config_page_buffer);
fbe_u32_t config_page_get_generation_code (void);
fbe_status_t build_configuration_diagnostic_page(fbe_sg_element_t * sg_list,
                                                 fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t config_page_get_gen_code(fbe_terminator_device_ptr_t virtual_phy_handle,
                                      fbe_u32_t *gen_code);
fbe_status_t config_page_get_num_of_sec_subencls(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                 fbe_u8_t *num_sec_subencls);
fbe_status_t config_page_get_subencl_id(vp_config_diag_page_info_t vp_config_diag_page_info,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t *subencl_id);
fbe_status_t 
config_page_get_subencl_desc_by_subencl_id(vp_config_diag_page_info_t *vp_config_diag_page_info,
                                           fbe_u8_t subencl_id,
                                           ses_subencl_desc_struct **subencl_desc);
fbe_status_t config_page_info_get_subencl_ver_desc_position(terminator_eses_config_page_info_t *config_page_info,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            fbe_u8_t *start_index,
                                                            fbe_u8_t *num_of_ver_descs);

fbe_status_t config_page_initialize_config_page_info(fbe_sas_enclosure_type_t encl_type, vp_config_diag_page_info_t *vp_config_diag_page_info);

fbe_status_t config_page_info_get_buf_id_by_subencl_info(vp_config_diag_page_info_t vp_config_diag_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
                                                        ses_buf_type_enum buf_type,
                                                        fbe_bool_t consider_writable,
                                                        fbe_u8_t writable,
                                                        fbe_bool_t consider_buf_index,
                                                        fbe_u8_t buf_index,
                                                        fbe_bool_t consider_buf_spec_info,
                                                        fbe_u8_t buf_spec_info,
                                                        fbe_u8_t *buf_id);

fbe_status_t config_page_element_exists(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        ses_elem_type_enum elem_type,
                                        fbe_bool_t consider_num_possible_elems,
                                        fbe_u8_t num_possible_elems,
                                        fbe_bool_t consider_type_desc_text,
                                        fbe_u8_t *type_desc_text);
fbe_status_t config_page_info_element_exists(terminator_eses_config_page_info_t *config_page_info,
                                            ses_subencl_type_enum subencl_type,
                                            terminator_eses_subencl_side side,
                                            ses_elem_type_enum elem_type,
                                            fbe_bool_t consider_num_possible_elems,
                                            fbe_u8_t num_possible_elems,
                                            fbe_bool_t consider_type_desc_text,
                                            fbe_u8_t *type_desc_text);

/* end of config page routine prototypes */

/*emc status page related routines */
fbe_status_t build_emc_enclosure_status_diagnostic_page(fbe_sg_element_t * sg_list, 
                                                        fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t process_emc_encl_ctrl_page(fbe_sg_element_t *sg_list,
                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                        fbe_u8_t *sense_buffer);

/* end of emc status page related routines */

/* download microcode status diag page related routines */
fbe_status_t build_download_microcode_status_diagnostic_page(fbe_sg_element_t * sg_list, fbe_terminator_device_ptr_t virtual_phy_handle);
/* end of download microcode status diag page related routines */

fbe_status_t process_payload_send_diagnostic(fbe_payload_ex_t * payload, 
                                             fbe_terminator_device_ptr_t virtual_phy_handle);

/* Buffer commands related routines */
fbe_status_t process_payload_read_buffer(fbe_payload_ex_t * payload, 
                                         fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t process_payload_write_buffer(fbe_payload_ex_t * payload, 
                                         fbe_terminator_device_ptr_t virtual_phy_handle);
/* End of Buffer commands related routines */

/* Interface functions for ESES module into terminator */
fbe_status_t eses_get_config_diag_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                            vp_config_diag_page_info_t **config_diag_page_info);

fbe_status_t eses_get_mode_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                     terminator_eses_vp_mode_page_info_t **vp_mode_page_info);

fbe_status_t eses_get_buf_info_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        fbe_u8_t buf_id,
                                        terminator_eses_buf_info_t **buf_info);
/* End Interface functions for ESES module into terminator */

/*Addl status page related routines */
fbe_status_t build_additional_element_status_diagnostic_page(fbe_sg_element_t * sg_list, 
                                                             fbe_terminator_device_ptr_t virtual_phy_handle);
/* End of Addl status page related routines */

/*Status diagnostic page related routines */
fbe_status_t build_enclosure_status_diagnostic_page(fbe_sg_element_t * sg_list, fbe_terminator_device_ptr_t virtual_phy_handle);
/* end of status diagnostic page related routines */

/* mode commands related routines */
fbe_status_t process_payload_mode_sense_10(fbe_payload_ex_t * payload, 
                                           fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t process_payload_mode_select_10(fbe_payload_ex_t * payload, 
                                            fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t mode_page_initialize_mode_page_info(terminator_eses_vp_mode_page_info_t *vp_mode_page_info);

void calculate_buffer_checksum(fbe_u8_t *buf_ptr, fbe_u32_t buf_length);

/* End of mode commands related routines */

/* EMC statistics Status Diagnostic Page related routines*/
fbe_status_t build_emc_statistics_status_diagnostic_page(fbe_sg_element_t * sg_list, 
                                                         fbe_terminator_device_ptr_t virtual_phy_handle);
/* End of EMC statistics Status Diagnostic Page related routines*/

/* Download microcode page(status and control) related routines */
fbe_status_t 
download_stat_page_initialize_page_info(
    vp_download_microcode_stat_diag_page_info_t *vp_download_microcode_stat_diag_page_info);
fbe_status_t 
download_ctrl_page_initialize_page_info(
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_diag_page_info);

/* Misc. ESES routines*/
fbe_u8_t *
fbe_eses_copy_sg_list_into_contiguous_buffer(fbe_sg_element_t *sg_list);

#endif // #ifndef TERMINATOR_ENCL_ESES_PLUGIN_H
