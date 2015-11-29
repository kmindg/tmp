#ifndef FBE_RAID_FRUTS_H
#define FBE_RAID_FRUTS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_fruts.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition of the fru tracking structure.
 *
 * @version
 *   5/14/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_raid_common.h"
#include "fbe/fbe_raid_ts.h"
#include "fbe_raid_memory_api.h"
#include "fbe_block_transport.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @struct fbe_raid_fru_info_t
 *********************************************************************
 * @brief Provides information on the disk operation to be used
 *        by each fru position.
 *
 *********************************************************************/
typedef struct fbe_raid_fru_info_s
{
    fbe_lba_t           lba;
    fbe_block_count_t   blocks;
    fbe_u16_t           position;
    fbe_raid_sg_index_t sg_index;
}
fbe_raid_fru_info_t;

/*!*******************************************************************
 * @struct fbe_raid_fru_eboard_t
 *********************************************************************
 * @brief
 *  The FRU Eboard structure is used to report status on operations to set of disks.
 *
 *********************************************************************/
typedef struct fbe_raid_fru_eboard_s
{
    fbe_u8_t hard_media_err_count; /*!< Count of hard media errors.*/
    fbe_u8_t menr_err_count; /*!< Count of media error no remap errs. */
    fbe_u8_t drop_err_count; /*!< Count of dropped errors. */
    fbe_u8_t dead_err_count; /*!< Count of dead errors. */
    fbe_u8_t soft_media_err_count; /*!< Count of soft recovered media errors. */
    fbe_u8_t abort_err_count; /*!< Count of aborted errors. */
    fbe_u8_t retry_err_count; /*!< Count of errors we can retry. */
    fbe_u8_t timeout_err_count; /*< Count of expired fruts. */
    fbe_u8_t unexpected_err_count; /*!< Count of unexpected errors. */
    fbe_u8_t zeroed_count; /*!< Count of zeroed requests. */
    fbe_u8_t bad_crc_count; /*!< Count of writes with bad crc status. */
    fbe_u8_t not_preferred_count; /*!< Count of I/Os with not preferred status. */
    fbe_u8_t reduce_qdepth_count; /*!< Count of I/Os with status to reduce queue depth. */
    fbe_u8_t reduce_qdepth_soft_count; /*!< Count of I/Os with success status to reduce queue depth. */

    fbe_u16_t hard_media_err_bitmap; /*!< Bitmap of hard media errors. */
    fbe_u16_t menr_err_bitmap; /*!< Bitmap of media error no remap errs. */
    fbe_u16_t drop_err_bitmap; /*!< Bitmap of dropped errors. */
    fbe_u16_t dead_err_bitmap; /*!< Bitmap of dead errors. */
    fbe_u16_t retry_err_bitmap; /*!< Bitmap of retryable errors. */
    fbe_u16_t zeroed_bitmap;    /*!< Bitmap of positions with zeros. */
}
fbe_raid_fru_eboard_t;

/*!*******************************************************************
 * @enum fbe_raid_fru_error_status_t
 *********************************************************************
 * @brief
 *  This is a classification of the status from a fruts chain.
 *  This takes into account prioritization of errors.
 *
 *********************************************************************/
typedef enum fbe_raid_fru_error_status_s
{
    FBE_RAID_FRU_ERROR_STATUS_INVALID = 0, /*! Not initialized. */
    FBE_RAID_FRU_ERROR_STATUS_SUCCESS, /*!< No error encountered. */

    /*! We are waiting to determine what to do (mark nr or shutdown). */
    FBE_RAID_FRU_ERROR_STATUS_WAITING,
    FBE_RAID_FRU_ERROR_STATUS_DEAD,     /*!< I/O failed but retryable after health evaluation */
    FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN, /*!< The raid group is shutting down. */
    FBE_RAID_FRU_ERROR_STATUS_ABORTED,  /*!< This request is aborted. */
    FBE_RAID_FRU_ERROR_STATUS_ERROR,    /*!< We received some error (media error, etc. */
    FBE_RAID_FRU_ERROR_STATUS_RETRY,     /*!< A retryable error was encountered. */
    FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED, /*!< An unexpected error was encountered. */
    FBE_RAID_FRU_ERROR_STATUS_INVALIDATE, /*!< Need to invalidate the blocks. */
    FBE_RAID_FRU_ERROR_STATUS_BAD_CRC,  /*!< bad crc found on a write request. */
    FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED,  /*!< We went not preferred and client said we can fail it. */
    FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD,  /*!< We failed this I/O and client should reduce qdepth. */
    FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT,  /*!< We succeeded, but client should reduce qdepth. */
    FBE_RAID_FRU_ERROR_STATUS_LAST
}
fbe_raid_fru_error_status_t; 

/*!*******************************************************************
 * @enum fbe_raid_fru_error_type_t
 *********************************************************************
 * @brief
 *  This is a categorization of the type of error a given fruts has.
 *  Many packet and/or blcok payload errors can map to a single error type.
 *
 *********************************************************************/
typedef enum fbe_raid_fru_error_type_s
{
    FBE_RAID_FRU_ERROR_TYPE_INVALID = 0, /*! Not initialized. */
    FBE_RAID_FRU_ERROR_TYPE_SUCCESS,     /*!< No error encountered. */

    FBE_RAID_FRU_ERROR_TYPE_NON_RETRYABLE, /*!< I/O failed and the device is not available. */
    FBE_RAID_FRU_ERROR_TYPE_RETRYABLE, /*!< I/O failed and can be retried later. */
	FBE_RAID_FRU_ERROR_TYPE_HARD_MEDIA, /*!< I/O failed and can't recover */
	FBE_RAID_FRU_ERROR_TYPE_SOFT_MEDIA, /*!< we had a soft media error on the I/O */
    FBE_RAID_FRU_ERROR_TYPE_LAST
}
fbe_raid_fru_error_type_t; 


/* Simply zero out the eboard.
 */
static __forceinline void fbe_raid_fru_eboard_init(fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_zero_memory(eboard_p, sizeof(fbe_raid_fru_eboard_t));
    return;
}

 /*!*******************************************************************
  * @enum fbe_raid_fruts_flags_t
  *********************************************************************
  * @brief The flags that pertain to the fruts.
  *
  *********************************************************************/
typedef enum fbe_raid_fruts_flags_e
{
    FBE_RAID_FRUTS_FLAG_INVALID           = 0x00000000,    /*< No flags set. */
    FBE_RAID_FRUTS_FLAG_OPTIMIZE          = 0x00000001,    /*< Use the mirror optimized start algorithm (if not degraded) */
    FBE_RAID_FRUTS_FLAG_FINISH_OPTIMIZE   = 0x00000002,    /*< Use the mirror optimized completion */
    FBE_RAID_FRUTS_FLAG_STRIPED_MIRROR    = 0x00000004,    /*< Used to determine how many bytes per block should be received */
    FBE_RAID_FRUTS_FLAG_BUSY_SWAP         = 0x00000008,    /*< DH expects the same vid when a busied request is retried */
    FBE_RAID_FRUTS_FLAG_UNALIGNED_WRITE   = 0x00000010,    /*< The drive write request is not aligned.  Required preread. */
    FBE_RAID_FRUTS_FLAG_ERROR_INJECTED    = 0x00000020,    /*< This Fruts had an error injected on it. */
    FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED     = 0x00000040,    /*< This fruts packet was initted and needs to be freed. */
    FBE_RAID_FRUTS_FLAG_ERROR_INHERITED   = 0x00000080,    /*< This fruts error was inherited from the parent. */
    FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING = 0x00000100,   /*< Used to determine whether the request is completed or not*/
    FBE_RAID_FRUTS_FLAG_RETRY_PHYSICAL_ERROR= 0x00000400,   /*< Set when we are retrying physical error. */
    FBE_RAID_FRUTS_FLAG_LAST                = 0x00000800,
 }
 fbe_raid_fruts_flags_t;

/*!*******************************************************************
 * @struct fbe_raid_fruts_t
 *********************************************************************
 * @brief This is the structure raid uses to track an I/O to a fru.
 *
 *********************************************************************/
typedef struct fbe_raid_fruts_s
{
    fbe_raid_common_t       common;

    fbe_u32_t               position;           /*!< position in array of FRUs */
    fbe_raid_fruts_flags_t flags;              /*!<  flags that are not forwarded to IORB */
    fbe_payload_block_operation_opcode_t opcode; /*!< This is the the type of block operation requested */
    fbe_lba_t               lba;                /*!< lba for this drive */
    fbe_block_count_t  blocks;             /*!< number of sectors */

    fbe_sg_element_t       *sg_p;               /*!< scatter gather allocated to this drive. */
    fbe_memory_id_t        sg_id;               /*!< Memory id of the scatter gather list. */
    fbe_u32_t              sg_element_offset;   /*!< Number of elements to skip for first valid. */

    /* The following are filled out use the block payload
     * status and qualifier.
     */
    fbe_payload_block_operation_status_t	status;
    fbe_payload_block_operation_qualifier_t status_qualifier;
    fbe_lba_t                               media_error_lba;
    fbe_object_id_t                      pvd_object_id;
    
    /* This is a dupliate of the pre-read descriptor in the packet
     */
    fbe_payload_pre_read_descriptor_t write_preread_desc; /*!< Pre(and post) read to handle unaligned requests */
    
    fbe_u32_t exported_block_size;/*!< The bytes per logical block used by RAID */
    fbe_u32_t optimal_block_size; /*!< The number of logical blocks that result in an aligned physical request */
    fbe_u32_t physical_block_size;/*!< The bytes per physical block for the destination drive */

    fbe_u32_t retry_count; /*!< number of times we retried due to a I/O failed/retry possible error */
    fbe_time_t time_stamp; /*!< When we originally sent this FRUTS. */
    fbe_block_transport_control_logical_error_t logical_error; /*! We need this in case we need to send a usurper downstream. */
    fbe_sg_element_t sg[2];
    /*! Packet to send to next level.
     */
    FBE_ALIGN(16)fbe_packet_t            io_packet;
} 
fbe_raid_fruts_t;

/*!********************************************************************* 
 * @struct fbe_raid_fruts_control_operation_status_info_t 
 *  
 * @brief 
 *   This contains the data info for Control Operation Status Info structure.
 *
 **********************************************************************/
typedef struct fbe_raid_fruts_control_operation_status_info_s{
    fbe_payload_control_status_t            control_operation_status;    /*!< Control Operation Status */
    fbe_payload_control_status_qualifier_t  control_operation_qualifier; /*!< Control Operation Qualifier */
    fbe_status_t                            packet_status;               /*!< Packet Status */
    fbe_u32_t                               packet_qualifier;            /*!< Packet Qualifier */
}fbe_raid_fruts_control_operation_status_info_t;

/***************************************************
 * FRU TS macro definitions.
 ***************************************************/
static __inline fbe_raid_fruts_t* fbe_raid_fruts_get_next(fbe_raid_fruts_t *fruts_p)
{
    return ((fbe_raid_fruts_t*)fbe_raid_common_get_next(&fruts_p->common));
}
static __inline void fbe_raid_fruts_set_siots(fbe_raid_fruts_t *fruts_p, fbe_raid_siots_t *parent_p) 
{
    fbe_raid_common_set_parent(&((fbe_raid_fruts_t *)fruts_p)->common, 
                                &((fbe_raid_siots_t *)parent_p)->common);
}

/*!**************************************************************
 *          fbe_raid_fruts_get_siots()
 ****************************************************************
 * @brief
 *  Get the parent siots from a fruts.
 *
 * @param fruts_p - fruts to get siots parent for. 
 *
 * @return fbe_raid_siots_t*
 *
 * @author
 *  5/20/2009   Ron Proulx  - Created
 *
 ****************************************************************/

static __inline fbe_raid_siots_t* fbe_raid_fruts_get_siots(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_common_t *common_p = NULL;

    if (!fbe_raid_common_is_flag_set(&fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS))
    {
        /* Input was not a fruts.
         */
        return NULL;
    }
    common_p = fbe_raid_common_get_parent(&fruts_p->common);

    if (common_p == NULL)
    {
        /* Not a valid parent.
         */
        return NULL;
    }

    if (fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_SIOTS))
    {
        /* This is a siots.
         */
    }
    else if (fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS))
    {
        /* This is a nested siots.
         */
    }
    else
    {
        /* Unknown flags combination.
         */
        common_p = NULL;
    }

    /* Cast this common back to an siots.
     */
    return((fbe_raid_siots_t *)common_p);
}
/******************************************
 * end fbe_raid_fruts_get_siots()
 ******************************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static __forceinline void fbe_raid_fruts_init_flags(fbe_raid_fruts_t *fruts_p,
                                             fbe_raid_fruts_flags_t flags)
{
    fruts_p->flags = flags;
    return;
}

static __forceinline void fbe_raid_fruts_set_flag(fbe_raid_fruts_t *fruts_p,
                                           fbe_raid_fruts_flags_t flag)
{
    fruts_p->flags |= flag;
    return;
}

static __forceinline void fbe_raid_fruts_clear_flag(fbe_raid_fruts_t *fruts_p,
                                             fbe_raid_fruts_flags_t flag)
{
    fruts_p->flags &= ~flag;
    return;
}

static __forceinline fbe_raid_fruts_flags_t fbe_raid_fruts_get_flags(fbe_raid_fruts_t *fruts_p)
{
    return fruts_p->flags;
}

static __forceinline fbe_bool_t fbe_raid_fruts_is_flag_set(fbe_raid_fruts_t *fruts_p,
                                                    fbe_raid_fruts_flags_t flags)
{
    return ((fruts_p->flags & flags) == flags);
}

static __forceinline void fbe_raid_fruts_set_position(fbe_raid_fruts_t *fruts_p,
                                               fbe_u32_t position)
{
    fruts_p->position = position;
    return;
}
static __forceinline fbe_packet_t *fbe_raid_fruts_get_packet(fbe_raid_fruts_t *fruts_p)
{
    return &fruts_p->io_packet;
}
static __forceinline fbe_status_t fbe_raid_fruts_set_block_status(fbe_raid_fruts_t *fruts_p, 
                                                           fbe_payload_block_operation_status_t status,
                                                           fbe_payload_block_operation_qualifier_t status_qualifier)
{
	fruts_p->status = status;
	fruts_p->status_qualifier = status_qualifier;
	return FBE_STATUS_OK;
}

/* Accessors for the memory request information
 */
static __forceinline fbe_memory_request_t *fbe_raid_fruts_get_memory_request(fbe_raid_fruts_t *fruts_p)
{
    return(&fruts_p->io_packet.memory_request);
}
static __forceinline void fbe_raid_fruts_get_memory_request_priority(fbe_raid_fruts_t *fruts_p,
                                                                    fbe_memory_request_priority_t *priority_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_fruts_get_memory_request(fruts_p);

    *priority_p = memory_request_p->priority;
    return;
}
static __forceinline void fbe_raid_fruts_set_memory_request_priority(fbe_raid_fruts_t *fruts_p,
                                                                     fbe_memory_request_priority_t new_priority)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_fruts_get_memory_request(fruts_p);

    memory_request_p->priority = new_priority;
    return;
}
static __forceinline void fbe_raid_fruts_get_memory_request_io_stamp(fbe_raid_fruts_t *fruts_p,
                                                                     fbe_memory_io_stamp_t *io_stamp_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_fruts_get_memory_request(fruts_p);

    *io_stamp_p = memory_request_p->io_stamp;
    return;
}
static __forceinline void fbe_raid_fruts_set_memory_request_io_stamp(fbe_raid_fruts_t *fruts_p,
                                                                     fbe_memory_io_stamp_t new_io_stamp)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_fruts_get_memory_request(fruts_p);

    memory_request_p->io_stamp = new_io_stamp;
    return;
}

fbe_status_t fbe_raid_fruts_get_block_status(fbe_raid_fruts_t *fruts_p,
                                             fbe_payload_block_operation_status_t *block_status_p);

fbe_status_t fbe_raid_fruts_get_status(fbe_raid_fruts_t *fruts_p,
                                       fbe_status_t *transport_status_p,
                                       fbe_u32_t *transport_qualifier_p,
                                       fbe_payload_block_operation_status_t *block_status_p,
                                       fbe_payload_block_operation_qualifier_t *block_qualifier_p);
fbe_status_t fbe_raid_fruts_get_media_error_lba(fbe_raid_fruts_t *fruts_p,
                                                fbe_lba_t *media_error_lba_p);
fbe_payload_block_operation_t *fbe_raid_fruts_to_block_operation(fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_get_retry_wait_msecs(fbe_raid_fruts_t *fruts_p,
                                                 fbe_lba_t *retry_wait_msecs_p);
static __inline void *fbe_raid_memory_id_get_memory_ptr(fbe_memory_id_t mem_id);
static __inline void fbe_raid_fruts_get_sg_ptr(fbe_raid_fruts_t *fruts_p,
                                             fbe_sg_element_t **sg_pp)
{
    /* In some cases we do not allocate an sg, but we pass down the sg from the client. 
     * Thus the sg_id is NULL. 
     */
    if (fruts_p->sg_id == NULL)
    {
        /* No Sg is allocated, use the sg_p from the fruts.
         */
        *sg_pp = fruts_p->sg_p;
    }
    else
    {
        /* Sg is allocated, extract the ptr to the sg list, using the offset
         */
        *sg_pp = (fbe_sg_element_t *)fbe_raid_memory_id_get_memory_ptr(fruts_p->sg_id);
        *sg_pp += fruts_p->sg_element_offset;
    }
    return;
}
/* Version of fbe_raid_fruts_get_sg_ptr which returns the value rather than setting
   a passed-in value pointer.
   This makes substituting for fbe_raid_memory_id_get_memory_ptr(fruts_p->sg_id) more transparent.                                                                                              .
   */
static __inline fbe_sg_element_t *fbe_raid_fruts_return_sg_ptr(fbe_raid_fruts_t *fruts_p)
{
    fbe_sg_element_t *sg_p;
    fbe_raid_fruts_get_sg_ptr(fruts_p, &sg_p);
    return (sg_p);
}

fbe_status_t fbe_raid_fruts_init_request(fbe_raid_fruts_t *const fruts_p,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_u32_t position);
fbe_status_t fbe_raid_fruts_init_read_request(fbe_raid_fruts_t *const fruts_p,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_u32_t position);
fbe_status_t fbe_raid_fruts_chain_destroy(fbe_queue_head_t *queue_head_p,
                                          fbe_u16_t *num_fruts_destroyed_p);
fbe_status_t fbe_raid_fruts_inc_error_count(fbe_raid_fruts_t *fruts_p,
                                            fbe_raid_fru_error_type_t err_type);
fbe_status_t fbe_raid_fru_eboard_display(fbe_raid_fru_eboard_t *eboard_p,
                                         fbe_raid_siots_t *siots_p,
                                         fbe_trace_level_t trace_level);

fbe_status_t fbe_raid_fruts_get_failed_io_pos_bitmap(fbe_raid_fruts_t   *const in_fruts_p,
                                                     fbe_u32_t          *out_failed_io_position_bitmap_p);

fbe_status_t fbe_raid_fruts_is_fruts_initialized(fbe_raid_fruts_t   *const in_fruts_p, 
                                                 fbe_u32_t          *out_is_init_b);
fbe_status_t fbe_raid_fruts_initialize_transport_packet(fbe_raid_fruts_t   *const in_fruts_p);

fbe_status_t  fbe_raid_fruts_usurper_send_logical_error_to_pdo(fbe_raid_fruts_t * fruts_p, 
                                                              fbe_packet_t* in_packet_p,
                                                              fbe_u16_t error_type, 
                                                              fbe_u16_t fru_pos);
fbe_bool_t fbe_raid_fruts_check_degraded_corrupt_data(fbe_raid_fruts_t *fruts_p,
                                                     fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_fruts_chain_send_timeout_usurper(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_position_bitmask_t bitmask_to_send);
fbe_status_t fbe_raid_fruts_chain_send_crc_usurper(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_position_bitmask_t bitmask_to_send);
fbe_status_t fbe_raid_siots_send_timeout_usurper(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_siots_send_crc_usurper(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_fruts_increment_eboard_counters(fbe_raid_fru_eboard_t *fru_eboard, fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_init_cpu_affinity_packet(fbe_raid_fruts_t *fruts_p);

#endif /* end FBE_RAID_FRUTS_H */
/*************************
 * end file fbe_raid_fru_ts.h
 *************************/


