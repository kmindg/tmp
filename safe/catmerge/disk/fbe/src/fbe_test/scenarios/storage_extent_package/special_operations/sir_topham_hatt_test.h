/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sir_topham_hatt_test.h
 ***************************************************************************
 *
 * @brief
 *  This file contains a common utility function definitions for drive firmware.
 *  download tests.
 *
 * @version
 *   3/16/2011 - Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/


/*!*******************************************************************
 * @def CLIFFORD_RAID_GROUP_CHUNK_SIZE
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define SIR_TOPHAM_HATT_MAX_SELECTED_DRIVES 10

/*!*******************************************************************
 * @def sir_topham_hatt_select_drives_s
 *********************************************************************
 * @brief Structure for selecting drives in fw.
 *
 *********************************************************************/
typedef struct sir_topham_hatt_select_drives_s
{
    fbe_u32_t num_of_drives;
    fbe_drive_selected_element_t selected_drives[SIR_TOPHAM_HATT_MAX_SELECTED_DRIVES];
} sir_topham_hatt_select_drives_t;


/*************************
 *   FUNCTION PROTOTYPES
 *************************/
void sir_topham_hatt_create_fw(fbe_sas_drive_type_t drive_type, fbe_drive_configuration_control_fw_info_t *fw_info);
void sir_topham_hatt_destroy_fw(fbe_drive_configuration_control_fw_info_t *fw_info_p);
void sir_topham_hatt_fw_set_tla(fbe_drive_configuration_control_fw_info_t *fw_info, const fbe_u8_t *tla, fbe_u32_t tla_size);
void sir_topham_hatt_write_fdf(fbe_drive_configuration_control_fw_info_t *fw_info);
void sir_topham_hatt_select_drives(fbe_fdf_header_block_t *header_ptr, sir_topham_hatt_select_drives_t *selected_drives);
void sir_topham_hatt_check_drive_download_status(fbe_drive_selected_element_t *drive,
												 fbe_drive_configuration_download_drive_state_t state,
												 fbe_drive_configuration_download_drive_fail_t fail_reason);
fbe_status_t sir_topham_hatt_wait_download_process_state(fbe_drive_configuration_download_get_process_info_t * dl_status,
                                                         fbe_drive_configuration_download_process_state_t dl_state,
                                                         fbe_u32_t seconds);
fbe_status_t sir_topham_hatt_check_fw_download_event_log(fbe_drive_configuration_control_fw_info_t *fw_info, 
                                                         fbe_u32_t msg_id, fbe_drive_configuration_download_process_fail_t fail_code,
                                                         fbe_bool_t *msg_present);
