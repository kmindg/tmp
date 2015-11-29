#ifndef FBE_PACKAGE_H
#define FBE_PACKAGE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"

typedef enum fbe_package_id_e{
	FBE_PACKAGE_ID_INVALID,
	FBE_PACKAGE_ID_PHYSICAL,
    FBE_PACKAGE_ID_NEIT,
	FBE_PACKAGE_ID_SEP_0, /* Storage Extent Package */
	FBE_PACKAGE_ID_ESP,
	FBE_PACKAGE_ID_KMS,
	/*add new package here and update the mask below as well*/
	FBE_PACKAGE_ID_LAST
}fbe_package_id_t;

enum fbe_package_notification_id_mask_e{
	FBE_PACKAGE_NOTIFICATION_ID_INVALID =		0x0,
	FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL =		0x1,
    FBE_PACKAGE_NOTIFICATION_ID_NEIT = 			0x2,
	FBE_PACKAGE_NOTIFICATION_ID_SEP_0 = 		0x4, /* Storage Extent Package */
	FBE_PACKAGE_NOTIFICATION_ID_ESP = 			0x8,
	FBE_PACKAGE_NOTIFICATION_ID_KMS = 			0x10,
	FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES = 	0x1F,
	FBE_PACKAGE_NOTIFICATION_ID_LAST
};

typedef fbe_u64_t fbe_package_notification_id_mask_t;/* to use fbe_package_notification_id_mask_e*/

typedef struct fbe_package_get_io_entry_s{
	fbe_service_io_entry_t io_entry; /* OUTPUT */
}fbe_package_get_io_entry_t;

typedef struct fbe_package_get_control_entry_s{
	fbe_service_control_entry_t control_entry; /* OUTPUT */
}fbe_package_get_control_entry_t;

typedef struct fbe_package_set_control_entry_s{
	fbe_service_control_entry_t control_entry; /* INPUT */
}fbe_package_set_control_entry_t;

#if defined(__cplusplus)|| defined(c_plusplus)
extern "C"
{
    typedef fbe_status_t ( __stdcall * fbe_io_entry_function_t)(struct fbe_packet_s * packet);
    fbe_status_t __stdcall fbe_get_package_id(fbe_package_id_t * package_id);
}
#else
typedef fbe_status_t ( * fbe_io_entry_function_t)(struct fbe_packet_s * packet);
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id);
#endif



#endif /* FBE_PACKAGE_H */