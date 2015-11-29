#ifndef KMS_DLL_H
#define KMS_DLL_H

#include "fbe/fbe_service.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_kms_api.h"

#define KMS_FILE_NAME_SIZE 128

typedef enum
{
    FBE_KMS_CONTROL_CODE_INVALID = 0,

    FBE_KMS_CONTROL_CODE_ENABLE_ENCRYPTION,
    FBE_KMS_CONTROL_CODE_START_REKEY,
    FBE_KMS_CONTROL_CODE_GET_RG_DEKS,
    FBE_KMS_CONTROL_CODE_START_BACKUP,
    FBE_KMS_CONTROL_CODE_COMPLETE_BACKUP,
    FBE_KMS_CONTROL_CODE_RESTORE,
    FBE_KMS_CONTROL_CODE_SET_RG_DEKS,
    FBE_KMS_CONTROL_CODE_SET_HOOK,
    FBE_KMS_CONTROL_CODE_GET_HOOK,
    FBE_KMS_CONTROL_CODE_CLEAR_HOOK,
    FBE_KMS_CONTROL_CODE_PUSH_RG_DEKS,
    FBE_KMS_CONTROL_CODE_GET_ENABLE_STATUS,

    /* Insert new control codes here. */

    FBE_KMS_CONTROL_CODE_LAST
}
fbe_kms_control_code_t;

typedef struct fbe_kms_record_drive_s{
	fbe_encryption_key_mask_t key_mask;
	fbe_u8_t serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1]; /* Drive Serial Number */
}fbe_kms_record_drive_t;

typedef struct fbe_kms_control_get_keys_s{
	fbe_object_id_t	object_id;
	fbe_u32_t   control_number;
	fbe_kms_record_drive_t drive[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
	fbe_u8_t key1[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1][FBE_DEK_SIZE];
	fbe_u8_t key2[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1][FBE_DEK_SIZE];
}fbe_kms_control_get_keys_t;

typedef struct fbe_kms_control_set_keys_s{
	fbe_object_id_t	object_id;
	fbe_u32_t   control_number;
    fbe_u32_t   width;
	fbe_kms_record_drive_t drive[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];

	fbe_u8_t key1[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1][FBE_DEK_SIZE];
	fbe_u8_t key2[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1][FBE_DEK_SIZE];

    /*! TRUE to set key and FALSE otherwise.
     */
    fbe_bool_t b_set_key[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
}fbe_kms_control_set_keys_t;

typedef struct fbe_kms_control_get_hook_s{
    fbe_kms_hook_t hook;
}fbe_kms_control_get_hook_t;

typedef struct fbe_kms_control_clear_hook_s{
    fbe_kms_hook_t hook;
}fbe_kms_control_clear_hook_t;

/*!*******************************************************************
 * @struct fbe_kms_package_load_params_t
 *********************************************************************
 * @brief The load parameters for our neit package.
 *
 *********************************************************************/
typedef struct fbe_kms_package_load_params_s
{
    fbe_u32_t flags;
    fbe_kms_hook_t hooks[KMS_HOOK_COUNT];
} fbe_kms_package_load_params_t;

typedef fbe_status_t (CALLBACK* kms_dll_init_function_t)(fbe_kms_package_load_params_t * param);
CSX_MOD_EXPORT fbe_status_t __cdecl kms_dll_init(fbe_kms_package_load_params_t * param);

typedef fbe_status_t (CALLBACK* kms_dll_destroy_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl kms_dll_destroy(void);

typedef fbe_status_t (CALLBACK* kms_dll_get_control_entry_function_t)(fbe_service_control_entry_t * service_control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  kms_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry);

typedef fbe_status_t (CALLBACK* kms_dll_set_sep_io_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  kms_dll_set_sep_io_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* kms_dll_set_sep_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  kms_dll_set_sep_control_entry(fbe_service_control_entry_t control_entry);

#endif /* KMS_DLL_H */
