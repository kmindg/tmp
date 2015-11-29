#ifndef BGSL_PAYLOAD_H
#define BGSL_PAYLOAD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bgsl_payload.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the bgsl_payload_t structure and the
 *  related structures, enums and definitions.
 * 
 *  The @ref bgsl_payload_t contains the functional transport specific payloads
 *  such as
 *  @ref bgsl_payload_block_operation_t
 * 
 *
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/bgsl_types.h"
#include "fbe/bgsl_payload_operation_header.h"
#include "fbe/bgsl_payload_control_operation.h"


/*! @defgroup bgsl_payload_interfaces  FBE Payload
 *  
 *  This is the set of definitions that make up the FBE Payload.
 *  
 *  The FBE payload is the portion of the FBE packet that is used by
 *  a functional transport or control packet to transfer operation specific
 *  information from one object or service to another.
 *  
 *  @ingroup bgsl_transport_interfaces
 * @{
 */ 
typedef bgsl_u32_t bgsl_payload_revision_t;


/*! @enum bgsl_payload_constants_e  
 *  @brief These are constants specific to the bgsl_payload.
 */
enum bgsl_payload_constants_e{
    /*! This is the number of block operations that are 
     *  allowed in the payload. 
     */
	BGSL_PAYLOAD_MAX_BLOCK_OPERATIONS = 2,
	BGSL_PAYLOAD_MAX_CONTROL_OPERATIONS = 2
};

/* Temporary definition of physical drive transaction structure */
typedef struct bgsl_payload_physical_drive_transaction_s {
	bgsl_u32_t retry_count;
	bgsl_u32_t transaction_state;
	bgsl_u32_t last_error_code;
}bgsl_payload_physical_drive_transaction_t;

/*! @enum bgsl_payload_sg_index_t  
 *  
 *  @brief This is the set of possible sg lists in the payload.
 *         We represent this as an index to make it easier for
 *         clients to loop over the set of sg lists.
 */
typedef enum bgsl_payload_sg_index_e 
{
    /*! Not a valid index. 
     */
    BGSL_PAYLOAD_SG_INDEX_INVALID, 
    /*! The index for the first sg list represented by  
     *  bgsl_payload_t->pre_sg_array
     */
    BGSL_PAYLOAD_SG_INDEX_PRE_SG_LIST,
    /*! The index for the main sg list represented by  
     *  bgsl_payload_t->sg_list
     */ 
    BGSL_PAYLOAD_SG_INDEX_SG_LIST, 
    /*! The index for the last sg list represented by  
     *  bgsl_payload_t->post_sg_array
     */
    BGSL_PAYLOAD_SG_INDEX_POST_SG_LIST
} 
bgsl_payload_sg_index_t;

/*! @enum BGSL_PAYLOAD_SG_ARRAY_LENGTH  
 *  
 *  @brief This is the max number of sg entries in the
 *         pre and post sgs which are embedded in the payload.
 */
enum { BGSL_PAYLOAD_SG_ARRAY_LENGTH = 8};


enum bgsl_payload_flags_e{
	BGSL_PAYLOAD_FLAG_NO_ATTRIB = 0x0,
	BGSL_PAYLOAD_FLAG_USE_PHYS_OFFSET = 0x00000001,
};

typedef bgsl_u32_t bgsl_payload_attr_t;

/*! @struct bgsl_payload_t 
 *  
 *  @brief The fbe payload contains a stack of payloads and allows the user
 *         of a payload to allocate allocate or release a payload for use
 *         at a given time.
 *  
 *         At any moment only one payload is valid, represented by the
 *         "current_operation".  The next operation which will be valid is
 *         identified by the next_operations (or NULL if there is no next
 *         operation).
 */
typedef struct bgsl_payload_s {

    /* bgsl_payload_revision_t revision; */

    /*! This points to one of the payloads listed below, which are currently in 
     *  use.  
     */
	bgsl_payload_operation_header_t * current_operation;

    /*! This points the the next valid payload from one of the below payloads. 
     */
	bgsl_payload_operation_header_t * next_operation;

    bgsl_payload_control_operation_t control_operation[BGSL_PAYLOAD_MAX_CONTROL_OPERATIONS];

	bgsl_u8_t opaque[256];
} bgsl_payload_t;

/*! @typedef bgsl_payload_completion_context_t 
 *  
 *  @brief This is the typedef for the completion context that is used in
 *         the @ref bgsl_payload_completion_function_t.
 */
typedef void * bgsl_payload_completion_context_t;

/*! @typedef bgsl_payload_completion_function_t 
 *  
 *  @brief This is the typedef for the completion function that runs when the
 *         payload is done.
 */
typedef bgsl_status_t (* bgsl_payload_completion_function_t) (bgsl_payload_t * payload, bgsl_payload_completion_context_t context);

bgsl_status_t bgsl_payload_init(bgsl_payload_t * payload);

bgsl_payload_control_operation_t * bgsl_payload_allocate_control_operation(bgsl_payload_t * payload);
bgsl_status_t bgsl_payload_release_control_operation(bgsl_payload_t * payload, bgsl_payload_control_operation_t * payload_control_operation);
bgsl_payload_control_operation_t * bgsl_payload_get_control_operation(bgsl_payload_t * payload);
bgsl_status_t bgsl_payload_increment_control_operation_level(bgsl_payload_t * payload);
bgsl_payload_control_operation_t * bgsl_payload_get_prev_control_operation(bgsl_payload_t * payload);


/*! @} */ /* end of group bgsl_payload_interfaces */

#endif /* BGSL_PAYLOAD_H */
