#ifndef BGSL_PAYLOAD_OPERATION_HEADER_H
#define BGSL_PAYLOAD_OPERATION_HEADER_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file bgsl_payload_operation_header.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the 
 *  bgsl_payload_operation_header_t structure and the related structures, 
 *  enums and definitions.
 * 
 *  The @ref bgsl_payload_operation_header_t allows us to treat many different
 *  payload types in a common way.  This allows the bgsl_payload_t to
 *  contain a stack of payloads.
 * 
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/bgsl_types.h"

/*! @defgroup bgsl_payload_operation_header FBE Payload Operation Header 
 *  
 *  This is the set of definitions that make up the FBE Payload Operation
 *  Header.
 *  
 *  
 *  @ingroup bgsl_payload_interfaces
 * @{
 */ 

/*! @enum bgsl_payload_opcode_t  
 *  
 *  @brief This opcode identifies the type of payload this header is using.
 *  
 *  @see bgsl_payload_operation_header_t
 */
typedef enum bgsl_payload_opcode_e {
	BGSL_PAYLOAD_OPCODE_INVALID,

	BGSL_PAYLOAD_OPCODE_BLOCK_OPERATION,
	BGSL_PAYLOAD_OPCODE_CONTROL_OPERATION,
	BGSL_PAYLOAD_OPCODE_DISCOVERY_OPERATION,
	BGSL_PAYLOAD_OPCODE_CDB_OPERATION,
	BGSL_PAYLOAD_OPCODE_FIS_OPERATION,
	BGSL_PAYLOAD_OPCODE_DMRB_OPERATION,
	BGSL_PAYLOAD_OPCODE_SMP_OPERATION,

	BGSL_PAYLOAD_OPCODE_LAST
}bgsl_payload_opcode_t;

/*! @struct bgsl_payload_operation_header_t  
 *  
 *  @brief This structure is always the first structure in any of our
 *         payload types such as @ref bgsl_payload_block_operation_t.
 *         This allows the framework to treat different payload types
 *         in a common way.  This allows us to link together payloads of varying
 *         types for creating our stack of payloads.
 *  
 *  @see bgsl_payload_t
 */
typedef struct bgsl_payload_operation_header_s{

    /*! The pointer to the prior header.  This will be NULL 
     *  if there is no prior header. 
     */
	struct bgsl_payload_operation_header_s * prev_header;

    /*! This is the type of payload for this header.
     */
	bgsl_payload_opcode_t payload_opcode;
}bgsl_payload_operation_header_t;

/*! @} */ /* end of group bgsl_payload_operation_header */

#endif /* BGSL_PAYLOAD_OPERATION_HEADER_H */
