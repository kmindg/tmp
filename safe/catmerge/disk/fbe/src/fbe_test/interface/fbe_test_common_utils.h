#ifndef FBE_TEST_COMMON_UTILS_H
#define FBE_TEST_COMMON_UTILS_H
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_neit.h"
#include "VolumeAttributes.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "kms_dll.h"

enum {
    FBE_TEST_RDGEN_MEMORY_SIZE_MB = 80
};
typedef struct fbe_test_package_list_s {
    fbe_u32_t           			number_of_packages;
    fbe_package_id_t    			package_list[FBE_PACKAGE_ID_LAST];
    fbe_status_t                    package_destroy_status[FBE_PACKAGE_ID_LAST];
    fbe_api_trace_error_counters_t  package_error[FBE_PACKAGE_ID_LAST];

    fbe_status_t                    package_disable_status[FBE_PACKAGE_ID_LAST];
}fbe_test_package_list_t;

fbe_status_t fbe_test_common_util_package_destroy(fbe_test_package_list_t *to_be_unloaded);
fbe_status_t fbe_test_common_util_verify_package_trace_error(fbe_test_package_list_t *to_be_unloaded);
fbe_status_t fbe_test_common_util_verify_package_destroy_status(fbe_test_package_list_t *to_be_unloaded);

/*!*******************************************************************
 * @struct fbe_test_common_util_lifecycle_state_ns_context_t
 *********************************************************************
 * @brief This is the notification context for lifecycle_state
 *
 *********************************************************************/
typedef struct fbe_test_common_util_lifecycle_state_ns_context_s
{
    fbe_semaphore_t sem;
    fbe_topology_object_type_t object_type;
    fbe_object_id_t object_id;
    fbe_notification_registration_id_t user_registration_id;
    fbe_notification_type_t expected_lifecycle_state;
    fbe_notification_type_t actual_lifecycle_state;

}fbe_test_common_util_lifecycle_state_ns_context_t;

/* the following is the utility functions for Job Action State Notification */
void fbe_test_common_util_register_lifecycle_state_notification(fbe_test_common_util_lifecycle_state_ns_context_t *ns_context,
                                                                fbe_package_notification_id_mask_t package_mask,
                                                                fbe_topology_object_type_t object_type,
                                                                fbe_object_id_t object_id,
                                                                fbe_notification_type_t expected_state_mask);
void fbe_test_common_util_unregister_lifecycle_state_notification (fbe_test_common_util_lifecycle_state_ns_context_t *ns_context);
void fbe_test_common_util_wait_lifecycle_state_notification(fbe_test_common_util_lifecycle_state_ns_context_t *ns_context,
                                                            fbe_time_t timeout_in_ms);


/* Object state etc functions */
fbe_status_t fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(fbe_object_id_t object_id, 
                                                                        fbe_transport_connection_target_t target_sp_of_object,
                                                                        fbe_lifecycle_state_t expected_lifecycle_state, 
                                                                        fbe_u32_t timeout_ms, 
                                                                        fbe_package_id_t package_id);
fbe_status_t fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_bool_t b_wait_for_both_sps,
                                                              fbe_object_id_t object_id, 
                                                              fbe_lifecycle_state_t expected_lifecycle_state, 
                                                              fbe_u32_t timeout_ms, 
                                                              fbe_package_id_t package_id);	

fbe_status_t fbe_test_sep_utils_wait_for_attribute(fbe_volume_attributes_flags attr, 
                                      fbe_u32_t timeout_in_sec, 
                                      fbe_object_id_t lun_object_id,
									  fbe_bool_t attribute_existence);															  

/* Generic notification test interface. */
fbe_status_t fbe_test_common_setup_notifications(fbe_bool_t b_is_dualsp_test);
fbe_u32_t fbe_test_common_count_bits(fbe_u64_t value_to_check);
fbe_status_t fbe_test_common_set_notification_to_wait_for(fbe_object_id_t object_id,
                                                         fbe_topology_object_type_t object_type,
                                                         fbe_notification_type_t notification_type,
                                                         fbe_status_t expected_job_status,
                                                         fbe_job_service_error_type_t expected_job_error_code);
fbe_status_t fbe_test_common_wait_for_notification(const char *function_p, fbe_u32_t line,
                                                 fbe_u32_t timeout_msecs,
                                                 fbe_notification_info_t *notification_info_p);
fbe_status_t fbe_test_common_cleanup_notifications(fbe_bool_t b_is_dualsp_test);
fbe_status_t fbe_test_common_notify_mark_peer_dead(fbe_bool_t b_is_dualsp_test);
fbe_status_t fbe_test_common_notify_mark_peer_alive(fbe_bool_t b_is_dualsp_test);

/* fbe_hw_test_main.c */
void fbe_test_util_set_hardware_mode(void);
fbe_bool_t fbe_test_util_is_simulation(void);
fbe_status_t fbe_test_common_util_test_setup_init(void);
void fbe_test_set_default_extended_test_level(fbe_u32_t level);
fbe_u32_t fbe_test_get_default_extended_test_level(void);
void fbe_test_init_random_seed(void);
void fbe_sep_util_destroy_neit_sep_phy_expect_errs(void);

/* reboot utilities */
fbe_status_t fbe_test_common_reboot_save_sep_params(fbe_sep_package_load_params_t *sep_params_p);
fbe_status_t fbe_test_common_reboot_restore_sep_params(fbe_sep_package_load_params_t *sep_params_p);
fbe_status_t fbe_test_common_reboot_this_sp(fbe_sep_package_load_params_t *sep_params_p, 
                                            fbe_neit_package_load_params_t *neit_params_p);
fbe_status_t fbe_test_common_reboot_sp(fbe_sim_transport_connection_target_t sp_to_reboot,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p);
fbe_status_t fbe_test_common_boot_sp(fbe_sim_transport_connection_target_t sp_to_reboot,
                                     fbe_sep_package_load_params_t *sep_params_p, 
                                     fbe_neit_package_load_params_t *neit_params_p);
fbe_status_t fbe_test_common_reboot_both_sps(void);

fbe_status_t fbe_test_common_panic_both_sps(void);

fbe_status_t fbe_test_common_reboot_sp_neit_sep_kms(fbe_sim_transport_connection_target_t sp_to_reboot,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p,
									   fbe_kms_package_load_params_t *kms_params_p);
const char *fbe_test_common_get_sp_name(fbe_sim_transport_connection_target_t sp);
fbe_status_t fbe_test_common_reboot_both_sps_neit_sep_kms(fbe_sim_transport_connection_target_t active_sp,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p,
                                       fbe_kms_package_load_params_t *kms_params_p);

#endif // FBE_TEST_COMMON_UTILS_H
