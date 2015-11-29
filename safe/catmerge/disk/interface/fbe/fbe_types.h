#ifndef FBE_TYPES_H
#define FBE_TYPES_H

#ifndef UEFI_ENV
#include "csx_ext.h"
#endif

typedef enum fbe_status_e{
    FBE_STATUS_INVALID = 0,
    FBE_STATUS_OK,
    FBE_STATUS_MORE_PROCESSING_REQUIRED,
    FBE_STATUS_PENDING,
    FBE_STATUS_CANCEL_PENDING,
    FBE_STATUS_CANCELED,
    FBE_STATUS_TIMEOUT,
    FBE_STATUS_INSUFFICIENT_RESOURCES,
    FBE_STATUS_NO_DEVICE,
    FBE_STATUS_NO_OBJECT,

    FBE_STATUS_SERVICE_MODE, /* used for database enter service mode */

    FBE_STATUS_CONTINUE, /* May be used for edge tap functianality ONLY */

    FBE_STATUS_BUSY,   /* The object is in activate or activate pending state */
    FBE_STATUS_SLUMBER,/* The object is in hibernate or hibernate pending state */
    FBE_STATUS_FAILED, /* The object is in fail or fail pending state */
    FBE_STATUS_DEAD,   /* The object is in destroy or destroy pending state */

    FBE_STATUS_NOT_INITIALIZED,
    FBE_STATUS_TRAVERSE, /* Base object returns this status when packet needs to be traversed */

    FBE_STATUS_EDGE_NOT_ENABLED, /* This status returned when packet sent to not enabled edge */
    FBE_STATUS_COMPONENT_NOT_FOUND,
    FBE_STATUS_ATTRIBUTE_NOT_FOUND,
    FBE_STATUS_HIBERNATION_EXIT_PENDING,/*this packet is an IO waking up the object, don't drain it*/
    FBE_STATUS_IO_FAILED_RETRYABLE,
    FBE_STATUS_IO_FAILED_NOT_RETRYABLE,

    FBE_STATUS_COMPLETE_JOB_DESTROY, /* This is used by Job service to complete the packet during queue destroy */
    FBE_STATUS_QUIESCED, /* Used by block transport server to error monitor ops that come to a held server. */
    FBE_STATUS_GENERIC_FAILURE,
    FBE_STATUS_NO_ACTION /* This will indicate that requested action is already performed */
}fbe_status_t;

enum fbe_magic_number_e{
    FBE_MAGIC_NUMBER_INVALID,
    FBE_MAGIC_NUMBER_BASE_PACKET        = 0xFBE0FBE0,
    FBE_MAGIC_NUMBER_EVENT              = 0xFBE1FBE1,
    FBE_MAGIC_NUMBER_MEMORY_REQUEST     = 0xFBE2FBE2,
    FBE_MAGIC_NUMBER_SIGNATURE          = 0xFBE4FBE4,
    FBE_MAGIC_NUMBER_DESTROYED_PACKET   = 0XFBE5FBE5,
    FBE_MAGIC_NUMBER_NONPAGED_METADATA  = 0xFBE7FBE7,
    FBE_MAGIC_NUMBER_OBJECT             = 0xFBE8FBE8,
    FBE_MAGIC_NUMBER_RAW_MIRROR_IO      = 0xFBEAFBEA,

    FBE_MAGIC_NUM_RDGEN_REQUEST         = 0xFBE0EDE0,
    FBE_MAGIC_NUM_RDGEN_OBJECT          = 0xFBE0EDE1,
    FBE_MAGIC_NUM_RDGEN_TS              = 0xFBE0EDE2,
    FBE_MAGIC_NUM_RDGEN_PEER_REQUEST    = 0xFBE0EDE3,
    FBE_MAGIC_NUM_RDGEN_EXTRA_SG        = 0xFBE0EDE4,

    /* -CA- is Core Action.. to have IOTS/SIOTS run on the same core */
    FBE_MAGIC_NUMBER_IOTS_REQUEST       = 0xFBE0FCA0,
    FBE_MAGIC_NUMBER_SIOTS_REQUEST      = 0xFBE0FCA1,
};

#define FBE_UNREFERENCED_PARAMETER(_p) (void) (_p)

#ifndef NULL
#define NULL 0
#endif 

#define FBE_ASSERT_AT_COMPILE_TIME(_expr) switch(0) { case 0: case _expr: ; }

#define FBE_UNSIGNED_MINUS_1            0xFFFFFFFFu

#ifndef UEFI_ENV
/* The standard build environment.
 */
typedef char fbe_char_t;
typedef char fbe_s8_t;
typedef unsigned char fbe_u8_t;
typedef unsigned short fbe_u16_t;
typedef int fbe_s32_t;
typedef unsigned int fbe_u32_t;
#else
/* Data types supported in the UEFI environment
 */
typedef CHAR8 fbe_char_t;
typedef INT8 fbe_s8_t;
typedef UINT8 fbe_u8_t;
typedef UINT16 fbe_u16_t;
typedef INT32 fbe_s32_t;
typedef UINT32 fbe_u32_t;
#endif

#define FBE_U64_MAX CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#define FBE_U32_MAX 0xffffffff
#define FBE_U16_MAX 0xffff
#define FBE_BYTE_NUM_BITS (sizeof(fbe_char_t) * 8)
#define FBE_U16_NUM_BITS (sizeof(fbe_u16_t) * 8)
#define FBE_U32_NUM_BITS (sizeof(fbe_u32_t) * 8)
#define FBE_IS_VAL_32_BITS(val) (val > FBE_U32_MAX)

typedef fbe_u32_t fbe_bool_t;
#define FBE_TRUE 1
#define FBE_FALSE 0
#define FBE_IS_TRUE(val) (val == FBE_TRUE)
#define FBE_IS_FALSE(val) (val == FBE_FALSE)

/* FBE_MIN returns the smaller of two values.
 */
#define FBE_MIN(a,b) ((a) < (b) ? (a) : (b))

/* FBE_MIN returns the larger of two values.
 */
#define FBE_MAX(a,b) ((a) > (b) ? (a) : (b))

#ifndef UEFI_ENV
/* The standard build environment.
 */
/* Please pay attention that __int64 is not in ANSI C standard */
typedef unsigned __int64 fbe_u64_t;
typedef unsigned __int64 fbe_dbgext_ptr;
typedef __int64 fbe_s64_t;
#else
/* Data types supported in the UEFI environment
 */
typedef UINT64 fbe_u64_t;
typedef UINT64 fbe_dbgext_ptr;
typedef INT64 fbe_s64_t;
#endif

typedef fbe_u32_t fbe_object_id_t;

typedef fbe_u64_t fbe_sas_address_t;
typedef fbe_u64_t fbe_ses_address_t;

typedef fbe_u64_t fbe_lba_t;
typedef fbe_u64_t fbe_block_count_t;
typedef fbe_u32_t fbe_block_size_t; 

typedef fbe_u32_t fbe_elements_per_parity_t;
typedef fbe_u32_t fbe_element_size_t;

typedef fbe_u64_t fbe_chunk_index_t;
typedef fbe_u32_t fbe_chunk_count_t;
typedef fbe_u32_t fbe_chunk_size_t; 
typedef fbe_u8_t * fbe_chunk_info_t;
typedef fbe_u64_t fbe_config_generation_t;


typedef fbe_u64_t fbe_slice_count_t;
typedef fbe_u64_t fbe_slice_index_t;

/*!*************************************************************************
 *  @var typedef fbe_u32_t fbe_enclosure_number_t
 *  @brief type definition for fbe_enclosure_number_t
 ***************************************************************************/ 
typedef fbe_u32_t fbe_port_number_t;
typedef fbe_u32_t fbe_enclosure_number_t;
typedef fbe_u32_t fbe_slot_number_t;
typedef fbe_u32_t fbe_enclosure_slot_number_t;
typedef fbe_u32_t fbe_enclosure_component_id_t;
typedef fbe_u32_t fbe_component_id_t;

#ifdef _AMD64_
typedef fbe_u64_t fbe_ptrhld_t;
#else
typedef fbe_u32_t fbe_ptrhld_t;
#endif

#ifndef UEFI_ENV 
#define    FBE_LBA_INVALID CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#define    FBE_CDB_BLOCKS_INVALID CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#define    FBE_SAS_ADDRESS_INVALID CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#define    FBE_CHUNK_INDEX_INVALID  CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#else
#define    FBE_LBA_INVALID 0xFFFFFFFFFFFFFFFF
#define    FBE_CDB_BLOCKS_INVALID 0xFFFFFFFFFFFFFFFF
#define    FBE_SAS_ADDRESS_INVALID 0xFFFFFFFFFFFFFFFF
#define    FBE_CHUNK_INDEX_INVALID 0xFFFFFFFFFFFFFFFF
#endif /* #ifndef UEFI_ENV  */

#define FBE_BUS_ID_INVALID      0xFFFFFFFF
#define FBE_TARGET_ID_INVALID   0xFFFFFFFF
#define FBE_LUN_ID_INVALID      0xFFFFFFFF
#define FBE_COMP_ID_INVALID     0xFFFFFFFF
#define FBE_RAID_ID_INVALID     0xFFFFFFFF

#define FBE_BACKEND_INVALID                 0xFFFFFFFF
#define FBE_ENCLOSURE_SLOT_NUMBER_INVALID   0xFFFFFFFF

/*BUS-ENCLOSURE-SLOT INVALID*/
#define FBE_INVALID_PORT_NUM                0xFF
#define FBE_INVALID_ENCL_NUM                0xFF
#define FBE_INVALID_SLOT_NUM                0xFF
#define FBE_PORT_NUMBER_INVALID             FBE_INVALID_PORT_NUM
#define FBE_ENCLOSURE_NUMBER_INVALID        FBE_INVALID_ENCL_NUM
#define FBE_SLOT_NUMBER_INVALID             FBE_INVALID_SLOT_NUM
//#define FBE_PORT_ID_INVALID                 FBE_INVALID_PORT_NUM

#define FBE_RAID_GROUP_INVALID     FBE_UNSIGNED_MINUS_1

#define FBE_DIPLEX_ADDRESS_INVALID 0xFF

#define FBE_INVALID_KEY_HANDLE 0 /*0xFFFFFFFFFFFFFFFF  Needs to be 0 (NULL) until properly initialized in PVD. 
														If Miniport folks will agree may remain 0 (NULL)*/

/* This type is diplex enclosure address in range 0 - 7 */
typedef fbe_u8_t fbe_diplex_address_t;

enum fbe_cpd_device_id_e {
    FBE_FIBRE_LOOP_ID_INVALID = 0xFFFFFFFF
};

typedef fbe_u32_t fbe_cpd_device_id_t; /* may be converted to windows bus target, 
                                    used as index to CPD ALPA table in FC SPD,
                                    TODO: come up with commen name between FC and SAS*/


typedef union fbe_address_u{
    fbe_diplex_address_t    diplex_address;
    fbe_sas_address_t       sas_address;
    fbe_cpd_device_id_t         cpd_device_id;
}fbe_address_t;

static __forceinline void fbe_flag_set(void * attributes, fbe_u32_t flag)
{
    /* for now we will support 32 bit only */
    fbe_u32_t * attr = (fbe_u32_t *)attributes;

    *attr |= 0x1 << flag;
}

static __forceinline void fbe_flag_reset(void * attributes, fbe_u32_t flag)
{
    /* for now we will support 32 bit only */
    fbe_u32_t * attr = (fbe_u32_t *)attributes;

    *attr &= ~(0x1 << flag);
}

static __forceinline fbe_bool_t fbe_flag_is_set(void * attributes, fbe_u32_t flag)
{
    /* for now we will support 32 bit only */
    fbe_u32_t * attr = (fbe_u32_t *)attributes;

    return ((*attr & (0x1 << flag)) != 0);
}

typedef fbe_u64_t fbe_generation_code_t; /* Windows defines atomic type as long */
typedef fbe_s32_t fbe_drive_configuration_handle_t;

#if 0
#ifdef _AMD64_
#define FBE_POINTER(_type,_name) \
    _type * _name
#else
#define FBE_POINTER(_type,_name) \
    _type * _name;               \
    fbe_u32_t _name##_reserved
#endif
#endif

#define FBE_ALIGN(x) CSX_ALIGN_N(x)

/*!********************************************************************* 
 * @typedef fbe_notification_registration_id_t 
 *  
 * @brief 
 *  This is a unique ID you get when you register, use it for unregistration
 * @ingroup fbe_api_common_interface
 **********************************************************************/
typedef fbe_u64_t fbe_notification_registration_id_t;

#if defined(SIMMODE_ENV)
#define	FBE_MAX_OBJECTS 300
#define FBE_MAX_PHYSICAL_OBJECTS 300
#define FBE_MAX_SEP_OBJECTS 1500

#else
#define	FBE_MAX_OBJECTS 5000

/* 1500 drives * 2 (physical and logical) +
   160 enclosures +
   16 buses +
   1 board = 3177 Round up to 3200

   This is a large config for Megatron.
   */

/* Making it to 5000 for future use*/

#define FBE_MAX_PHYSICAL_OBJECTS 5000

/*(16384 MAX LU + 1000 drives (so add VD to each) + 1000 RG which can be R10 with 1 mirror per drive each so this is 1000 more
if we take the other worst case it's 62 R10 with 16 mirros under it which uses all of the drives in the system. This gives us less that 1000 objects
so we'll go with the 1000 mirros for the 1000 max RG.So 16384 + 2000 + 2000 ~ 20992 after rounding to be devisible by 64*/
#define FBE_MAX_SEP_OBJECTS 20992
#endif

/*ESP has most of the object tracking as tables, 50 should be enouhg for it's usage*/
#define FBE_MAX_ESP_OBJECTS 50

#define	FBE_OBJECT_ID_INVALID 0x7FFFFFFE/*FBE_MAX_OBJECTS + 1*/

/* Pool-id is used for PVD objects to determine which pool the drive belongs to.. */
#define FBE_POOL_ID_INVALID FBE_OBJECT_ID_INVALID+1

/* The first object ids could be reserved */
#define FBE_RESERVED_OBJECT_IDS 0x100
#define FBE_WWN_BYTES 16

/*!********************************************************************* 
 * @typedef fbe_lun_number_t 
 *
 * @brief 
 *      Definition of lun number assigned by unisphere
 **********************************************************************/
typedef fbe_u32_t   fbe_lun_number_t;
/*!********************************************************************* 
 * @typedef fbe_raid_group_number_t 
 *
 * @brief 
 *      Definition of raid group number assigned by unisphere
 **********************************************************************/
typedef fbe_u32_t   fbe_raid_group_number_t;

/*!********************************************************************* 
 * @typedef fbe_assigned_wwid_t 
 *
 * @brief 
 *      Definition of a 128-bit World Wide ID, IEEE type 6.
 *      Represented as a FiberChannel unique ID
 **********************************************************************/
typedef struct fbe_assigned_wwid_s{
    fbe_u8_t bytes[FBE_WWN_BYTES];
} fbe_assigned_wwid_t;

/*!********************************************************************* 
 * @typedef fbe_user_defined_lun_name_t 
 *
 * @brief 
 *      Define the UNICODE character length of the user friendly name
 *      that can be assigned to exported devices. Because we are using
 *      UNICODE characters the byte length is 64.
 **********************************************************************/
#define FBE_USER_DEFINED_LUN_NAME_LEN           64
typedef struct fbe_user_defined_lun_name_s{
    fbe_u8_t name[FBE_USER_DEFINED_LUN_NAME_LEN];
} fbe_user_defined_lun_name_t;

/*based on windows API, might need to be changed if porting*/
#define FBE_SEMAPHORE_MAX 0x7FFFFFFF

#define FBE_CPU_ID_INVALID 0xFFFFFFFF
#define FBE_CPU_ID_MAX 64
#define FBE_CPU_ID_MASK 0x3F

typedef fbe_u32_t fbe_cpu_id_t;
typedef fbe_u64_t fbe_cpu_affinity_mask_t;

#define FBE_GIGABYTE        1073741824

/* The version header definition for SEP package */
#pragma pack(1)
typedef struct fbe_sep_version_header_s {
    fbe_u32_t   size;  /* the valid size */
    fbe_u32_t   padded_reserve_space[3]; /* reserve the space */
}fbe_sep_version_header_t;
#pragma pack()

#if defined(ALAMOSA_WINDOWS_ENV) && defined(CSX_BV_LOCATION_KERNEL_SPACE)
typedef fbe_u8_t fbe_irp_stack_flags_t; /* 8 bits in Windows kernel case - matches EMCPAL_IRP_STACK_FLAGS */
#else
typedef fbe_u16_t fbe_irp_stack_flags_t; /* 16 bits in CSX-backed case - matches EMCPAL_IRP_STACK_FLAGS */
#endif

typedef fbe_u64_t fbe_key_handle_t;

/*!********************************************************************* 
 * @typedef fbe_key_t 
 *
 * @brief 
 *      Defination of Encryption Key  which is 256 byte long.
 **********************************************************************/
#define FBE_KEY_LEN           256
typedef struct fbe_key_s{
    fbe_u8_t key[FBE_KEY_LEN];
} fbe_key_t;
#endif /* FBE_TYPES_H */
