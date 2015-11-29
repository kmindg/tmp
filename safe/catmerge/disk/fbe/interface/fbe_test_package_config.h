#ifndef FBE_TEST_PACKAGE_CONFIG_H
#define FBE_TEST_PACKAGE_CONFIG_H

#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_neit.h"

extern fbe_service_control_entry_t sep_control_entry;
extern fbe_io_entry_function_t sep_io_entry;

extern fbe_service_control_entry_t physical_package_control_entry;
extern fbe_io_entry_function_t physical_package_io_entry;

extern fbe_service_control_entry_t neit_package_control_entry;

extern fbe_service_control_entry_t esp_control_entry;

extern fbe_service_control_entry_t kms_control_entry;

extern fbe_bool_t is_ica_stamp_generated;

fbe_status_t fbe_test_load_physical_package(void);
fbe_status_t fbe_test_unload_physical_package(void);

fbe_status_t fbe_test_load_sep_package(fbe_packet_t *packet_p);
fbe_status_t fbe_test_load_sep(fbe_sep_package_load_params_t *params_p);
fbe_status_t fbe_test_unload_sep(void);

fbe_status_t fbe_test_load_esp(void);
fbe_status_t fbe_test_unload_esp(void);
             
fbe_status_t fbe_test_load_neit_package(fbe_packet_t *packet_p);
fbe_status_t fbe_test_load_neit(fbe_neit_package_load_params_t *params_p);
fbe_status_t fbe_test_unload_neit_package(void);
fbe_status_t fbe_test_disable_neit_package(void);

fbe_status_t fbe_test_load_kms_package(fbe_packet_t *packet_p);
//fbe_status_t fbe_test_load_kms(fbe_kms_package_load_params_t *params_p);
fbe_status_t fbe_test_unload_kms(void);

#endif /* FBE_TEST_PACKAGE_CONFIG_H */
