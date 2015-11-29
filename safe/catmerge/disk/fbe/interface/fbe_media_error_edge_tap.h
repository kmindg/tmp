/***************************************************************************   
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_media_error_edge_tap.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the media error edge tap definitions.
 *
 * HISTORY
 *   1/12/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*! @enum fbe_error_record_type_t 
 *  @brief This is the type of error we are injecting.
 */
typedef enum fbe_error_record_type_s
{
    FBE_ERROR_RECORD_TYPE_UNINITIALIZED,
    FBE_ERROR_RECORD_TYPE_MEDIA_ERROR,
    FBE_ERROR_RECORD_TYPE_ALL_ERRORS,
}
fbe_error_record_type_t;

/*! @struct fbe_media_edge_tap_info_t  
 *  
 *  @brief This is the structure containing information
 *         about the operation we are receiving.
 *         This allows us to abstract away the details of
 *         the operation we are receiving from the
 *         format of those details (CDB, FIS, etc.)
 */
typedef struct fbe_media_edge_tap_info_s
{
    /*! Logical Block Address
     */
    fbe_lba_t lba;

    /*! Number of blocks in transfer.
     */
    fbe_block_count_t blocks;

    /*! Type of operation.  Read, Write, reassign, etc.
     */
    fbe_u8_t opcode;

    /*! Ptr to sense info. 
     */
    fbe_u8_t *sense_buffer_p;
}
fbe_media_edge_tap_info_t;

/*! @struct fbe_error_record_t  
 *  @brief This is the details of the error record that represents
 *         per block details for errors we are injecting.
 */
typedef struct fbe_error_record_s
{
    /*! Lba to inject the error.
     */
    fbe_lba_t lba;

    /*! The kind of error to inject.
     */
    fbe_error_record_type_t error_type;

    /*! The number of times this has been injected. 
     */
    fbe_u32_t injected_count;

    /*! TRUE means we are injecting this error 
     * FALSE means we are not injecting this error. 
     */
    fbe_bool_t b_active;
}
fbe_error_record_t;

/*! @enum FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS  
 *  @brief This is the max error records we will
 *         allow to be used with the edge tap.
 */
enum {FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS = 256};

/*! @struct fbe_media_error_edge_tap_object_t 
 *  @brief This is the edge tap object.
 */
typedef struct fbe_media_error_edge_tap_object_s
{
    /* All the errors we will be injecting
     * This is limited to 
     */
    fbe_error_record_t records[FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS];

    /*! Number of packets seen by the tap.
     */
    fbe_u32_t packet_received_count;
}
fbe_media_error_edge_tap_object_t;

/*! @struct fbe_api_edge_tap_create_info_t
 *  @brief This structure is used to create an edge tap.
 *         It contains all the information the caller needs to
 *         establish the edge tap.  
 */
typedef struct fbe_api_edge_tap_create_info_s
{
    /*! Ptr to to the edge tap object itself.
     *  The caller can use this to call functions that  
     *  manipulate the edge tap.  
     */
    fbe_media_error_edge_tap_object_t *edge_tap_object_p;

    /*! This is a pointer to the edge tap function. 
     *  This is the function pointer that gets put
     *  into the edge.
     */
    fbe_status_t (*edge_tap_fn)(fbe_packet_t *);
}
fbe_api_edge_tap_create_info_t;

/*! @struct fbe_media_error_edge_tap_stats_t 
 *  @brief This is the interface for retrieving the
 *         set of edge tap statistics.
 */
typedef struct fbe_media_error_edge_tap_stats_s
{
    /*! The number of records in the following array.
     */
    fbe_u32_t count;
    /*! Array of records beginning with this location. 
     *  This is filled in by the server. 
     */
    fbe_error_record_t *record_p;

    /*! Number of packets seen by the tap.
     */
    fbe_u32_t packet_count;
}
fbe_media_error_edge_tap_stats_t;

/*************************
 * API Functions
 *************************/

fbe_status_t fbe_media_error_edge_tap_object_create(fbe_api_edge_tap_create_info_t *info_p);
fbe_error_record_t * fbe_media_error_edge_tap_object_find_overlap(
    fbe_media_error_edge_tap_object_t *const edge_tap_p,
    const fbe_lba_t lba,
    const fbe_block_count_t blocks,
    fbe_error_record_type_t error);
fbe_status_t fbe_media_error_edge_tap_object_add(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                 const fbe_lba_t lba,
                                                 const fbe_block_count_t blocks,
                                                 fbe_error_record_type_t error);
fbe_status_t fbe_media_error_edge_tap_object_clear(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                   const fbe_lba_t lba,
                                                   const fbe_block_count_t blocks);
fbe_status_t fbe_media_error_edge_tap_object_get_stats(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                       fbe_media_error_edge_tap_stats_t *const stats_p);
void fbe_media_edge_tap_unit_test(void);
fbe_status_t fbe_media_error_edge_tap_object_is_active(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                       const fbe_lba_t lba,
                                                       const fbe_block_count_t blocks,
                                                       fbe_error_record_type_t error);

/*************************
 * end file fbe_media_error_edge_tap.h
 *************************/


