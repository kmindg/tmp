#ifndef __FBE_RAID_MEMORY_API_H__
#define __FBE_RAID_MEMORY_API_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory_api.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of the raid memory allocations.
 *
 * @version
 *   7/31/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_memory.h"
#include "fbe_raid_fruts.h"

/*!*******************************************************************
 * @def FBE_RAID_MEMORY_HEADER_MAGIC_NUMBER
 *********************************************************************
 * @brief This is the magic number we put into our memory header.
 * 
 *        This is important since it allows us to catch sg or buffer
 *        overruns.
 *
 *********************************************************************/
#define FBE_RAID_MEMORY_HEADER_MAGIC_NUMBER 0xAB0DEFBE

/*!*******************************************************************
 * @def FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER
 *********************************************************************
 * @brief This is the magic number we put into our memory FOOTER.
 * 
 *        This is important since it allows us to catch sg or buffer
 *        overruns.
 *
 *********************************************************************/
#define FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER 0xFBEAB0DE

/*!*******************************************************************
 * @def FBE_RAID_MEMORY_REQUEST_MAX_NUM_DETAILS
 *********************************************************************
 * @brief This is the fixed number of details we currently allow.
 *        We can allocate 4 different SG types plus buffers in one request.
 *
 *********************************************************************/
#define FBE_RAID_MEMORY_REQUEST_MAX_NUM_DETAILS 5

/*!*******************************************************************
 * @enum fbe_raid_memory_type_t
 *********************************************************************
 * @brief This is the reference to the kind of memory we allocated.
 *        Has everything we need to determine where to free the memory to.
 *
 *********************************************************************/
typedef enum fbe_raid_memory_type_e
{
    FBE_RAID_MEMORY_TYPE_INVALID = 0,
    FBE_RAID_MEMORY_TYPE_FRUTS,
    FBE_RAID_MEMORY_TYPE_SIOTS,
    FBE_RAID_MEMORY_TYPE_VRTS,
    FBE_RAID_MEMORY_TYPE_VCTS,
    FBE_RAID_MEMORY_TYPE_SG_1,
    FBE_RAID_MEMORY_TYPE_SG_8,
    FBE_RAID_MEMORY_TYPE_SG_32,
    FBE_RAID_MEMORY_TYPE_SG_128,
    FBE_RAID_MEMORY_TYPE_SG_MAX,
    FBE_RAID_MEMORY_TYPE_BUFFERS_WITH_SG,
    FBE_RAID_MEMORY_TYPE_BUFFERS_NO_SG,
    FBE_RAID_MEMORY_TYPE_2KB_BUFFER,
    FBE_RAID_MEMORY_TYPE_32KB_BUFFER,
    FBE_RAID_MEMORY_TYPE_MAX
}
fbe_raid_memory_type_t;

/*!*******************************************************************
 * @enum fbe_raid_memory_priority_t
 *********************************************************************
 * @brief This is the priority the memory is being requested at
 *
 *********************************************************************/
typedef enum fbe_raid_memory_priority_e
{
    FBE_RAID_MEMORY_PRIORITY_INVALID      = -1, /*! FBE_MEMORY_REQUEST_PRIORITY_LEVEL_FOR_CLASS_INVALID */
    FBE_RAID_MEMORY_PRIORITY_IOTS         =  0, /*! FBE_MEMORY_REQUEST_PRIORITY_LEVEL_FOR_CLASS_LOW */
    FBE_RAID_MEMORY_PRIORITY_SIOTS        =  1, /*! FBE_MEMORY_REQUEST_PRIORITY_LEVEL_FOR_CLASS_NORMAL */ 
    FBE_RAID_MEMORY_PRIORITY_NESTED_SIOTS =  2, /*! FBE_MEMORY_REQUEST_PRIORITY_LEVEL_FOR_CLASS_HIGH */
    FBE_RAID_MEMORY_PRIORITY_LAST
}
fbe_raid_memory_priority_t;

/*!*******************************************************************
 * @struct fbe_raid_memory_header_t
 *********************************************************************
 * @brief This is the header at the top of every raid allocation.
 *
 *********************************************************************/
typedef struct fbe_raid_memory_header_s
{
    fbe_queue_element_t queue_element;

    /*! This is the type of memory we allocated.
     */
    fbe_raid_memory_type_t type;

    /*! This magic number allows us to catch sg/buffer/ts overruns.
     */
    fbe_u32_t magic_number;
}
fbe_raid_memory_header_t;


/*!*******************************************************************
 * @struct  fbe_raid_memory_element_t
 *********************************************************************
 * @brief   Since raid use the memory service request to access the
 *          allocated chunks, we abstract the `special' element links
 *          used by the memory service so that if their definition changes
 *          we only need to change the definition below.
 * 
 * @note    See definition accessors for fbe_memory_hptr_t
 *
 *********************************************************************/
#define fbe_raid_memory_element_t   fbe_memory_hptr_t


/*!*******************************************************************
 * @struct fbe_raid_memory_info_t
 *********************************************************************
 * @brief This structure is use to consolidate information about a raid
 *        memory request.  All the values are only valid for the life
 *        of the I/O they are associated with.
 * 
 * @note  See accessors below
 *
 *********************************************************************/
typedef struct fbe_raid_memory_info_s
{
    /*! This is the generated queue of requested pages, chunks 
     */
    fbe_memory_request_t       *memory_request_p;

    /*! This is the generated queue of requested pages, chunks 
     */
    fbe_raid_memory_element_t  *chunk_queue_head_p;

    /*! Pointer to current chunk's queue element links
     */
    fbe_raid_memory_element_t  *current_chunk_element_p;

    /*! Number of bytes remaining in current current chunk
     */
    fbe_u32_t                   bytes_remaining_in_current_chunk;

    /*! Chunk size in bytes 
     */
    fbe_u32_t                   chunk_size_in_bytes;

    /*! Pointer to last element for buffers
     */
    fbe_raid_memory_element_t  *buffer_last_element_p;

    /*! Number of bytes remaining in last buffer chunk
     */
    fbe_u32_t                   bytes_remaining_in_last_buffer_chunk;
}
fbe_raid_memory_info_t;



#define FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__ 
#define FBE_RAID_MEMORY_INFO_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__

/* Accessors for memory request info
 */
static __inline void fbe_raid_memory_info_set_memory_request(fbe_raid_memory_info_t *memory_info_p,
                                                             fbe_memory_request_t *memory_request_p)
{
    memory_info_p->memory_request_p = memory_request_p;
    return;
}
static __inline void fbe_raid_memory_info_set_queue_head(fbe_raid_memory_info_t *memory_info_p,
                                                         fbe_raid_memory_element_t *queue_head_p)
{
    memory_info_p->chunk_queue_head_p = queue_head_p;
    return;
}

static __inline void fbe_raid_memory_info_set_current_element(fbe_raid_memory_info_t *memory_info_p, 
                                                              fbe_raid_memory_element_t *queue_element_p)
{
    memory_info_p->current_chunk_element_p = queue_element_p;
    return;
}

static __inline void fbe_raid_memory_info_set_bytes_remaining_in_page(fbe_raid_memory_info_t *memory_info_p,
                                                                      fbe_u32_t bytes_remaining_in_chunk)
{
    memory_info_p->bytes_remaining_in_current_chunk = bytes_remaining_in_chunk;
    return;
}

static __inline void fbe_raid_memory_info_set_page_size_in_bytes(fbe_raid_memory_info_t *memory_info_p,
                                                                 fbe_u32_t chunk_size_in_bytes)
{
    memory_info_p->chunk_size_in_bytes = chunk_size_in_bytes;
    return;
}

static __inline void fbe_raid_memory_info_set_buffer_end_element(fbe_raid_memory_info_t *memory_info_p,
                                                                 fbe_raid_memory_element_t *queue_element_p)
{
    memory_info_p->buffer_last_element_p = queue_element_p;
    return;
}

static __inline void fbe_raid_memory_info_set_buffer_end_bytes_remaining(fbe_raid_memory_info_t *memory_info_p,
                                                                         fbe_u32_t bytes_remaining_in_chunk)
{
    memory_info_p->bytes_remaining_in_last_buffer_chunk = bytes_remaining_in_chunk;
}

static __inline fbe_memory_request_t *fbe_raid_memory_info_get_memory_request(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->memory_request_p);
}

static __inline fbe_raid_memory_element_t *fbe_raid_memory_info_get_queue_head(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->chunk_queue_head_p);
}

static __inline fbe_memory_header_t *fbe_raid_memory_info_get_master_header(fbe_raid_memory_info_t *memory_info_p)
{
    return((fbe_memory_header_t *)memory_info_p->chunk_queue_head_p->master_header);
}

static __inline fbe_raid_memory_element_t *fbe_raid_memory_info_get_current_element(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->current_chunk_element_p);
}

static __inline fbe_raid_memory_element_t *fbe_raid_memory_info_get_next_element(fbe_raid_memory_info_t *memory_info_p)
{
    return(fbe_memory_get_next_memory_element(memory_info_p->current_chunk_element_p));
}

static __inline fbe_u32_t fbe_raid_memory_info_get_bytes_remaining_in_page(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->bytes_remaining_in_current_chunk);
}

static __inline fbe_u32_t fbe_raid_memory_info_get_page_size_in_bytes(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->chunk_size_in_bytes);
}

static __inline fbe_raid_memory_element_t *fbe_raid_memory_info_get_buffer_end_element(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->buffer_last_element_p);
}

static __inline fbe_u32_t fbe_raid_memory_info_get_buffer_end_bytes_remaining(fbe_raid_memory_info_t *memory_info_p)
{
    return(memory_info_p->bytes_remaining_in_last_buffer_chunk);
}

static __inline fbe_bool_t fbe_raid_memory_info_is_more_pages(fbe_raid_memory_info_t *memory_info_p)
{
    if (memory_info_p->current_chunk_element_p->next_header == NULL)
    {
        return(FBE_FALSE);
    }
    return(FBE_TRUE);
}

static __inline fbe_bool_t fbe_raid_memory_info_is_memory_left(fbe_raid_memory_info_t *memory_info_p)
{
    if ((fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p) == 0) &&
        (memory_info_p->current_chunk_element_p->next_header == NULL)             )
    {
        return(FBE_FALSE);
    }
    return(FBE_TRUE);
}

static __inline void fbe_raid_memory_info_set_current_to_buffer_start(fbe_raid_memory_info_t *memory_info_p)
{
    memory_info_p->current_chunk_element_p = fbe_raid_memory_info_get_queue_head(memory_info_p);
    memory_info_p->bytes_remaining_in_current_chunk = memory_info_p->chunk_size_in_bytes;
    return;
}

static __inline fbe_raid_siots_t *fbe_raid_memory_info_get_siots(fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_siots_t       *siots_p = NULL;
    fbe_memory_request_t   *memory_request_p = fbe_raid_memory_info_get_memory_request(memory_info_p);
    
    if (memory_request_p != NULL)
    {
	    siots_p = (fbe_raid_siots_t *)((fbe_u8_t *)memory_request_p - (fbe_u8_t *)(&((fbe_raid_siots_t  *)0)->memory_request));
    }

    return(siots_p);
}

static __forceinline fbe_status_t fbe_raid_memory_request_get_master_element(fbe_memory_request_t *memory_request_p,
                                                                             fbe_memory_hptr_t **master_pp)
{
    fbe_memory_header_t *master_memory_header_p;

    master_memory_header_p = (fbe_memory_header_t *)memory_request_p->ptr;
    if (master_memory_header_p == NULL)
    {
        *master_pp = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *master_pp = &master_memory_header_p->u.hptr;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t fbe_raid_memory_request_get_master_data_element(fbe_memory_request_t *memory_request_p,
                                                                                  fbe_memory_hptr_t **master_pp)
{
    fbe_memory_header_t *master_memory_header_p;

    master_memory_header_p = (fbe_memory_header_t *)memory_request_p->data_ptr;
    if (master_memory_header_p == NULL)
    {
        *master_pp = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *master_pp = &master_memory_header_p->u.hptr;
    return FBE_STATUS_OK;
}

static __forceinline void fbe_raid_memory_request_increment_priority(fbe_memory_request_t *memory_request_p)
{
    memory_request_p->priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    return;
}

fbe_raid_memory_type_t fbe_raid_memory_get_sg_type_by_index(fbe_raid_sg_index_t sg_index);
fbe_u32_t fbe_raid_memory_sg_index_to_max_count(fbe_raid_sg_index_t sg_index);
fbe_raid_sg_index_t fbe_raid_memory_get_index_by_sg_type(fbe_raid_memory_type_t type);
fbe_u32_t fbe_raid_memory_type_get_size(fbe_raid_memory_type_t type);
fbe_memory_chunk_size_t fbe_raid_memory_size_to_dps_size(fbe_u32_t page_size);
fbe_raid_memory_type_t fbe_raid_memory_dps_size_to_raid_type(fbe_memory_chunk_size_t page_size);
void fbe_raid_memory_header_init(fbe_raid_memory_header_t *header_p,
                                 fbe_raid_memory_type_t type);

fbe_bool_t fbe_raid_memory_header_is_valid(fbe_raid_memory_header_t *header_p);

/********************************* 
 * fbe_raid_memory_api.c
 *********************************/
fbe_status_t fbe_raid_memory_api_allocate(fbe_memory_request_t *memory_request_p, 
                                          fbe_bool_t b_memory_tracking_enabled);
fbe_status_t fbe_raid_memory_api_allocation_complete(fbe_memory_request_t *memory_request_p,
                                                     fbe_bool_t b_track_deferred_allocation);
void fbe_raid_memory_api_deferred_allocation(void);
void fbe_raid_memory_api_aborted_allocation(void);
void fbe_raid_memory_api_pending_allocation(void);
void fbe_raid_memory_api_allocation_error(void);
fbe_status_t fbe_raid_memory_api_free(fbe_memory_request_t *memory_request_p,
                                      fbe_bool_t b_memory_tracking_enabled);
fbe_status_t fbe_raid_memory_api_abort_request(fbe_memory_request_t *memory_request_p);
void fbe_raid_memory_api_free_error(void);
void fbe_raid_memory_get_zero_sg(fbe_sg_element_t **sg_p);
fbe_bool_t fbe_raid_memory_api_is_initialized(void);
fbe_status_t fbe_raid_memory_initialize(void);
fbe_status_t fbe_raid_memory_destroy(void);

/********************************* 
 * fbe_raid_memory_mock.c        *
 *********************************/
typedef void (*fbe_raid_memory_mock_error_callback_t)(void);
typedef fbe_status_t (*fbe_raid_memory_mock_allocate_mock_t)(fbe_memory_request_t *memory_request_p);
typedef fbe_status_t (*fbe_raid_memory_mock_abort_mock_t)(fbe_memory_request_t *memory_request_p);
typedef fbe_status_t (*fbe_raid_memory_mock_free_mock_t)(fbe_memory_ptr_t memory_p);
fbe_status_t fbe_raid_memory_mock_configure_mocks(fbe_bool_t b_enable_mock_memory,
                                                  fbe_bool_t b_allow_deferred_allocations,
                                                  fbe_raid_memory_mock_allocate_mock_t mock_allocate,
                                                  fbe_raid_memory_mock_abort_mock_t mock_abort,
                                                  fbe_raid_memory_mock_free_mock_t mock_free,
                                                  fbe_raid_memory_mock_error_callback_t error_callback);
fbe_status_t fbe_raid_memory_mock_api_allocate(fbe_memory_request_t *memory_request_p);
fbe_status_t fbe_raid_memory_mock_api_abort(fbe_memory_request_t *memory_request_p);
fbe_status_t fbe_raid_memory_mock_api_free(fbe_memory_header_t *master_memory_header_p);
fbe_bool_t fbe_raid_memory_mock_is_mock_enabled(void);
void fbe_raid_memory_mock_setup_mocks(void);
void fbe_raid_memory_mock_destroy_mocks(void);
void fbe_raid_memory_mock_change_defer_delay(fbe_bool_t b_quiesced);
void fbe_raid_memory_mock_validate_memory_leaks(void);

/********************************** 
 * fbe_raid_memory_track.c        *
 **********************************/
void fbe_raid_memory_track_initialize(void);
void fbe_raid_memory_track_allocation(fbe_memory_request_t *const memory_request_p);
void fbe_raid_memory_track_deferred_allocation(fbe_memory_request_t *const memory_request_p);
void fbe_raid_memory_track_free(fbe_memory_header_t *const master_memory_header_p);
void fbe_raid_memory_track_remove_aborted_request(fbe_memory_request_t *const memory_request_p);
void fbe_raid_memory_track_destroy(void);
fbe_atomic_t fbe_raid_memory_track_get_outstanding_allocations(void);

/*********************************
 * end file fbe_raid_memory_api.h
 ********************************/

#endif /* end __FBE_RAID_MEMORY_API_H__ */



