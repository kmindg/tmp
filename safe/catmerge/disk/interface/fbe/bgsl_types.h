#ifndef BGSL_TYPES_H
#define BGSL_TYPES_H

#include "csx_ext.h"

typedef enum bgsl_status_e{
    BGSL_STATUS_INVALID = 0,
    BGSL_STATUS_OK,
    BGSL_STATUS_MORE_PROCESSING_REQUIRED,
    BGSL_STATUS_PENDING,
    BGSL_STATUS_CANCEL_PENDING,
    BGSL_STATUS_CANCELED,
    BGSL_STATUS_TIMEOUT,
    BGSL_STATUS_INSUFFICIENT_RESOURCES,
    BGSL_STATUS_NO_DEVICE,
    BGSL_STATUS_NO_OBJECT,

    BGSL_STATUS_CONTINUE, /* May be used for edge tap functianality ONLY */

    BGSL_STATUS_BUSY,   /* The object is in activate or activate pending state */
    BGSL_STATUS_SLUMBER,/* The object is in hibernate or hibernate pending state */
    BGSL_STATUS_FAILED, /* The object is in fail or fail pending state */
    BGSL_STATUS_DEAD,   /* The object is in destroy or destroy pending state */

    BGSL_STATUS_NOT_INITIALIZED,
    BGSL_STATUS_TRAVERSE, /* Base object returns this status when packet needs to be traversed */

    BGSL_STATUS_EDGE_NOT_ENABLED, /* This status returned when packet sent to not enabled edge */

    BGSL_STATUS_COMPONENT_NOT_FOUND,
    BGSL_STATUS_ATTRIBUTE_NOT_FOUND,
    BGSL_STATUS_UNSUPPORTED_CLASS_ID,
    BGSL_STATUS_UNSUPPORTED_PACKAGE_ID,
    BGSL_STATUS_GENERIC_FAILURE
}bgsl_status_t;

enum bgsl_magic_number_e{
    BGSL_MAGIC_NUMBER_INVALID,
    BGSL_MAGIC_NUMBER_BASE_PACKET = 0xFBE0FBE0,
    BGSL_MAGIC_NUMBER_MGMT_PACKET = 0xFBE1FBE1,
    BGSL_MAGIC_NUMBER_IO_PACKET   = 0xFBE2FBE2,
    BGSL_MAGIC_NUMBER_OBJECT      = 0xFBE8FBE8
};

#define BGSL_UNREFERENCED_PARAMETER(_p) (void) (_p)

#ifndef NULL
#define NULL 0
#endif 

#define BGSL_ASSERT_AT_COMPILE_TIME(_expr) switch(0) { case 0: case _expr: ; }

typedef unsigned char bgsl_u8_t;
typedef unsigned short bgsl_u16_t;

typedef int bgsl_s32_t;
typedef unsigned int bgsl_u32_t;
#define BGSL_U32_MAX 0xffffffff

typedef bgsl_u32_t bgsl_bool_t;
#define BGSL_TRUE 1
#define BGSL_FALSE 0
#define BGSL_IS_TRUE(val) (val == BGSL_TRUE)
#define BGSL_IS_FALSE(val) (val == BGSL_FALSE)

/* BGSL_MIN returns the smaller of two values.
 */
#define BGSL_MIN(a,b) ((a) < (b) ? (a) : (b))

/* BGSL_MIN returns the larger of two values.
 */
#define BGSL_MAX(a,b) ((a) > (b) ? (a) : (b))

/* Please pay attention that __int64 is not in ANSI C standard */
typedef unsigned __int64 bgsl_u64_t;
typedef unsigned __int64 bgsl_dbgext_ptr;
typedef __int64 bgsl_s64_t;

typedef bgsl_u32_t bgsl_object_id_t;

typedef bgsl_u64_t bgsl_sas_address_t;
typedef bgsl_u64_t bgsl_ses_address_t;

typedef bgsl_u64_t bgsl_lba_t;
typedef bgsl_u32_t bgsl_block_count_t;
typedef bgsl_u32_t bgsl_block_size_t; 


#define    BGSL_LBA_INVALID 0xFFFFFFFFFFFFFFFF


#define    BGSL_SAS_ADDRESS_INVALID 0xFFFFFFFFFFFFFFFF

#define BGSL_BUS_ID_INVALID      0xFFFFFFFF
#define BGSL_TARGET_ID_INVALID   0xFFFFFFFF
#define BGSL_LUN_ID_INVALID      0xFFFFFFFF

#define BGSL_DIPLEX_ADDRESS_INVALID 0xFF
/* This type is diplex enclosure address in range 0 - 7 */
typedef bgsl_u8_t bgsl_diplex_address_t;

enum bgsl_cpd_device_id_e {
    BGSL_FIBRE_LOOP_ID_INVALID = 0xFFFFFFFF
};

typedef bgsl_u32_t bgsl_cpd_device_id_t; /* may be converted to windows bus target, 
                                    used as index to CPD ALPA table in FC SPD,
                                    TODO: come up with commen name between FC and SAS*/

typedef bgsl_u32_t bgsl_port_number_t;

typedef union bgsl_address_u{
    bgsl_diplex_address_t    diplex_address;
    bgsl_sas_address_t       sas_address;
    bgsl_cpd_device_id_t         cpd_device_id;
}bgsl_address_t;

static __forceinline void bgsl_flag_set(void * attributes, bgsl_u32_t flag)
{
    /* for now we will support 32 bit only */
    bgsl_u32_t * attr = (bgsl_u32_t *)attributes;

    *attr |= 0x1 << flag;
}

static __forceinline void bgsl_flag_reset(void * attributes, bgsl_u32_t flag)
{
    /* for now we will support 32 bit only */
    bgsl_u32_t * attr = (bgsl_u32_t *)attributes;

    *attr &= ~(0x1 << flag);
}

static __forceinline bgsl_bool_t bgsl_flag_is_set(void * attributes, bgsl_u32_t flag)
{
    /* for now we will support 32 bit only */
    bgsl_u32_t * attr = (bgsl_u32_t *)attributes;

    return ((*attr & (0x1 << flag)) != 0);
}

typedef bgsl_u64_t bgsl_generation_code_t; /* Windows defines atomic type as long */
typedef bgsl_s32_t bgsl_drive_configuration_handle_t;

#endif /* BGSL_TYPES_H */
