#ifndef FBE_SERVICE_MANAGER_INTERFACE_H
#define FBE_SERVICE_MANAGER_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"

typedef enum fbe_service_manager_control_code_e {
	FBE_SERVICE_MANAGER_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_SERVICE_MANAGER),

	FBE_SERVICE_MANAGER_CONTROL_CODE_SET_PHYSICAL_PACKAGE_CONTROL_ENTRY,

	FBE_SERVICE_MANAGER_CONTROL_CODE_LAST
} fbe_service_manager_control_code_t;

/* FBE_SERVICE_MANAGER_CONTROL_CODE_SET_PHYSICAL_PACKAGE_CONTROL_ENTRY */
typedef struct fbe_service_manager_control_set_physical_package_control_entry_s{
	fbe_service_control_entry_t control_entry;
}fbe_service_manager_control_set_physical_package_control_entry_t;

fbe_status_t fbe_service_manager_set_physical_package_control_entry(fbe_service_control_entry_t control_entry);
fbe_status_t fbe_service_manager_set_sep_control_entry(fbe_service_control_entry_t control_entry);
fbe_status_t fbe_service_manager_set_esp_package_control_entry(fbe_service_control_entry_t control_entry);
fbe_status_t fbe_service_manager_set_neit_control_entry(fbe_service_control_entry_t control_entry);
fbe_status_t fbe_service_manager_set_kms_control_entry(fbe_service_control_entry_t control_entry);

/*FBE3, sharel:temporary here just so we can build the shim, to be removed once we flare is removed*/
fbe_status_t fbe_service_manager_send_control_packet(fbe_packet_t * packet);

#endif /* FBE_SERVICE_MANAGER_INTERFACE_H */