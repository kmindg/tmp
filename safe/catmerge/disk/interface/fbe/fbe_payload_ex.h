#ifndef FBE_PAYLOAD_EX_H
#define FBE_PAYLOAD_EX_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_payload_ex.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the fbe_payload_ex_t structure and the
 *  related structures, enums and definitions.
 * 
 *  The @ref fbe_payload_ex_t contains the functional transport specific payloads
 *  such as
 *  @ref fbe_payload_ex_block_operation_t
 * 
 *
 * @version
 *   09/03/2011:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe/fbe_payload_discovery_operation.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_payload_dmrb_operation.h"
#include "fbe/fbe_payload_smp_operation.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_raid_ts.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe/fbe_payload_stripe_lock_operation.h"
#include "fbe/fbe_payload_persistent_memory_operation.h"


/*! @defgroup fbe_payload_ex_interfaces  FBE Payload
 *  
 *  This is the set of definitions that make up the FBE Payload.
 *  
 *  The FBE payload is the portion of the FBE packet that is used by
 *  a functional transport or control packet to transfer operation specific
 *  information from one object or service to another.
 *  
 *  @ingroup fbe_transport_interfaces
 * @{
 */ 


/*! @enum fbe_payload_ex_constants_e  
 *  @brief These are constants specific to the fbe_payload.
 */
enum fbe_payload_ex_constants_e{
    /*! This is the number of block operations that are 
     *  allowed in the payload. 
     */
    FBE_PAYLOAD_EX_MEMORY_SIZE = 1536,
};


/* Temporary definition of physical drive transaction structure */
/*TODO: this should be moved to cdb_payload.  When it is, you will need to be careful since
  the current implementation releases and re-allocates a new cdb_payload on retries. -wayne*/
typedef struct fbe_payload_physical_drive_transaction_s {
    fbe_u32_t retry_count;
    fbe_u32_t transaction_state;
    fbe_u32_t last_error_code;
}fbe_payload_physical_drive_transaction_t;

/*! @enum fbe_payload_sg_index_t  
 *  
 *  @brief This is the set of possible sg lists in the payload.
 *         We represent this as an index to make it easier for
 *         clients to loop over the set of sg lists.
 */
typedef enum fbe_payload_sg_index_e 
{
    /*! Not a valid index. 
     */
    FBE_PAYLOAD_SG_INDEX_INVALID, 
    /*! The index for the first sg list represented by  
     *  fbe_payload_t->pre_sg_array
     */
    FBE_PAYLOAD_SG_INDEX_PRE_SG_LIST,
    /*! The index for the main sg list represented by  
     *  fbe_payload_t->sg_list
     */ 
    FBE_PAYLOAD_SG_INDEX_SG_LIST, 
    /*! The index for the last sg list represented by  
     *  fbe_payload_t->post_sg_array
     */
    FBE_PAYLOAD_SG_INDEX_POST_SG_LIST
} fbe_payload_sg_index_t;

/*! @enum FBE_PAYLOAD_SG_ARRAY_LENGTH  
 *  
 *  @brief This is the max number of sg entries in the
 *         pre and post sgs which are embedded in the payload.
 */
enum { FBE_PAYLOAD_SG_ARRAY_LENGTH = 2};


enum fbe_payload_flags_e{
    FBE_PAYLOAD_FLAG_NO_ATTRIB = 0x0,
    FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET = 0x00000001,
};

typedef fbe_u32_t fbe_payload_attr_t;


typedef struct fbe_payload_memory_operation_s{
    fbe_payload_operation_header_t          operation_header; /* Must be first */
	fbe_memory_io_master_t memory_io_master;
}fbe_payload_memory_operation_t;

/*! @enum fbe_payload_buffer_operation_constants_e  
 *  @brief These are constants specific to the buffer payload
 */
enum fbe_payload_buffer_operation_constants_e{
    /*! This is the maximum number of bytes for a buffer payload
     */
    FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE = 128,
};
typedef struct fbe_payload_buffer_operation_s{
    fbe_payload_operation_header_t          operation_header; /* Must be first */
	fbe_u8_t buffer[FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE];
}fbe_payload_buffer_operation_t;

/*! @struct fbe_payload_ex_t 
 *  
 *  @brief The fbe storage extent
 *         payload contains a stack of payloads and allows the user
 *         of a payload to allocate allocate or release a payload for use
 *         at a given time.
 *  
 *         At any moment only one payload is valid, represented by the
 *         "current_operation".  The next operation which will be valid is
 *         identified by the next_operations (or NULL if there is no next
 *         operation).
 */
//#pragma pack(1)
typedef struct fbe_payload_ex_s {

    /**************************************************************************** 
     * We currently keep the current operation, next operation, sg_list and
     * sg_list_count control operations at the head of this structure so that it 
     * is in sync with the fbe_payload_ex_t. 
     ****************************************************************************/

    /*! This points to one of the payloads listed below, which are currently in use. */
    fbe_payload_operation_header_t * current_operation;

    /*! This points the the next valid payload from one of the below payloads. */
    fbe_payload_operation_header_t * next_operation;

    /*! This is the pointer to the data the client is sending with this payload. 
     * For example, if it were a write operation it is the pointer to the sg 
     * list that contains write data. 
     */
    fbe_sg_element_t *  sg_list;       


	fbe_u8_t operation_memory[FBE_PAYLOAD_EX_MEMORY_SIZE]; /* All operations allocated from here */

    /* In order for the raid levels to not allocate these resources, 
     * we embed them inside the payload. 
     */
	/* Will be dinamically allocated soon */
    FBE_ALIGN(8)fbe_raid_iots_t  iots; 
    fbe_raid_siots_t siots;
	
	/* This is to make fbe_sep_payload_get_iots function happy */
    //fbe_raid_iots_t  * iots_ptr; 
    //fbe_raid_siots_t * siots_ptr;


    fbe_u32_t sg_list_count; /*!< number of elements on the list */
	fbe_u32_t operation_memory_index; /* Index to the free memory */

	/* From Physical payload */
    fbe_payload_attr_t payload_attr;
    fbe_u64_t physical_offset;

    /*! This is an embedded sg list which is used to 
     *  contain the first data.  This sg list should be
     *  traversed first.  The sg list to be traversed
     *  next would be sg_list.
     */
    fbe_sg_element_t pre_sg_array[FBE_PAYLOAD_SG_ARRAY_LENGTH];

    /*! This is the embedded sg list which has that last bit of data. 
     *  This sg list should be traversed after the sg_list. 
     */
    fbe_sg_element_t post_sg_array[FBE_PAYLOAD_SG_ARRAY_LENGTH];

    /* Temoprary transaction structure */
    fbe_payload_physical_drive_transaction_t physical_drive_transaction;

	/*! This is the lba where a media error occurred. 
     *  We only expect this to be valid for cases where a media error occurs
     *  With status of FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
     *  or with the qualifier of
     *  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP or
     *  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED.
     */
    fbe_lba_t media_error_lba;

    /*! When a FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE  
     *  error is received this field contains the number of milliseconds
     *  the client should wait before retrying.
     */
    fbe_time_t retry_wait_msecs;


	/* START :STILL NEEDED TO KEEP FLARE ALIVE AND ENABLE RETRIES, DO NOT REMOVE UNTIL REMOVING FLARE*/
    /*! This is a temporary addition to let the upper DH know what was the SCSI code of the 
	 * IO we need to retry, it should be removed from here once PDO does all the recovery (sharel).  
     */
	fbe_scsi_error_code_t	scsi_error_code;

	/*! This is a temporary addition to place the sense buffer of the error in one word
	this should be done by the drive object and consumed by the upper DH to be placed in the cm logs (sharel) 
     */
	fbe_u32_t				sense_data;
	/* END :STILL NEEDED TO KEEP FLARE ALIVE AND ENABLE RETRIES, DO NOT REMOVE UNTIL REMOVING FLARE*/

	/*! This is a object id of the physical drive to which the IO request is sent. */
    fbe_object_id_t         pvd_object_id;

    /*! Pointer to the verify error counts. */
    void        * verify_error_counts_p;

    /*! Pointer to the lun performance statistic structure. */
    void        * performace_stats_p;

	fbe_payload_memory_operation_t * payload_memory_operation; /* Helps to keep track of I/O related memory allocations */

    fbe_time_t  start_time; /*! Time used by LUN to determine total response time. */

    /*It can be a pointer also once actual DEK in place*/
    fbe_key_handle_t   key_handle;
} fbe_payload_ex_t;
//#pragma pack()


/*! @typedef fbe_payload_ex_completion_context_t 
 *  
 *  @brief This is the typedef for the completion context that is used in
 *         the @ref fbe_payload_ex_completion_function_t.
 */
typedef void * fbe_payload_ex_completion_context_t;

/*! @typedef fbe_payload_ex_completion_function_t 
 *  
 *  @brief This is the typedef for the completion function that runs when the
 *         payload is done.
 */
typedef fbe_status_t (* fbe_payload_ex_completion_function_t) (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);

/*!**************************************************************
 * @fn fbe_payload_ex_get_pre_sg_list(fbe_payload_ex_t *payload,
 *                             fbe_sg_element_t ** sg_list)
 ****************************************************************
 * @brief
 *  This function returns the pre sg list field from the payload
 *  structure.  This is the sg list that should be traversed first
 *  in the payload.
 *
 * @param payload - The payload to init.
 * @param sg_list - Ptr to the sg list ptr to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_pre_sg_list(fbe_payload_ex_t * payload, fbe_sg_element_t ** sg_list)
{
    *sg_list = &payload->pre_sg_array[0];

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_payload_ex_get_pre_sg_list()
 **************************************/

/*!**************************************************************
 * @fn fbe_payload_ex_get_post_sg_list(fbe_payload_ex_t *payload,
 *                             fbe_sg_element_t ** sg_list)
 ****************************************************************
 * @brief
 *  This function returns the post sg list field from the payload
 *  structure.
 *  This is the sg list that should be traversed last of all
 *  the sg lists in the payload.
 *
 * @param payload - The payload to init.
 * @param sg_list - Ptr to the sg list ptr to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_post_sg_list(fbe_payload_ex_t * payload, fbe_sg_element_t ** sg_list)
{
    *sg_list = &payload->post_sg_array[0];

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_payload_ex_get_post_sg_list()
 **************************************/


/*!**************************************************************
 * @fn fbe_payload_ex_get_physical_offset(fbe_payload_ex_t *payload, 
 *                                     fbe_u64_t * physical_offset)
 ****************************************************************
 * @brief
 *  This function returns the physical_offset field from the payload
 *  structure.
 *  This is the physical_offset that should be used by miniport
 *  to calculate physical addresses.
 *
 * @param payload - The payload structure.
 * @param physical_offset - Ptr to the physical_offset to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_physical_offset(fbe_payload_ex_t *payload, fbe_u64_t * physical_offset)
{
    *physical_offset = payload->physical_offset;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_ex_set_physical_offset(fbe_payload_ex_t *payload, 
 *                                     fbe_u64_t physical_offset)
 ****************************************************************
 * @brief
 *  This function sets the physical_offset field of the payload
 *  structure.
 *  This is the physical_offset that should be used by miniport
 *  to calculate physical addresses.
 *
 * @param payload - The payload structure.
 * @param physical_offset - physical_offset to be set.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_set_physical_offset(fbe_payload_ex_t *payload, fbe_u64_t  physical_offset)
{
    payload->physical_offset = physical_offset;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_attr(fbe_payload_ex_t *payload, 
 *                          fbe_payload_attr_t * payload_attr)
 ****************************************************************
 * @brief
 *  This function returns the payload_attr field from the payload
 *  structure.
 *
 * @param payload - The payload structure.
 * @param payload_attr - Ptr to the payload_attr to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_attr(fbe_payload_ex_t *payload, fbe_payload_attr_t * payload_attr)
{
    *payload_attr = payload->payload_attr;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_ex_set_attr(fbe_payload_ex_t *payload, 
 *                          fbe_payload_attr_t  payload_attr)
 ****************************************************************
 * @brief
 *  This function sets the payload_attr field of the payload
 *  structure.
 *
 * @param payload - The payload structure.
 * @param payload_attr - payload_attr to be set.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t fbe_payload_ex_set_attr(fbe_payload_ex_t *payload, fbe_payload_attr_t  payload_attr)
{
    payload->payload_attr = payload_attr;
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * @fn fbe_payload_ex_set_sg_list(fbe_payload_ex_t *payload,
 *                             fbe_sg_element_t * sg_list,
 *                             fbe_u32_t sg_list_count)
 ****************************************************************
 * @brief
 *  This function sets the sg list and sg list count fields in the payload.
 *
 * @param payload - The payload to init.
 * @param sg_list - the sg list to set into the payload.
 * @param sg_list_count - the sg list count value to set into the payload.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_set_sg_list(fbe_payload_ex_t * payload, fbe_sg_element_t * sg_list, fbe_u32_t sg_list_count)
{
    payload->sg_list = sg_list;
    payload->sg_list_count = sg_list_count;

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * @fn fbe_payload_ex_get_sg_list(fbe_payload_ex_t *payload,
 *                             fbe_sg_element_t ** sg_list,
 *                             fbe_u32_t *sg_list_count)
 ****************************************************************
 * @brief
 *  This function returns the sg list and the sg list count fields
 *  from the payload structure.
 *
 * @param payload - The payload to init.
 * @param sg_list - Ptr to the sg list ptr to return.
 * @param sg_list_count - Ptr to the sg list count value to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_sg_list(fbe_payload_ex_t * payload, fbe_sg_element_t ** sg_list, fbe_u32_t * sg_list_count)
{
    *sg_list = payload->sg_list;
    if(sg_list_count != NULL){
        *sg_list_count = payload->sg_list_count;
    }
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_ex_set_pvd_object_id(fbe_payload_ex_t * payload, fbe_object_id_t pvd_object_id) 
{
     payload->pvd_object_id = pvd_object_id;
     return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_ex_get_pvd_object_id(fbe_payload_ex_t * payload, fbe_object_id_t * pvd_object_id) 
{
     * pvd_object_id = payload->pvd_object_id;
     return FBE_STATUS_OK;
}


static __forceinline fbe_status_t  
fbe_payload_ex_set_scsi_error_code(fbe_payload_ex_t * payload, fbe_scsi_error_code_t scsi_error_code)
{
    payload->scsi_error_code = scsi_error_code;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t  
fbe_payload_ex_get_scsi_error_code(fbe_payload_ex_t * payload, fbe_scsi_error_code_t * scsi_error_code)
{
    *scsi_error_code = payload->scsi_error_code;
    return FBE_STATUS_OK;

}

static __forceinline fbe_status_t  
fbe_payload_ex_set_key_handle(fbe_payload_ex_t * payload, fbe_key_handle_t key_handle)
{
    payload->key_handle = key_handle;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t  
fbe_payload_ex_get_key_handle(fbe_payload_ex_t * payload, fbe_key_handle_t *key_handle)
{
    *key_handle = payload->key_handle;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t  
fbe_payload_ex_set_sense_data(fbe_payload_ex_t * payload, fbe_u32_t sense_data)
{
    payload->sense_data = sense_data;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t  
fbe_payload_ex_get_sense_data(fbe_payload_ex_t * payload, fbe_u32_t * sense_data)
{
    *sense_data = payload->sense_data;
    return FBE_STATUS_OK;

}

static __forceinline fbe_status_t 
fbe_payload_ex_get_retry_wait_msecs(fbe_payload_ex_t * payload, fbe_time_t *msecs)
{
     *msecs = payload->retry_wait_msecs;
     return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_ex_set_retry_wait_msecs(fbe_payload_ex_t * payload, fbe_time_t msecs)
{
     payload->retry_wait_msecs = msecs;
     return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_payload_ex_get_iots()
 ****************************************************************
 * @brief
 *  This function returns the ptr to the embedded iots.
 *
 * @param payload - The payload.
 * @param iots_pp - Ptr to the iots to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_iots(fbe_payload_ex_t * payload, fbe_raid_iots_t ** iots_pp)
{
    *iots_pp = &payload->iots;

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_payload_ex_get_siots()
 ****************************************************************
 * @brief
 *  This function returns the ptr to the embedded siots.
 *
 * @param payload - The payload.
 * @param siots_pp - Ptr to the siots to return.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_ex_get_siots(fbe_payload_ex_t * payload, fbe_raid_siots_t ** siots_pp)
{
    *siots_pp = &payload->siots;

    return FBE_STATUS_OK;
}

void fbe_base_transport_trace(fbe_trace_level_t trace_level,
                              fbe_u32_t message_id,
                              const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,3,4)));

/*!**************************************************************
 * @fn fbe_payload_ex_get_block_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a block operation.  If the current active payload is
 *  not a block operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_block_operation() was called to allocate the block
 *  operation, then fbe_payload_ex_increment_block_operation_level() was also
 *  called to make the current block operation "active".  Typically the sending
 *  of the block operation on the edge calls the increment, although if no edge
 *  is being used, then the client might need to call the increment function.
 *
 * @param payload - The payload to get the block operation for.
 *
 * @return fbe_payload_block_operation_t * - The current active payload, which
 *                                           should be a block operation.
 *         NULL is returned if the current active payload is not a block
 *         operation.
 *
 ****************************************************************/
static __forceinline fbe_payload_block_operation_t * 
fbe_payload_ex_get_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is block operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_block_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_present_block_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 * 	This function fetches the present block operation from the payload
 * 	structure.
 * 	It returns NULL if it does not find any block operation associated
 * 	with payload.
 * 
 * @param payload - The payload to get the block operation for.
 *
 * @return fbe_payload_block_operation_t * - The previous payload, which
 *                                           should be a block operation.
 * 		   NULL is returned if the payload has no associated block
 *		   operation.
 *
 ****************************************************************/
static __forceinline fbe_payload_block_operation_t * 
fbe_payload_ex_get_present_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    while(current_operation_header != NULL)
    {
        /* break the loop if the operation is block operation. */
        if(current_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
        {
            break;
        }

        /* get the next prevous operation. */
        current_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    }

    /* return the present block operation from payload */
    return (fbe_payload_block_operation_t *)current_operation_header;
}

static __forceinline fbe_payload_stripe_lock_operation_t * 
fbe_payload_ex_get_stripe_lock_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is lba_lock operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_stripe_lock_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_cdb_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a cdb operation.  If the current active payload is
 *  not a cdb operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_cdb_operation() was called to allocate the cdb
 *  operation, then fbe_payload_ex_increment_cdb_operation_level() was also
 *  called to make the current cdb operation "active".  Typically the sending
 *  of the cdb operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the cdb operation for.
 *
 * @return fbe_payload_cdb_operation_t * - The current active payload, which
 *                                           should be a cdb operation.
 *         NULL is returned if the current active payload is not a cdb
 *         operation.
 *
 ****************************************************************/
static __forceinline fbe_payload_cdb_operation_t * 
fbe_payload_ex_get_cdb_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        /* We will add error trace later
            For now we have to support both io_block and payload, so some packets may not have payload operation at all
        */
        return NULL;
    }

    /* Check if current operation is cdb operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CDB_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", 
                                 __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_cdb_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_dmrb_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a dmrb operation.  If the current active payload is
 *  not a dmrb operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_dmrb_operation() was called to allocate the dmrb
 *  operation, then fbe_payload_ex_increment_dmrb_operation_level() was also
 *  called to make the current dmrb operation "active".  Typically the sending
 *  of the dmrb operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the dmrb operation for.
 *
 * @return fbe_payload_dmrb_operation_t * - The current active payload, which
 *                                           should be a dmrb operation.
 *         NULL is returned if the current active payload is not a dmrb
 *         operation.
 *
 ****************************************************************/
static __forceinline fbe_payload_dmrb_operation_t * 
fbe_payload_ex_get_dmrb_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is dmrb operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DMRB_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_dmrb_operation_t *)payload->current_operation;
}
fbe_status_t fbe_payload_ex_init(fbe_payload_ex_t * payload);

fbe_payload_control_operation_t * fbe_payload_ex_allocate_control_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_control_operation(fbe_payload_ex_t * payload, fbe_payload_control_operation_t * payload_control_operation);
fbe_payload_control_operation_t * fbe_payload_ex_get_control_operation(fbe_payload_ex_t * payload);
fbe_payload_control_operation_t * fbe_payload_ex_get_present_control_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_control_operation_level(fbe_payload_ex_t * payload);
fbe_payload_control_operation_t * fbe_payload_ex_get_prev_control_operation(fbe_payload_ex_t * payload);
fbe_payload_control_operation_t * fbe_payload_ex_get_any_control_operation(fbe_payload_ex_t * payload);

fbe_payload_block_operation_t * fbe_payload_ex_allocate_block_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_block_operation(fbe_payload_ex_t * payload, fbe_payload_block_operation_t * payload_block_operation);
fbe_payload_block_operation_t * fbe_payload_ex_get_prev_block_operation(fbe_payload_ex_t * payload);
fbe_payload_block_operation_t * fbe_payload_ex_search_prev_block_operation(fbe_payload_ex_t * payload);
fbe_payload_block_operation_t * fbe_payload_ex_get_next_block_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_block_operation_level(fbe_payload_ex_t * payload);

fbe_payload_metadata_operation_t * fbe_payload_ex_allocate_metadata_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_metadata_operation(fbe_payload_ex_t * payload, fbe_payload_metadata_operation_t * payload_metadata_operation);
fbe_payload_metadata_operation_t * fbe_payload_ex_get_metadata_operation(fbe_payload_ex_t * payload);
fbe_payload_metadata_operation_t * fbe_payload_ex_get_prev_metadata_operation(fbe_payload_ex_t * payload);
fbe_payload_metadata_operation_t *fbe_payload_ex_get_any_metadata_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_metadata_operation_level(fbe_payload_ex_t * payload);
fbe_payload_metadata_operation_t * fbe_payload_ex_check_metadata_operation(fbe_payload_ex_t * payload);


fbe_payload_persistent_memory_operation_t * fbe_payload_ex_allocate_persistent_memory_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_persistent_memory_operation(fbe_payload_ex_t * payload, fbe_payload_persistent_memory_operation_t * payload_persistent_memory_operation);
fbe_payload_persistent_memory_operation_t * fbe_payload_ex_get_persistent_memory_operation(fbe_payload_ex_t * payload);
fbe_payload_persistent_memory_operation_t * fbe_payload_ex_get_prev_persistent_memory_operation(fbe_payload_ex_t * payload);
fbe_payload_persistent_memory_operation_t *fbe_payload_ex_get_any_persistent_memory_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_persistent_memory_operation_level(fbe_payload_ex_t * payload);

fbe_payload_stripe_lock_operation_t * fbe_payload_ex_allocate_stripe_lock_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_stripe_lock_operation(fbe_payload_ex_t * payload, fbe_payload_stripe_lock_operation_t * payload_stripe_lock_operation);

fbe_payload_stripe_lock_operation_t * fbe_payload_ex_get_prev_stripe_lock_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_stripe_lock_operation_level(fbe_payload_ex_t * payload);

fbe_payload_discovery_operation_t * fbe_payload_ex_allocate_discovery_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_discovery_operation(fbe_payload_ex_t * payload, fbe_payload_discovery_operation_t * payload_discovery_operation);
fbe_payload_discovery_operation_t * fbe_payload_ex_get_discovery_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_discovery_operation_level(fbe_payload_ex_t * payload);

fbe_payload_cdb_operation_t * fbe_payload_ex_allocate_cdb_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_cdb_operation(fbe_payload_ex_t * payload, fbe_payload_cdb_operation_t * payload_cdb_operation);

fbe_status_t fbe_payload_ex_increment_cdb_operation_level(fbe_payload_ex_t * payload);

fbe_payload_fis_operation_t * fbe_payload_ex_allocate_fis_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_fis_operation(fbe_payload_ex_t * payload, fbe_payload_fis_operation_t * payload_fis_operation);
fbe_payload_fis_operation_t * fbe_payload_ex_get_fis_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_fis_operation_level(fbe_payload_ex_t * payload);

fbe_payload_dmrb_operation_t * fbe_payload_ex_allocate_dmrb_operation(fbe_payload_ex_t * payload);

fbe_status_t fbe_payload_ex_release_dmrb_operation(fbe_payload_ex_t * payload, fbe_payload_dmrb_operation_t * payload_dmrb_operation);
fbe_status_t fbe_payload_ex_increment_dmrb_operation_level(fbe_payload_ex_t * payload);

fbe_payload_smp_operation_t * fbe_payload_ex_allocate_smp_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_smp_operation(fbe_payload_ex_t * payload, fbe_payload_smp_operation_t * payload_smp_operation);
fbe_payload_smp_operation_t * fbe_payload_ex_get_smp_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_increment_smp_operation_level(fbe_payload_ex_t * payload);

fbe_status_t fbe_payload_ex_get_current_opcode(fbe_payload_ex_t * payload, fbe_payload_opcode_t * opcode);

fbe_status_t fbe_payload_ex_get_media_error_lba(fbe_payload_ex_t * payload, fbe_lba_t * lba);
fbe_status_t fbe_payload_ex_set_media_error_lba(fbe_payload_ex_t * payload, fbe_lba_t lba);

fbe_status_t fbe_payload_ex_set_verify_error_count_ptr(fbe_payload_ex_t * payload, void *error_counts_p);
fbe_status_t fbe_payload_ex_get_verify_error_count_ptr(fbe_payload_ex_t * payload, void **error_counts_p);


fbe_payload_memory_operation_t * fbe_payload_ex_allocate_memory_operation(fbe_payload_ex_t * payload);
fbe_status_t fbe_payload_ex_release_memory_operation(fbe_payload_ex_t * payload, fbe_payload_memory_operation_t * memory_operation);

fbe_payload_buffer_operation_t * fbe_payload_ex_get_buffer_operation(fbe_payload_ex_t * payload);
fbe_payload_buffer_operation_t * fbe_payload_ex_get_any_buffer_operation(fbe_payload_ex_t * payload);
fbe_payload_buffer_operation_t * fbe_payload_ex_allocate_buffer_operation(fbe_payload_ex_t * payload, fbe_u32_t requested_buffer_size);
fbe_status_t fbe_payload_ex_release_buffer_operation(fbe_payload_ex_t * payload, fbe_payload_buffer_operation_t * payload_buffer_operation);

static __forceinline fbe_status_t 
fbe_payload_ex_set_lun_performace_stats_ptr(fbe_payload_ex_t * payload, void *lun_performace_stats_p)
{
    payload->performace_stats_p = lun_performace_stats_p;

    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_ex_get_lun_performace_stats_ptr(fbe_payload_ex_t * payload, void **lun_performace_stats_p)
{
    *lun_performace_stats_p = payload->performace_stats_p;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t fbe_payload_ex_set_start_time(fbe_payload_ex_t * payload, fbe_time_t time)
{
    payload->start_time = time;
    return FBE_STATUS_OK;
}
#endif /* fbe_payload_ex_H */
