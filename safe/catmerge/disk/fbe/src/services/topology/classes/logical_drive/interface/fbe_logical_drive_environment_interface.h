#ifndef FBE_LOGICAL_DRIVE_ENVIRONMENT_INTERFACE_H
#define FBE_LOGICAL_DRIVE_ENVIRONMENT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_environment_interface.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains accessor methods which isolate the logical drive from
 *  fbe.
 * 
 * HISTORY
 *  04/29/2008:  Created. RPF
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"

/* Accessor functions for the payload status.
 */
static __inline void 
fbe_ldo_set_payload_status(fbe_packet_t * packet_p, 
                           fbe_payload_block_operation_status_t code, 
                           fbe_payload_block_operation_qualifier_t qualifier)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_set_status(block_operation_p, code, qualifier);
    return;
}

/* Accessor functions for the payload status.
 */
static __inline fbe_payload_block_operation_status_t fbe_ldo_get_payload_status(fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_status_t status;
    fbe_payload_block_get_status(block_operation_p, &status);
    return status;
}

/* Accessor functions for the payload status qualifier.
 */
static __inline fbe_payload_block_operation_qualifier_t fbe_ldo_get_payload_qualifier(fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_qualifier_t qualifier;

    fbe_payload_block_get_qualifier(block_operation_p, &qualifier);
    return qualifier;
}

/* Accessor functions for the opcode.
 */
static __forceinline fbe_payload_block_operation_opcode_t
ldo_packet_get_block_cmd_opcode(fbe_packet_t * const packet_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
	return opcode;
}
/* Accessor functions for the lba.
 */
static __forceinline fbe_lba_t
ldo_packet_get_block_cmd_lba(fbe_packet_t * const packet_p)
{
    fbe_lba_t lba;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
	return lba;
}

/* Accessor functions for the block count.
 */
static __forceinline fbe_block_count_t
ldo_packet_get_block_cmd_blocks(fbe_packet_t * const packet_p)
{
    fbe_block_count_t blocks;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
	return blocks;
}

/* Accessor functions for the block size.
 */
static __forceinline fbe_block_size_t
ldo_packet_get_block_cmd_block_size(fbe_packet_t * const packet_p)
{
    fbe_block_size_t block_size;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_block_size(block_operation_p, &block_size);
	return block_size;
}
/* Accessor functions for the block size.
 */
static __forceinline fbe_block_size_t
ldo_packet_get_block_cmd_optimum_block_size(fbe_packet_t * const packet_p)
{
    fbe_block_size_t optimum_block_size;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_optimum_block_size(block_operation_p, &optimum_block_size);
	return optimum_block_size;
}

/* This will return the write pre-read descriptor.
 */
static __forceinline fbe_payload_pre_read_descriptor_t *
ldo_packet_get_block_cmd_pre_read_desc_ptr(fbe_packet_t * const packet_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_pre_read_descriptor_t *pre_read_desc_p;

    fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);
	return pre_read_desc_p;
}
/* This will set the write pre-read descriptor.
 */
static __forceinline void 
ldo_packet_set_block_cmd_pre_read_desc_ptr(fbe_packet_t * const packet_p, 
                                                fbe_payload_pre_read_descriptor_t * const desc_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_set_pre_read_descriptor(block_operation_p, desc_p);
	return;
}

/* This will return the write pre-read descriptor's lba.
 */
static __forceinline fbe_lba_t
ldo_packet_get_block_cmd_pre_read_desc_lba(fbe_packet_t * const packet_p)
{
    fbe_lba_t lba;
    fbe_payload_pre_read_descriptor_t *pre_read_desc_p;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);
    fbe_payload_pre_read_descriptor_get_lba(pre_read_desc_p, &lba);
	return lba;
}

/* This will return the write pre-read descriptor's sg_ptr.
 */
static __forceinline fbe_sg_element_t *
ldo_packet_get_block_cmd_pre_read_desc_sg_ptr(fbe_packet_t * const packet_p)
{
    fbe_sg_element_t *sg_p;
    fbe_payload_pre_read_descriptor_t *pre_read_desc_p;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);
    fbe_payload_pre_read_descriptor_get_sg_list(pre_read_desc_p, &sg_p);
	return sg_p;
}

#endif /* FBE_LOGICAL_DRIVE_ENVIRONMENT_INTERFACE_H */
