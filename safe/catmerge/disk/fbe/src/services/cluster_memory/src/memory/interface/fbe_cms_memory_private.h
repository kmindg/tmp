#ifndef	FBE_CMS_MEMORY_PRIVATE_H
#define	FBE_CMS_MEMORY_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cms_memory_private.h
 ***************************************************************************
 *
 * @brief
 *  This file defines the data types and function prototypes used to access
 *  persistent memory.
 *
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe_types.h"
#include "fbe_winddk.h"
#include "K10NvRamLib.h"
#include "fbe_cms_memory.h"
#include "fbe_cms_defines.h"

#define FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE        "~$FBE$CMS$GLOBAL$DATA$~"
#define FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE_SIZE   24

/* Align buffer and global data allocations to the size of a buffer. */
#define FBE_CMS_MEMORY_FORWARD_ALLOCATION_ALIGNMENT     FBE_CMS_BUFFER_SIZE

/* Align tag allocations to a 128-byte cache line. */
#define FBE_CMS_MEMORY_BACKWARD_ALLOCATION_ALIGNMENT    0x80

/* Align mapping to a 4MB page table boundary. */
#define FBE_CMS_MEMORY_MAPPING_ALIGNMENT            0x400000

/* "SIOB" in ASCII */
#define FBE_CMS_MEMORY_BIOS_OBJECT_ID                   0xC4B8E0A3
#define FBE_CMS_MEMORY_BIOS_REVISION_ID                 0x02
#define FBE_CMS_MEMORY_BIOS_PERSISTENCE_REQUESTED       0x1
#define FBE_CMS_MEMORY_BIOS_PERSISTENCE_NOT_REQUESTED   0x0

/*! @struct fbe_cms_bios_persistence_request_t
 *  @brief This structure is located in NVRAM.
 */
#pragma pack(1)
typedef struct fbe_cms_bios_persistence_request_s
{
    fbe_u32_t   object_id;
    fbe_u16_t   revision_id;
    fbe_u16_t   persist_request;
    fbe_u32_t   persist_status;
    fbe_u8_t    reserved_bytes[8];
} fbe_cms_bios_persistence_request_t;
#pragma pack()

/*! @struct fbe_cms_memory_sgl_t
 *  @brief 
 */
typedef struct fbe_cms_memory_sgl_s
{
    fbe_u64_t  physical_address;
    fbe_u64_t  length;

} fbe_cms_memory_sgl_t;

/*! @struct fbe_cms_memory_extended_sgl_t
 *  @brief 
 */
typedef struct fbe_cms_memory_extended_sgl_s
{
    fbe_u64_t  physical_address;
    fbe_u64_t  length;
    fbe_u8_t * virtual_address;
    csx_p_io_map_t io_map_obj;

} fbe_cms_memory_extended_sgl_t;


/*! @struct fbe_cms_memory_persisted_data_t
 *  @brief 
 */
typedef struct fbe_cms_memory_persisted_data_s
{
    fbe_char_t  signature[FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE_SIZE];
    fbe_u32_t   unsafe_to_remove;
    fbe_u32_t   persistence_requested;
} fbe_cms_memory_persisted_data_t;


/*! @struct fbe_cms_memory_volatile_data_t
 *  @brief 
 */
typedef struct fbe_cms_memory_volatile_data_s
{
    fbe_spinlock_t                          lock;
    fbe_bool_t                              init_completed;
    fbe_cms_memory_persist_status_t         status;

    fbe_cms_memory_extended_sgl_t           original_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
    fbe_cms_memory_extended_sgl_t           working_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
    fbe_cms_memory_extended_sgl_t           peer_original_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
    fbe_cms_memory_extended_sgl_t           peer_working_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH]; 

    NVRAM_SECTION_RECORD                    nvram_section_info;
    fbe_u8_t                            *   nvram_data_p;
    fbe_cms_bios_persistence_request_t  *   bios_mp_p;

    fbe_cms_memory_persisted_data_t     *   persisted_data_p;
    fbe_cms_memory_buffer_descriptor_t  *   buffer_map;

    fbe_u32_t                               number_of_buffers;
    fbe_u32_t                               buffer_size;
    fbe_u32_t                               forward_allocation_alignment;
    fbe_u32_t                               backward_allocation_alignment;
} fbe_cms_memory_volatile_data_t;

/*! @enum fbe_cms_memory_cmi_msg_type_t
 *  @brief 
 */
typedef enum fbe_cms_memory_cmi_msg_type_e
{
    FBE_CMS_MEMORY_CMI_MSG_TYPE_INVALID,
    FBE_CMS_MEMORY_CMI_MSG_TYPE_SEND_PARAMETERS,
    FBE_CMS_MEMORY_CMI_MSG_TYPE_SEND_SYSTEM_SGL,
    FBE_CMS_MEMORY_CMI_MSG_TYPE_LAST

} fbe_cms_memory_cmi_msg_type_t;

/*! @struct fbe_cms_memory_cmi_send_parameters_t
 *  @brief 
 */
typedef struct fbe_cms_memory_cmi_send_parameters_s
{
    fbe_u32_t   number_of_buffers;
    fbe_u32_t   buffer_size;
    fbe_u32_t   forward_allocation_alignment;
    fbe_u32_t   backward_allocation_alignment;
} fbe_cms_memory_cmi_send_parameters_t;

/*! @struct fbe_cms_memory_cmi_send_system_sgl_t
 *  @brief 
 */
typedef struct fbe_cms_memory_cmi_send_system_sgl_s
{
	fbe_cms_memory_sgl_t sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
} fbe_cms_memory_cmi_send_system_sgl_t;


struct fbe_cms_memory_cmi_msg_s;
typedef fbe_status_t (*fbe_cms_memory_cmi_callback_t)(struct fbe_cms_memory_cmi_msg_s *returned_msg, fbe_status_t completion_status, void *context);

/*! @struct fbe_cms_memory_cmi_msg_t
 *  @brief 
 */
typedef struct fbe_cms_memory_cmi_msg_s
{
	fbe_cms_memory_cmi_msg_type_t	msg_type;
    fbe_cms_memory_cmi_callback_t 	completion_callback;
    
     union{
         fbe_cms_memory_cmi_send_parameters_t parameters;
         fbe_cms_memory_cmi_send_system_sgl_t system_sgl;
     } payload;
} fbe_cms_memory_cmi_msg_t;


/* Private Function prototypes */

/* These are implemented as common code, and will be called from sim/hw code */
void fbe_cms_memory_init_volatile_data();
fbe_status_t fbe_cms_memory_init_common(fbe_cms_memory_persist_status_t reason, fbe_u32_t number_of_buffers);
fbe_status_t fbe_cms_memory_destroy_common();

/* These are implemented for sim/hw, and will be called by common code*/
fbe_status_t fbe_cms_memory_get_memory();
fbe_status_t fbe_cms_memory_set_unsafe_to_remove();
fbe_status_t fbe_cms_memory_set_request_persistence();

#endif /* #ifndef FBE_CMS_MEMORY_PRIVATE_H */
