#ifndef FBE_RAID_GROUP_TEST_STATE_OBJECT_H
#define FBE_RAID_GROUP_TEST_STATE_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_library_test_state_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition for the raid group's test state object.
 *  The purpose of this object is to contain relevant data structures
 *  that we need in order to test logical drive code.
 *
 * @version
 *   12/03/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_library_test_proto.h"
#include "fbe_raid_library_test.h"

/*! @struct fbe_raid_group_test_state_t 
 *  
 *  @brief This object is used for testing the raid group
 *         and its state machines.
 *  
 *         In order to test the raid group's code we need
 *         to have certain structures, which this code expects as input
 *         already setup.  This includes the @ref fbe_raid_group_t object, the packet and
 *         its associated data.
 *  
 *         Note that we insert check words in the structure in order to detect
 *         cases where one item may overflow into the next.
 */
typedef struct fbe_raid_group_test_state_s
{
    fbe_u32_t        check_word_0;
    /*! This is the instantiation of the logical drive we will use in testing. 
     */
    fbe_raid_geometry_t raid_geometry;
    fbe_u32_t        check_word_1;

    /*! This is the io packet memory and the ptr to the io packet. 
     *  We allocate a full chunk since this is how much memory a packet
     *  consumes.
     */
    fbe_packet_t io_packet_memory;
    fbe_payload_pre_read_descriptor_t pre_read_descriptor;
    fbe_u32_t        check_word_2;
    fbe_packet_t    *io_packet_p;
    fbe_u32_t        check_word_3;

    /*! This is the io command object.  This is used in test cases where 
     *  the code accepts an io command as input.  This is only in cases
     *  where resources need to get allocated.
     */
    fbe_raid_iots_t  *iots_p;
    fbe_u32_t        check_word_4;
    fbe_raid_siots_t *siots_p;
    fbe_u32_t        check_word_5;
    
    /*! This is the sg that the client's io packet uses. 
     */
    fbe_sg_element_t io_packet_sg[FBE_RAID_GROUP_TEST_MAX_SG_LENGTH];
    fbe_u32_t        check_word_6;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         io_packet_sg_memory;
    fbe_u32_t        check_word_7;
    fbe_u32_t        sg_offset;
    fbe_u32_t        sg_length;

    /*! This is the sg that the io command uses. 
     */
    fbe_sg_element_t iots_sg[FBE_RAID_GROUP_TEST_MAX_SG_LENGTH];
    fbe_u32_t        check_word_8;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         iots_sg_memory;
    fbe_u32_t        check_word_9;

    /* This is the sg that the io command uses. */
    fbe_sg_element_t pre_read_desc_sg[FBE_RAID_GROUP_TEST_MAX_SG_LENGTH];
    fbe_u32_t        check_word_10;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         pre_read_desc_sg_memory;
    fbe_u32_t        check_word_11;

    /*! This is only used in test cases where negotiate information is required.
     */
    fbe_block_edge_t block_edge;    
    fbe_u32_t        check_word_12;

    /*! This is the maximum number of blocks per sg entry that we will allow. 
     */
    fbe_u32_t        max_blocks_per_sg_entry;
      
    fbe_u32_t        check_word_13;
}
fbe_raid_group_test_state_t;

/*************************
 *   ACCESSOR METHODS
 *************************/
static __inline fbe_sg_element_t * fbe_raid_group_test_state_get_io_packet_sg(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->io_packet_sg[0];
}
static __inline fbe_sg_element_t * fbe_raid_group_test_state_get_iots_sg(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->iots_sg[0];
}
static __inline fbe_sg_element_t * fbe_raid_group_test_state_get_pre_read_desc_sg(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->pre_read_desc_sg[0];
}
static __inline fbe_raid_geometry_t * fbe_raid_group_test_state_get_raid_geometry(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->raid_geometry;
}
static __inline fbe_packet_t * fbe_raid_group_test_state_get_io_packet(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->io_packet_p;
}
static __inline fbe_raid_iots_t * fbe_raid_group_test_state_get_iots(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->iots_p;
}
static __inline void fbe_raid_group_test_state_set_iots(fbe_raid_group_test_state_t * const state_p)
{
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(fbe_raid_group_test_state_get_io_packet(state_p));
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    state_p->iots_p = iots_p;
    return;
}
static __inline fbe_raid_siots_t * fbe_raid_group_test_state_get_siots(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->siots_p;
}
static __inline void fbe_raid_group_test_state_set_siots(fbe_raid_group_test_state_t * const state_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(fbe_raid_group_test_state_get_io_packet(state_p));
    fbe_payload_ex_get_siots(payload_p, &siots_p);
    state_p->siots_p = siots_p;
    return;
}
static __inline fbe_u8_t * fbe_raid_group_test_state_get_io_packet_sg_memory(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->io_packet_sg_memory;
}
static __inline fbe_u8_t * fbe_raid_group_test_state_get_iots_memory(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->iots_sg_memory;
}
static __inline fbe_u8_t * fbe_raid_group_test_state_get_pre_read_desc_sg_memory(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->pre_read_desc_sg_memory;
}
static __inline fbe_u32_t fbe_raid_group_test_get_max_blks_per_sg(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->max_blocks_per_sg_entry;
}
static __inline void fbe_raid_group_test_set_max_blks_per_sg(fbe_raid_group_test_state_t * const state_p,
                                               fbe_u32_t max_blks)
{
    state_p->max_blocks_per_sg_entry = max_blks;
    return;
}
static __inline fbe_block_edge_t * fbe_raid_group_test_state_get_block_edge(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->block_edge;
}
static __inline fbe_u32_t fbe_raid_group_test_get_sg_offset(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->sg_offset;
}
static __inline void fbe_raid_group_test_set_sg_offset(fbe_raid_group_test_state_t * const state_p,
                                         fbe_u32_t sg_offset)
{
    state_p->sg_offset = sg_offset;
}
static __inline fbe_u32_t fbe_raid_group_test_get_sg_length(fbe_raid_group_test_state_t * const state_p)
{
    return state_p->sg_length;
}
static __inline void fbe_raid_group_test_set_sg_length(fbe_raid_group_test_state_t * const state_p,
                                         fbe_u32_t sg_length)
{
    state_p->sg_length = sg_length;
}

static __inline fbe_payload_pre_read_descriptor_t *const 
fbe_raid_group_test_get_pre_read_descriptor(fbe_raid_group_test_state_t * const state_p)
{
    return &state_p->pre_read_descriptor;
}

/* The below is a function definition that uses the test state for 
 * testing setup sgs function. 
 */
typedef fbe_status_t (*fbe_raid_group_test_count_setup_sgs_fn)(fbe_raid_group_test_state_t * const test_state_p,
                                                               fbe_raid_group_test_state_machine_task_t *const task_p );

/* This is a test case function prototype for setting up a state machine test.
 */
typedef fbe_status_t (*fbe_raid_group_test_state_setup_fn_t)(fbe_raid_group_test_state_t * const test_state_p,
                                                             fbe_raid_group_test_state_machine_task_t *const task_p);
/* This is a test case function prototype for executing a single state.
 */
typedef fbe_status_t (*fbe_raid_group_test_state_execute_fn_t)(fbe_raid_group_test_state_t * const test_state_p);

/* This is a teardown function for the test state.
 */
typedef fbe_status_t (*fbe_raid_group_test_state_destroy_fn_t)(fbe_raid_group_test_state_t * const test_state_p);

/*************************
 *   API FUNCTIONS
 *************************/
fbe_raid_group_test_state_t *fbe_raid_group_test_get_state_object(void);

fbe_status_t fbe_raid_group_test_state_init(fbe_raid_group_test_state_t * const state_p);

void fbe_raid_group_test_state_init_check_words(fbe_raid_group_test_state_t * const state_p);

fbe_status_t fbe_raid_group_test_state_validate_check_words(fbe_raid_group_test_state_t * const state_p);

fbe_status_t fbe_raid_group_test_state_destroy(fbe_raid_group_test_state_t * const state_p);

fbe_status_t fbe_raid_group_test_execute_state_machine(
    fbe_raid_group_test_state_t * const test_state_p,
    fbe_raid_group_test_state_setup_fn_t state_setup_fn_p,
    fbe_raid_group_test_state_execute_fn_t state_fn_p,
    fbe_raid_group_test_state_destroy_fn_t destroy_fn_p,
    fbe_raid_group_test_state_machine_task_t *const task_p);

fbe_status_t fbe_raid_test_state_setup(fbe_raid_group_test_state_t * const test_state_p,
                                       fbe_raid_group_test_state_machine_task_t *const task_p,
                                       fbe_payload_block_operation_opcode_t opcode);

#endif /* FBE_RAID_GROUP_TEST_STATE_OBJECT_H */
/*****************************************************
 * end fbe_raid_library_test_state_object.h
 *****************************************************/
