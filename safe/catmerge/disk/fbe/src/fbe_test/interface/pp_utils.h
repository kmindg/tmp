#ifndef PP_UTILS_H
#define PP_UTILS_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe_private_space_layout.h"

/*!*******************************************************************
 * @def TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/

fbe_sas_address_t fbe_test_pp_util_get_unique_sas_address(void);

fbe_status_t fbe_test_pp_util_insert_sas_drive(fbe_u32_t backend_number,
                                               fbe_u32_t encl_number,
                                               fbe_u32_t slot_number,
                                               fbe_block_size_t block_size,
                                               fbe_lba_t capacity,
                                               fbe_api_terminator_device_handle_t  *drive_handle_p);
fbe_status_t fbe_test_pp_util_insert_unique_sas_drive(fbe_u32_t backend_number,
                                                      fbe_u32_t encl_number,
                                                      fbe_u32_t slot_number,
                                                      fbe_block_size_t block_size,
                                                      fbe_lba_t capacity,
                                                      fbe_api_terminator_device_handle_t  *drive_handle_p);

fbe_status_t fbe_test_pp_util_get_unique_sas_drive_info(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_block_size_t block_size,
                                                   fbe_lba_t *capacity,
                                                   fbe_api_terminator_device_handle_t  *drive_handle_p,
                                                   fbe_terminator_sas_drive_info_t *sas_drive_p);

fbe_status_t fbe_test_pp_util_insert_unique_sas_drive_for_dualsp(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_block_size_t block_size,
                                                   fbe_lba_t capacity,
                                                   fbe_api_terminator_device_handle_t  *drive_handle_p,
                                                   fbe_terminator_sas_drive_info_t sas_drive);

fbe_status_t fbe_test_pp_util_insert_sas_drive_extend(fbe_u32_t backend_number,
                                               fbe_u32_t encl_number,
                                               fbe_u32_t slot_number,
                                               fbe_block_size_t block_size,
                                               fbe_lba_t capacity,
                                               fbe_sas_address_t sas_address,
                                               fbe_sas_drive_type_t drive_type,
                                               fbe_api_terminator_device_handle_t  *drive_handle_p);
fbe_status_t fbe_test_pp_util_insert_sata_drive(fbe_u32_t backend_number,
                                                fbe_u32_t encl_number,
                                                fbe_u32_t slot_number,
                                                fbe_block_size_t block_size,
                                                fbe_lba_t capacity,
                                                fbe_api_terminator_device_handle_t  *drive_handle_p);
fbe_status_t fbe_test_pp_util_insert_sas_drive_with_info(fbe_terminator_sas_drive_info_t *drive_info_p,
                                                         fbe_api_terminator_device_handle_t  *drive_handle_p);
fbe_status_t fbe_test_pp_util_insert_sas_enclosure(fbe_u32_t backend_number,
                                                     fbe_u32_t encl_number,
                                                     fbe_u8_t *uid,
                                                     fbe_sas_address_t sas_address,
                                                     fbe_sas_enclosure_type_t encl_type,
                                                     fbe_api_terminator_device_handle_t  port_handle,
                                                     fbe_api_terminator_device_handle_t  *enclosure_handle_p);
fbe_status_t fbe_test_pp_util_insert_viper_enclosure(fbe_u32_t backend_number,
                                     fbe_u32_t encl_number,
                                     fbe_api_terminator_device_handle_t  port_handle,
                                     fbe_api_terminator_device_handle_t  *enclosure_handle_p);
fbe_status_t fbe_test_pp_util_insert_dpe_enclosure(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_sas_enclosure_type_t encl_type,
                                                   fbe_api_terminator_device_handle_t  port_handle,
                                                   fbe_api_terminator_device_handle_t  *enclosure_handle_p);
fbe_status_t fbe_test_pp_util_insert_sas_port(fbe_u32_t io_port_number,
                                  fbe_u32_t portal_number,
                                  fbe_u32_t backend_number,
                                  fbe_sas_address_t sas_address,
                                  fbe_port_type_t port_type,
                                  fbe_api_terminator_device_handle_t  *port_handle_p);
fbe_status_t fbe_test_pp_util_insert_sas_pmc_port(fbe_u32_t io_port_number,
                                  fbe_u32_t portal_number,
                                  fbe_u32_t backend_number,
                                  fbe_api_terminator_device_handle_t  *port_handle_p);
fbe_status_t fbe_test_pp_util_insert_armada_board(void);
fbe_status_t fbe_test_pp_util_insert_board_of_type(SPID_HW_TYPE platform_type);
fbe_status_t fbe_test_pp_util_insert_board(void);
fbe_status_t fbe_test_pp_util_remove_sas_drive(fbe_u32_t port_number, 
                                               fbe_u32_t enclosure_number, 
                                               fbe_u32_t slot_number);
fbe_status_t fbe_test_pp_util_remove_sata_drive(fbe_u32_t port_number, 
                                                fbe_u32_t enclosure_number, 
                                                fbe_u32_t slot_number);
fbe_status_t fbe_test_pp_util_verify_pdo_state(fbe_u32_t port_number,
                                               fbe_u32_t enclosure_number,
                                               fbe_u32_t slot_number,
                                               fbe_lifecycle_state_t expected_state,
                                               fbe_u32_t timeout_ms);
fbe_status_t fbe_test_pp_util_pull_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t *drive_handle_p);
fbe_status_t fbe_test_pp_util_reinsert_drive(fbe_u32_t backend_number,
                                             fbe_u32_t encl_number,
                                             fbe_u32_t slot_number,
                                             fbe_api_terminator_device_handle_t drive_handle);

fbe_u32_t fbe_test_pp_util_get_extended_testing_level(void);

fbe_status_t fbe_test_insert_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                           fbe_terminator_sas_encl_info_t *encl_info, 
                                           fbe_terminator_api_device_handle_t *encl_handle,
                                           fbe_u32_t *num_handles);

fbe_status_t fbe_test_insert_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_terminator_api_device_handle_t *drive_handle);

fbe_status_t fbe_test_pp_util_wait_for_term_pp_sync(fbe_u32_t timeout_ms);
fbe_status_t fbe_test_pp_util_verify_term_pp_sync(void);

fbe_status_t fbe_test_pp_util_wait_for_valid_port_object_handle (fbe_u32_t port, 
        fbe_lifecycle_state_t expected_lifecycle_state, fbe_u32_t timeout_ms, fbe_u32_t *object_handle_p);
fbe_status_t fbe_test_pp_util_insert_sas_flash_drive(fbe_u32_t backend_number,
                                                     fbe_u32_t encl_number,
                                                     fbe_u32_t slot_number,
                                                     fbe_block_size_t block_size,
                                                     fbe_lba_t capacity,
                                                     fbe_api_terminator_device_handle_t  *drive_handle_p);
void fbe_test_pp_util_set_terminator_drive_debug_flags(fbe_terminator_drive_debug_flags_t in_terminator_drive_debug_flags);

void fbe_test_pp_util_set_terminator_drive_debug_flags(fbe_terminator_drive_debug_flags_t in_terminator_drive_debug_flags);

fbe_status_t fbe_test_pp_wait_for_ps_status(fbe_object_id_t object_id,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_bool_t expectedInserted,
                                             fbe_mgmt_status_t expectedFaulted,
                                             fbe_u32_t timeoutMs);
fbe_status_t fbe_test_pp_wait_for_cooling_status(fbe_object_id_t object_id,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_bool_t expectedInserted,
                                             fbe_mgmt_status_t expectedFaulted,
                                             fbe_u32_t timeoutMs);

/*!*******************************************************************
 * @struct fbe_test_pp_util_lifecycle_state_ns_context_t
 *********************************************************************
 * @brief This is the notification context for lifecycle_state
 *
 *********************************************************************/
typedef struct fbe_test_pp_util_lifecycle_state_ns_context_s
{
    fbe_semaphore_t sem;
    fbe_spinlock_t lock;
    fbe_lifecycle_state_t expected_lifecycle_state;
    fbe_bool_t state_match;
    fbe_u32_t timeout_in_ms;
    fbe_topology_object_type_t object_type;
    /* add to here, if more fields to match */
}fbe_test_pp_util_lifecycle_state_ns_context_t;

/* the following is the utility functions for Job Action State Notification */
void fbe_test_pp_util_register_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context);
void fbe_test_pp_util_unregister_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context);
void fbe_test_pp_util_wait_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context);


/*!*******************************************************************
 * @def FBE_TEST_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
#define FBE_TEST_DRIVES_PER_ENCL 15

void fbe_test_pp_util_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                             fbe_u32_t num_4160_drives,
                                                             fbe_block_count_t drive_capacity);

void fbe_test_pp_util_create_config_vary_capacity(fbe_u32_t num_520_drives,
                                                  fbe_u32_t num_4160_drives,
                                                  fbe_block_count_t drive_capacity,
                                                  fbe_block_count_t extra_blocks);

/* common destroy function for physical package test suite*/
void fbe_test_pp_common_destroy(void);
void fbe_test_neit_pp_common_destroy(void);
fbe_bool_t fbe_test_pp_util_get_cmd_option(char *option_name_p);
void fbe_test_pp_util_enable_trace_limits(void);
fbe_status_t fbe_test_wait_for_term_pp_sync(fbe_u32_t timeout_ms);
fbe_status_t fbe_test_calc_object_count(fbe_terminator_device_count_t *dev_counts);
fbe_status_t fbe_test_verify_term_pp_sync(void);
void fbe_test_pp_utils_set_default_520_sas_drive(fbe_sas_drive_type_t drive_type);
void fbe_test_pp_utils_set_default_4160_sas_drive(fbe_sas_drive_type_t drive_type);
#endif
