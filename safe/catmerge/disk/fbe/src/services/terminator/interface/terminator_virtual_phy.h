#ifndef TERMINATOR_VIRTUAL_PHY_H
#define TERMINATOR_VIRTUAL_PHY_H

#include "terminator_base.h"
#include "fbe/fbe_types.h"
#include "fbe_sas.h"


#define MAX_POSSIBLE_DRIVE_SLOTS    60
#define MAX_POSSIBLE_SINGLE_LANE_CONNS_PER_PORT 8
#define MAX_POSSIBLE_ESES_PAGES 20


typedef struct terminator_enclosure_firmware_new_rev_record_s
{
    fbe_queue_element_t queue_element;
    ses_subencl_type_enum subencl_type;
    terminator_eses_subencl_side side;
    ses_comp_type_enum  comp_type;
    fbe_u32_t slot_num;
    CHAR   new_rev_number[16];
} terminator_enclosure_firmware_new_rev_record_t;

typedef struct terminator_sas_virtual_phy_info_s{
    fbe_cpd_shim_callback_login_t login_data;
    fbe_sas_enclosure_type_t enclosure_type;

    ses_stat_elem_array_dev_slot_struct *drive_slot_status;
    fbe_u8_t *drive_slot_insert_count;
    fbe_u8_t *drive_power_down_count;

    ses_stat_elem_exp_phy_struct *phy_status;

    ses_stat_elem_ps_struct *ps_status;
    ses_stat_elem_sas_conn_struct *sas_conn_status;

    ses_stat_elem_cooling_struct *cooling_status;

    ses_stat_elem_temp_sensor_struct *temp_sensor_status;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_status;

    ses_stat_elem_display_struct *display_status;

    // There is one enclosure element for each LCC subenclosure
    // and chassis subenclosure. There is no ability to control
    // indicators in the peer LCC's, so no structure defined for
    // peer for now.
    ses_stat_elem_encl_struct encl_status;

    ses_stat_elem_encl_struct chassis_encl_status;

    ses_pg_emc_encl_stat_struct emcEnclStatus;

    ses_ps_info_elem_struct emcPsInfoStatus;

    ses_general_info_elem_array_dev_slot_struct * general_info_elem_drive_slot;   

    // dont declare as bool , as in future we may want to set
    // unit attention N number of times--etc.
    fbe_u8_t unit_attention;

    // All the eses info related to ESES pages other
    // than "Status elements"(status page) is
    // is stored here
    terminator_vp_eses_page_info_t eses_page_info;
    fbe_u32_t           miniport_sas_device_table_index;
    // queue holds new firmware revision numbers for different components
    fbe_queue_head_t    new_firmware_rev_queue_head;
    // time intervals to specify how long we need to wait before
    // starting logging out and logging in LCC subenclosure in ms
    fbe_u32_t activate_time_intervel;
    fbe_u32_t reset_time_intervel;
}terminator_sas_virtual_phy_info_t;

typedef struct terminator_virtual_phy_s{
    base_component_t base;
}terminator_virtual_phy_t;

typedef struct terminator_sas_encl_eses_info_s{
    fbe_sas_enclosure_type_t encl_type;
    fbe_u8_t max_drive_count;
    fbe_u8_t max_phy_count;
    fbe_u8_t max_encl_conn_count; // not used currently
    fbe_u8_t max_lcc_conn_count;  // max sas coonnector count per LCC.
    fbe_u8_t max_port_conn_count; // max connectors per port ( the number of single lanes + one entire connector)
    fbe_u8_t max_single_lane_port_conn_count; // The number of single lanes
    fbe_u8_t max_ps_count;
    fbe_u8_t max_cooling_elem_count;
    fbe_u8_t max_temp_sensor_elems_count;  // Per LCC
    fbe_u8_t drive_slot_to_phy_map[MAX_POSSIBLE_DRIVE_SLOTS];
    fbe_u8_t max_diplay_character_count;
    fbe_u8_t max_lcc_count;
    fbe_u8_t max_ee_lcc_count;
    fbe_u8_t max_ext_cooling_elem_count;
    fbe_u8_t max_bem_cooling_elem_count;  //cuurently only jetfire has this type of fan
    fbe_u8_t max_conn_id_count;
    fbe_u8_t individual_conn_to_phy_map[MAX_POSSIBLE_CONNECTOR_ID_COUNT][MAX_POSSIBLE_SINGLE_LANE_CONNS_PER_PORT];
    fbe_u8_t conn_id_to_drive_start_slot[MAX_POSSIBLE_CONNECTOR_ID_COUNT];
}terminator_sas_encl_eses_info_t;

typedef struct terminator_sas_unsupported_eses_page_info_s{
    fbe_u8_t unsupported_receive_diag_pages[MAX_POSSIBLE_ESES_PAGES];
    fbe_u32_t unsupported_receive_diag_page_count;
    fbe_u8_t unsupported_send_diag_pages[MAX_POSSIBLE_ESES_PAGES];
    fbe_u32_t unsupported_send_diag_page_count;
}terminator_sas_unsupported_eses_page_info_t;

/* definition of a terminator_eses_buf_node for holding buf memory to avoid the memory leak */
typedef struct terminator_eses_free_mem_node_s 
{
   fbe_u8_t *buf;
   struct terminator_eses_free_mem_node_s *next;
}terminator_eses_free_mem_node_t;

terminator_virtual_phy_t * virtual_phy_new(void);
terminator_sas_virtual_phy_info_t * sas_virtual_phy_info_new(
    fbe_sas_enclosure_type_t encl_type, 
    fbe_sas_address_t sas_address);

fbe_cpd_shim_callback_login_t * sas_virtual_phy_info_get_login_data(terminator_sas_virtual_phy_info_t * self);
fbe_status_t sas_virtual_phy_info_set_enclosure_type(terminator_sas_virtual_phy_info_t * self, fbe_sas_enclosure_type_t enclosure_type);
fbe_sas_enclosure_type_t sas_virtual_phy_info_get_enclosure_type(terminator_sas_virtual_phy_info_t * self);
fbe_status_t sas_virtual_phy_call_io_api(terminator_virtual_phy_t * device, fbe_terminator_io_t * terminator_io);
fbe_status_t sas_virtual_phy_get_phy_eses_status(terminator_virtual_phy_t * self, fbe_u32_t phy_number, ses_stat_elem_exp_phy_struct *exp_phy_stat);
fbe_status_t sas_virtual_phy_set_phy_eses_status(terminator_virtual_phy_t * self, fbe_u32_t phy_number, ses_stat_elem_exp_phy_struct exp_phy_stat);
fbe_status_t sas_virtual_phy_set_sas_address(terminator_virtual_phy_t * self, fbe_sas_address_t sas_address);

fbe_status_t sas_virtual_phy_set_drive_eses_status(terminator_virtual_phy_t * self, fbe_u32_t slot_number, ses_stat_elem_array_dev_slot_struct drive_status);
fbe_status_t sas_virtual_phy_get_drive_eses_status(terminator_virtual_phy_t * self, fbe_u32_t slot_number, ses_stat_elem_array_dev_slot_struct *drive_status);
fbe_status_t sas_virtual_phy_get_drive_slot_to_phy_mapping(fbe_u8_t drive_slot, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type);
fbe_status_t sas_virtual_phy_get_individual_conn_to_phy_mapping(fbe_u8_t individual_lane, fbe_u8_t connector_id, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type);
fbe_status_t sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping(fbe_sas_enclosure_type_t encl_type,
                                                  fbe_u8_t connector_id, 
                                                  fbe_u8_t * drive_start_slot);
fbe_bool_t sas_virtual_phy_phy_corresponds_to_drive_slot(fbe_u8_t phy_id, fbe_u8_t *drive_slot, fbe_sas_enclosure_type_t encl_type);

fbe_status_t sas_virtual_phy_set_drive_slot_insert_count(terminator_virtual_phy_t * self, 
                                                         fbe_u32_t slot_number, 
                                                         fbe_u8_t insert_count);
fbe_status_t sas_virtual_phy_get_drive_slot_insert_count(terminator_virtual_phy_t * self, 
                                                         fbe_u32_t slot_number, 
                                                         fbe_u8_t *insert_count);
fbe_status_t sas_virtual_phy_set_drive_power_down_count(terminator_virtual_phy_t * self, 
                                                        fbe_u32_t slot_number, 
                                                        fbe_u8_t power_down_count);
fbe_status_t sas_virtual_phy_get_drive_power_down_count(terminator_virtual_phy_t * self, 
                                                        fbe_u32_t slot_number, 
                                                        fbe_u8_t *power_down_count);
fbe_status_t sas_virtual_phy_clear_drive_power_down_counts(terminator_virtual_phy_t * self);
fbe_status_t sas_virtual_phy_clear_drive_slot_insert_counts(terminator_virtual_phy_t * self);


fbe_bool_t sas_virtual_phy_phy_corresponds_to_connector(
    fbe_u8_t phy_id, 
    fbe_u8_t *connector, 
    fbe_u8_t *connector_id,
    fbe_sas_enclosure_type_t encl_type);
fbe_status_t sas_virtual_phy_set_ps_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_ps_id ps_id, 
    ses_stat_elem_ps_struct ps_stat);
fbe_status_t sas_virtual_phy_get_ps_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_ps_id ps_id, 
    ses_stat_elem_ps_struct *ps_stat);
fbe_status_t sas_virtual_phy_get_emcEnclStatus(terminator_virtual_phy_t * self,
                                               ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t sas_virtual_phy_set_emcEnclStatus(terminator_virtual_phy_t * self,
                                               ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t sas_virtual_phy_get_emcPsInfoStatus(terminator_virtual_phy_t * self,
                                                 ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t sas_virtual_phy_set_emcPsInfoStatus(terminator_virtual_phy_t * self,
                                                 ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t sas_virtual_phy_get_emcGeneralInfoDirveSlotStatus(terminator_virtual_phy_t * self,
                                                 ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                                 fbe_u8_t drive_slot);
fbe_status_t sas_virtual_phy_set_emcGeneralInfoDirveSlotStatus(terminator_virtual_phy_t * self,
                                                 ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                                 fbe_u8_t drive_slot);
fbe_status_t sas_virtual_phy_set_sas_conn_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct sas_conn_stat);
fbe_status_t sas_virtual_phy_get_sas_conn_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct *sas_conn_stat);
fbe_status_t sas_virtual_phy_get_cooling_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_cooling_id cooling_id, 
    ses_stat_elem_cooling_struct *cooling_stat);
fbe_status_t sas_virtual_phy_set_cooling_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_cooling_id cooling_id, 
    ses_stat_elem_cooling_struct cooling_stat);
fbe_status_t sas_virtual_phy_set_temp_sensor_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_temp_sensor_id temp_sensor_id, 
    ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t sas_virtual_phy_get_temp_sensor_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_temp_sensor_id temp_sensor_id, 
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat);
fbe_status_t sas_virtual_phy_set_overall_temp_sensor_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_temp_sensor_id temp_sensor_id, 
    ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t sas_virtual_phy_get_overall_temp_sensor_eses_status(
    terminator_virtual_phy_t * self, 
    terminator_eses_temp_sensor_id temp_sensor_id, 
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat);

fbe_status_t sas_virtual_phy_check_enclosure_type(fbe_sas_enclosure_type_t encl_type);
fbe_status_t sas_virtual_phy_max_temp_sensor_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_temp_sensor_elems);
fbe_status_t sas_virtual_phy_max_ps_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ps_elems);
fbe_status_t sas_virtual_phy_max_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_cooling_elems);
fbe_status_t sas_virtual_phy_max_ext_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ext_cooling_elems);
fbe_status_t sas_virtual_phy_max_bem_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_bem_cooling_elems);
fbe_status_t sas_virtual_phy_max_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_lccs);
fbe_status_t sas_virtual_phy_max_ee_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ee_lccs);
fbe_status_t sas_virtual_phy_max_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t sas_virtual_phy_max_conn_id_count(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conn_id_count);
fbe_status_t sas_virtual_phy_max_single_lane_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t sas_virtual_phy_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t sas_virtual_phy_max_phys(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_phys);
fbe_status_t sas_virtual_phy_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots);
fbe_status_t sas_virtual_phy_max_display_characters(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_diplay_characters);

fbe_status_t sas_virtual_phy_destroy(terminator_sas_virtual_phy_info_t *attributes);
void sas_virtual_phy_free_allocated_memory(terminator_sas_virtual_phy_info_t *attributes);

fbe_status_t sas_virtual_phy_get_enclosure_ptr(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_terminator_device_ptr_t *enclosure_ptr);
fbe_status_t sas_virtual_phy_get_enclosure_uid(fbe_terminator_device_ptr_t handle, fbe_u8_t ** uid);
fbe_status_t sas_virtual_phy_get_enclosure_type(fbe_terminator_device_ptr_t handle, fbe_sas_enclosure_type_t *encl_type);
fbe_status_t sas_virtual_phy_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);
fbe_bool_t sas_virtual_phy_is_eses_page_supported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);
fbe_status_t sas_virtual_phy_get_eses_page_info(terminator_virtual_phy_t * self, 
                                                terminator_vp_eses_page_info_t **vp_eses_page_info);
fbe_status_t sas_virtual_phy_set_ver_desc(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      ses_ver_desc_struct ver_desc);
fbe_status_t sas_virtual_phy_get_ver_desc(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      ses_ver_desc_struct *ver_desc);
fbe_status_t sas_virtual_phy_set_ver_num(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      CHAR *ver_num);
fbe_status_t sas_virtual_phy_get_ver_num(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      CHAR *ver_num);
fbe_status_t sas_virtual_phy_set_activate_time_interval(terminator_virtual_phy_t * self, fbe_u32_t time_interval);
fbe_status_t sas_virtual_phy_get_activate_time_interval(terminator_virtual_phy_t * self, fbe_u32_t *time_interval);
fbe_status_t sas_virtual_phy_set_reset_time_interval(terminator_virtual_phy_t * self, fbe_u32_t time_interval);
fbe_status_t sas_virtual_phy_get_reset_time_interval(terminator_virtual_phy_t * self, fbe_u32_t *time_interval);
fbe_status_t sas_virtual_phy_set_download_microcode_stat_desc(terminator_virtual_phy_t * self,
                                                              fbe_download_status_desc_t download_stat_desc);
fbe_status_t sas_virtual_phy_get_download_microcode_stat_desc(terminator_virtual_phy_t * self,
                                                              fbe_download_status_desc_t *download_stat_desc);
fbe_status_t 
sas_virtual_phy_get_vp_download_microcode_ctrl_page_info(
    terminator_virtual_phy_t * self,
    vp_download_microcode_ctrl_diag_page_info_t **vp_download_ctrl_page_info);
fbe_status_t sas_virtual_phy_get_firmware_download_status(terminator_virtual_phy_t * self, fbe_u8_t *status);
fbe_status_t sas_virtual_phy_set_firmware_download_status(terminator_virtual_phy_t * self, fbe_u8_t status);
fbe_status_t sas_virtual_phy_increment_gen_code(terminator_virtual_phy_t * self);
fbe_status_t sas_virtual_phy_get_unit_attention(terminator_virtual_phy_t * self,
                                                fbe_u8_t *unit_attention);
fbe_status_t sas_virtual_phy_set_unit_attention(terminator_virtual_phy_t * self,
                                                fbe_u8_t unit_attention);
fbe_status_t sas_virtual_phy_init(void);
fbe_status_t sas_virtual_phy_get_encl_eses_status(terminator_virtual_phy_t * self, 
                                                  ses_stat_elem_encl_struct *encl_stat);
fbe_status_t sas_virtual_phy_set_encl_eses_status(terminator_virtual_phy_t * self, 
                                                  ses_stat_elem_encl_struct encl_stat);
fbe_status_t sas_virtual_phy_get_chassis_encl_eses_status(terminator_virtual_phy_t * self, 
                                                          ses_stat_elem_encl_struct *chassis_encl_stat);

fbe_status_t sas_virtual_phy_set_chassis_encl_eses_status(terminator_virtual_phy_t * self, 
                                                          ses_stat_elem_encl_struct chassis_encl_stat);

fbe_status_t sas_virtual_phy_get_buf_info_by_buf_id(terminator_virtual_phy_t * self,
                                                    fbe_u8_t buf_id,
                                                    terminator_eses_buf_info_t **buf_info_ptr);

fbe_status_t sas_virtual_phy_set_buf(terminator_virtual_phy_t * self, 
                                    fbe_u8_t buf_id,
                                    fbe_u8_t *buf,
                                    fbe_u32_t len);

fbe_status_t  sas_virtual_phy_set_bufid2_buf_area_to_bufid1(terminator_virtual_phy_t * self,
                                                            fbe_u8_t buf_id1,
                                                            fbe_u8_t buf_id2);

void sas_virtual_phy_free_config_page_info_memory(vp_config_diag_page_info_t *vp_config_page_info);

fbe_status_t sas_virtual_phy_get_diplay_eses_status(terminator_virtual_phy_t * self,
                                                    terminator_eses_display_character_id display_character_id,
                                                    ses_stat_elem_display_struct *display_stat_elem);
fbe_status_t sas_virtual_phy_set_diplay_eses_status(terminator_virtual_phy_t * self,
                                                    terminator_eses_display_character_id display_character_id,
                                                    ses_stat_elem_display_struct display_stat_elem);

fbe_status_t sas_virtual_phy_push_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    fbe_u8_t  comp_type,
                                                                                    fbe_u32_t slot_num,
                                                                                    CHAR   *new_rev_number);
fbe_status_t sas_virtual_phy_pop_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    fbe_u32_t slot_num,
                                                                                    ses_comp_type_enum  *comp_type_p,
                                                                                    CHAR   *new_rev_number);
fbe_status_t sas_virtual_phy_update_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    ses_comp_type_enum  comp_type,
                                                                                    fbe_u32_t slot_num,
                                                                                    CHAR   *new_rev_number);


#endif /* TERMINATOR_VIRTUAL_PHY_H */
