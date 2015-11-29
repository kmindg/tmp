#ifndef FBE_NOTIFICATION_LIB_H
#define FBE_NOTIFICATION_LIB_H

#include "fbe/fbe_notification_interface.h"

#if defined(__cplusplus)
#define FBE_NOTIFICATION_CALL __stdcall
#else
#define FBE_NOTIFICATION_CALL
#endif

#if defined(__cplusplus) || defined(c_plusplus)
# define FBE_NOTIFICATION_CPP_EXPORT_START   extern "C" {
# define FBE_NOTIFICATION_CPP_EXPORT_END }
#else
# define FBE_NOTIFICATION_CPP_EXPORT_START
# define FBE_NOTIFICATION_CPP_EXPORT_END
#endif

FBE_NOTIFICATION_CPP_EXPORT_START
fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_fbe_package_to_package_bitmap(fbe_package_id_t fbe_package, fbe_package_notification_id_mask_t *package_mask);
fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_package_bitmap_to_fbe_package(fbe_package_notification_id_mask_t package_mask, fbe_package_id_t * fbe_package);
fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_notification_type_to_state(fbe_notification_type_t notification_type, fbe_lifecycle_state_t *state);
fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_state_to_notification_type (fbe_lifecycle_state_t state, fbe_notification_type_t *notification_type);
FBE_NOTIFICATION_CPP_EXPORT_END


#endif /* FBE_NOTIFICATION_LIB_H */


