#ifndef FBE_PAYLOAD_OPERATION_HEADER_H
#define FBE_PAYLOAD_OPERATION_HEADER_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_payload_operation_header.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the 
 *  fbe_payload_operation_header_t structure and the related structures, 
 *  enums and definitions.
 * 
 *  The @ref fbe_payload_operation_header_t allows us to treat many different
 *  payload types in a common way.  This allows the fbe_payload_ex_t to
 *  contain a stack of payloads.
 * 
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

/*! @defgroup fbe_payload_operation_header FBE Payload Operation Header 
 *  
 *  This is the set of definitions that make up the FBE Payload Operation
 *  Header.
 *  
 *  
 *  @ingroup fbe_payload_interfaces
 * @{
 */ 

/*! @enum fbe_payload_opcode_t  
 *  
 *  @brief This opcode identifies the type of payload this header is using.
 *  
 *  @see fbe_payload_operation_header_t
 */
typedef enum fbe_payload_opcode_e {
	FBE_PAYLOAD_OPCODE_INVALID,

	FBE_PAYLOAD_OPCODE_BLOCK_OPERATION,
	FBE_PAYLOAD_OPCODE_CONTROL_OPERATION,
	FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION,
	FBE_PAYLOAD_OPCODE_CDB_OPERATION,
	FBE_PAYLOAD_OPCODE_FIS_OPERATION,
	FBE_PAYLOAD_OPCODE_DMRB_OPERATION,
	FBE_PAYLOAD_OPCODE_SMP_OPERATION,
	FBE_PAYLOAD_OPCODE_DIPLEX_OPERATION,
	FBE_PAYLOAD_OPCODE_METADATA_OPERATION,
	FBE_PAYLOAD_OPCODE_MEMORY_OPERATION,
	FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION,
	FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION,
    FBE_PAYLOAD_OPCODE_BUFFER_OPERATION,

	FBE_PAYLOAD_OPCODE_LAST
}fbe_payload_opcode_t;

/*! @struct fbe_payload_operation_header_t  
 *  
 *  @brief This structure is always the first structure in any of our
 *         payload types such as @ref fbe_payload_block_operation_t.
 *         This allows the framework to treat different payload types
 *         in a common way.  This allows us to link together payloads of varying
 *         types for creating our stack of payloads.
 *  
 *  @see fbe_payload_ex_t
 */
typedef struct fbe_payload_operation_header_s{

    /*! The pointer to the prior header.  This will be NULL 
     *  if there is no prior header. 
     */
	struct fbe_payload_operation_header_s * prev_header;

    /*! This is the type of payload for this header.
     */
	fbe_payload_opcode_t payload_opcode;

	/*! This status is to preserve the original packet status for this operation.
	 */
	/* Peter Puhov. We need to find another way */
	fbe_status_t status;

}fbe_payload_operation_header_t;

/* Peter Puhov. We need to find another way */
static __forceinline  void
fbe_payload_operation_hearder_set_status(fbe_payload_operation_header_t * payload_operation_header, fbe_status_t status)
{
    payload_operation_header->status = status;
}

static __forceinline fbe_status_t
fbe_payload_operation_hearder_get_status(fbe_payload_operation_header_t * payload_operation_header)
{
    return payload_operation_header->status;
}


/*! @} */ /* end of group fbe_payload_operation_header */

#endif /* FBE_PAYLOAD_OPERATION_HEADER_H */
