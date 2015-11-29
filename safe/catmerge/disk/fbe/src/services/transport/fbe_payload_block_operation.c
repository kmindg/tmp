/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_payload_block_operation.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions for the payload's block operation.
 * 
 *  The fbe_payload_block_operation_t is the payload for the logical
 *  storage extent protocol.  This protocol is used for all logical block
 *  operations.
 * 
 * @see fbe_payload_block_operation_t
 * 
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"


/*!**************************************************************
 * @fn fbe_payload_block_build_operation(   
 *        fbe_payload_block_operation_t *payload_block_operation,   
 *        fbe_payload_block_operation_opcode_t block_opcode,  
 *        fbe_lba_t lba,  
 *        fbe_block_count_t block_count,  
 *        fbe_block_size_t block_size,  
 *        fbe_block_size_t optimum_block_size,  
 *        fbe_payload_pre_read_descriptor_t *pre_read_descriptor)  
 ****************************************************************
 * @brief This is the builder function for creating a block  
 *        operation payload. 
 *        NOTE: By default this will set the 'check checksum' flag. If
 *        the caller does not wish to check checksums, they are responsible
 *        for clearing the flag.
 * 
 * @param payload_block_operation - The ptr to the payload structure
 *                                  that we wish to build.
 * @param block_opcode - The block payload opcode to place in the structure.
 * @param lba - The logical block address to place in the struct.
 * @param block_count - The number of blocks for this operation.
 * @param block_size - The size of the block in bytes for this operation.
 * @param optimum_block_size - The number of blocks that make up the optimal
 *                             block size.
 * @param pre_read_descriptor - The ptr to the pre-read descriptor, which
 *                              is only used for write operations and
 *                              which contains a description of the read
 *                              data which is needed for writes that are not
 *                              aligned to the optimum block size.
 * 
 * @return fbe_status_t always FBE_STATUS_OK
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_block_build_operation(fbe_payload_block_operation_t * payload_block_operation,
                                  fbe_payload_block_operation_opcode_t block_opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t block_count,
                                  fbe_block_size_t block_size,
                                  fbe_block_size_t optimum_block_size,
                                  fbe_payload_pre_read_descriptor_t * pre_read_descriptor)
{
    payload_block_operation->block_opcode = block_opcode;
    payload_block_operation->lba = lba;
    payload_block_operation->block_count = block_count;
    payload_block_operation->block_size = block_size;
    payload_block_operation->optimum_block_size = optimum_block_size;
    payload_block_operation->pre_read_descriptor = pre_read_descriptor;
    payload_block_operation->block_edge_p = NULL;
    payload_block_operation->throttle_count = 0;
    payload_block_operation->io_credits = 0;
    //payload_block_operation->pdo_object_id = FBE_OBJECT_ID_INVALID ;
    payload_block_operation->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    payload_block_operation->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_payload_block_init_flags(payload_block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_build_identify(
 *        fbe_payload_block_operation_t * block_operation_p)
 ****************************************************************
 * @brief This is the builder function for creating an identify block  
 *        operation payload.
 *        @see FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY
 * 
 * @param block_operation_p - The ptr to the payload structure
 *                            that we wish to build.
 * 
 * @return fbe_status_t always FBE_STATUS_OK
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_block_build_identify(fbe_payload_block_operation_t * block_operation_p)
{
    /* Setup the opcodes in the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY,
                                      0, 0, 0, 0, NULL);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_build_negotiate_block_size(
 *        fbe_payload_block_operation_t * block_operation_p)
 ****************************************************************
 * @brief This is the builder function for creating a
 *        negotiate block size block operation payload.
 *        @see FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE
 * 
 * @param block_operation_p - The ptr to the payload structure
 *                            that we wish to build.
 * 
 * @return fbe_status_t always FBE_STATUS_OK
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_block_build_negotiate_block_size(fbe_payload_block_operation_t * block_operation_p)
{
    /* Setup the opcodes in the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE,
                                      0,    /* lba is not valid for this request */
                                      0,    /* block_count is not valid for this request */
                                      FBE_BE_BYTES_PER_BLOCK, /* This is the default client block size*/ 
                                      0,    /* Optimal block size is unknown */ 
                                      NULL  /* There is no pre-read descriptor */);
    return FBE_STATUS_OK;
}
