#ifndef FBE_ENCRYPTION_H
#define FBE_ENCRYPTION_H

/***************************************************************************/
/** @file fbe_encryption.h
***************************************************************************
*
* @brief
*  This file contains definitions of functions that are related to encryption
* 
***************************************************************************/


#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_raid_ts.h"

#define FBE_DEK_SIZE 64

#define FBE_ENCRYPTION_KEY_SIZE 72 /* Wrapped DEK */
#define FBE_ENCRYPTION_WRAPPED_KEK_SIZE 48 
#define FBE_ENCRYPTION_KEK_KEK_SIZE 40

#define FBE_ENCRYPTION_KEYS_BATCH_PROCESSING_SIZE  10

#define FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH 128

typedef fbe_u64_t fbe_encryption_key_mask_t;

enum fbe_system_encryption_masks_e{
    FBE_ENCRYPTION_KEY_MASK_NONE = 0,
    FBE_ENCRYPTION_KEY_MASK_VALID  = 0x01,
    FBE_ENCRYPTION_KEY_MASK_UPDATE = 0x02,
};

static __forceinline void fbe_encryption_key_mask_get(fbe_encryption_key_mask_t *mask_p,
                                               fbe_u32_t mask_index,
                                               fbe_encryption_key_mask_t *output_p)
{
    if (mask_index == 0) {
        *output_p = *mask_p & FBE_U32_MAX;
    } else {
        *output_p = *mask_p >> 32;
    }
}
static __forceinline fbe_bool_t fbe_encryption_key_mask_both_valid(fbe_encryption_key_mask_t *mask_p)
{
    fbe_encryption_key_mask_t mask0, mask1;

    fbe_encryption_key_mask_get(mask_p, 0, &mask0);
    fbe_encryption_key_mask_get(mask_p, 1, &mask1);

    if ((mask0 & FBE_ENCRYPTION_KEY_MASK_VALID) &&
        (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID)){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static __forceinline fbe_bool_t fbe_encryption_key_mask_has_valid_key(fbe_encryption_key_mask_t *mask_p)
{
    fbe_encryption_key_mask_t mask0, mask1;

    fbe_encryption_key_mask_get(mask_p, 0, &mask0);
    fbe_encryption_key_mask_get(mask_p, 1, &mask1);

    if ((mask0 & FBE_ENCRYPTION_KEY_MASK_VALID) ||
        (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID)){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static __forceinline void fbe_encryption_key_mask_set(fbe_encryption_key_mask_t *mask_p,
                                               fbe_u32_t mask_index,
                                               fbe_encryption_key_mask_t new_mask)
{
    if (mask_index == 0) {
        *mask_p |= new_mask & FBE_U32_MAX;
    } else {
        *mask_p |= new_mask << 32;
    }
}

typedef enum fbe_system_encryption_mode_e{
    FBE_SYSTEM_ENCRYPTION_MODE_INVALID = 0,
    FBE_SYSTEM_ENCRYPTION_MODE_UNENCRYPTED = 16,
    FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED = 32,
}fbe_system_encryption_mode_t;

typedef enum fbe_system_encryption_state_e{
    FBE_SYSTEM_ENCRYPTION_STATE_INVALID = 0,
    FBE_SYSTEM_ENCRYPTION_STATE_UNENCRYPTED,
    FBE_SYSTEM_ENCRYPTION_STATE_IN_PROGRESS = 16,
    FBE_SYSTEM_ENCRYPTION_STATE_ENCRYPTED = 32,
    FBE_SYSTEM_ENCRYPTION_STATE_SCRUBBING = 64,
    FBE_SYSTEM_ENCRYPTION_STATE_NOLICENSE = 254,
    FBE_SYSTEM_ENCRYPTION_STATE_UNSUPPORTED = 255,
}fbe_system_encryption_state_t;

typedef enum fbe_system_scrubbing_state_e{
    FBE_SYSTEM_SCRUBBING_STATE_INVALID = 0,
    FBE_SYSTEM_SCRUBBING_STATE_IN_PROGRESS = 16,
    FBE_SYSTEM_SCRUBBING_STATE_ENCRYPTED = 32,
}fbe_system_scrubbing_state_t;

typedef struct fbe_encryption_key_info_s {
    fbe_encryption_key_mask_t key_mask;
    fbe_u8_t key1[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u8_t key2[FBE_ENCRYPTION_KEY_SIZE];
}fbe_encryption_key_info_t;

typedef struct fbe_encryption_setup_encryption_keys_s{
    fbe_object_id_t object_id;
    fbe_u32_t num_of_keys;
    fbe_config_generation_t generation_number;
    fbe_encryption_key_info_t encryption_keys[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1]; /* +1 for PACO */
}fbe_encryption_setup_encryption_keys_t;

typedef struct fbe_encryption_key_table_s {
    fbe_u32_t                               num_of_entries;
    fbe_encryption_setup_encryption_keys_t  key_table_entry[FBE_ENCRYPTION_KEYS_BATCH_PROCESSING_SIZE];
}fbe_encryption_key_table_t;

#pragma pack(1)
typedef struct fbe_encryption_hardware_ssv_s {
    fbe_u8_t hw_ssv_midplane_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
    fbe_u8_t hw_ssv_spa_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
    fbe_u8_t hw_ssv_spb_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
    fbe_u8_t hw_ssv_mgmta_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
    fbe_u8_t hw_ssv_mgmtb_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
    fbe_u8_t hw_ssv_midplane_product_sn[FBE_ENCRYPTION_HARDWARE_SSV_ENTRY_LENGTH];
}fbe_encryption_hardware_ssv_t;
#pragma pack()

static __forceinline fbe_encryption_key_info_t * fbe_encryption_get_key_info_handle(fbe_encryption_setup_encryption_keys_t *key_handle, fbe_u32_t index)
{
    return(&key_handle->encryption_keys[index]);
}

typedef enum fbe_encryption_backup_state_e{
    FBE_ENCRYPTION_BACKUP_INVALID = 0,
    FBE_ENCRYPTION_BACKUP_REQUIERED = 16,
    FBE_ENCRYPTION_BACKUP_IN_PROGRESS = 32,
    FBE_ENCRYPTION_BACKUP_COMPLETED = 64,
    FBE_ENCRYPTION_LOCKBOX_CORRUPTED = 128,
}fbe_encryption_backup_state_t;

#endif /* FBE_DATABASE_H */

