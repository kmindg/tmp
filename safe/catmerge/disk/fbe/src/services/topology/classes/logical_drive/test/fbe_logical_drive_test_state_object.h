#ifndef FBE_LOGICAL_DRIVE_TEST_STATE_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_STATE_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_state_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition for the logical drive object's test state object.
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
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"

/*! @def FBE_LDO_STATE_MAX_BLKS_PER_SG  
 *  
 *  @brief This is the max number of blocks that we will allow per
 *         sg entry in our testing.
 */
#define FBE_LDO_STATE_MAX_BLKS_PER_SG 0xFFFFFFFF 

/*! @struct fbe_ldo_test_state_t 
 *  
 *  @brief This object is used for testing the logical drive object
 *         and its state machines.
 *  
 *         In order to test the logical drive object's code we need
 *         to have certain structures, which this code expects as input
 *         already setup.  This includes the @ref fbe_logical_drive_t object,
 *         the I/O packet and its associated data.
 *  
 *         In other cases the code expects an @ref fbe_logical_drive_io_cmd_t so
 *         we have one of these setup as well.
 *  
 *         Note that we insert check words in the structure in order to detect
 *         cases where one item may overflow into the next.
 */
typedef struct fbe_ldo_test_state_s
{
    fbe_u32_t        check_word_0;
    /*! This is the instantiation of the logical drive we will use in testing. 
     */
    fbe_logical_drive_t logical_drive;
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
    fbe_logical_drive_io_cmd_t io_cmd;
    fbe_u32_t        check_word_4;
    
    /*! This is the sg that the client's io packet uses. 
     */
    fbe_sg_element_t io_packet_sg[FBE_LDO_MAX_SG_LENGTH];
    fbe_u32_t        check_word_5;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         io_packet_sg_memory;
    fbe_u32_t        check_word_6;
    fbe_u32_t        sg_offset;
    fbe_u32_t        sg_length;

    /*! This is the sg that the io command uses. 
     */
    fbe_sg_element_t io_cmd_sg[FBE_LDO_MAX_SG_LENGTH];
    fbe_u32_t        check_word_7;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         io_cmd_sg_memory;
    fbe_u32_t        check_word_8;

    /* This is the sg that the io command uses. */
    fbe_sg_element_t pre_read_desc_sg[FBE_LDO_MAX_SG_LENGTH];
    fbe_u32_t        check_word_9;

    /*! Since the data coming in is not real data, 
     *  we need a memory address to use in the sg, and this is the memory
     *  address that we will use when setting up the sg. 
     */
    fbe_u8_t         pre_read_desc_sg_memory;
    fbe_u32_t        check_word_10;

    /*! This is only used in test cases where negotiate information is required.
     */
    fbe_block_transport_negotiate_t negotiate;    
    fbe_u32_t        check_word_11;

    /*! This is the maximum number of blocks per sg entry that we will allow. 
     */
    fbe_u32_t        max_blocks_per_sg_entry;
      
    fbe_u32_t        check_word_12;
}
fbe_ldo_test_state_t;

/*************************
 *   ACCESSOR METHODS
 *************************/
static __inline fbe_sg_element_t * fbe_ldo_test_state_get_io_packet_sg(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->io_packet_sg[0];
}
static __inline fbe_sg_element_t * fbe_ldo_test_state_get_io_cmd_sg(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->io_cmd_sg[0];
}
static __inline fbe_sg_element_t * fbe_ldo_test_state_get_pre_read_desc_sg(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->pre_read_desc_sg[0];
}
static __inline fbe_logical_drive_t * fbe_ldo_test_state_get_logical_drive(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->logical_drive;
}
static __inline fbe_packet_t * fbe_ldo_test_state_get_io_packet(fbe_ldo_test_state_t * const state_p)
{
    return state_p->io_packet_p;
}
static __inline fbe_logical_drive_io_cmd_t * fbe_ldo_test_state_get_io_cmd(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->io_cmd;
}
static __inline fbe_u8_t * fbe_ldo_test_state_get_io_packet_sg_memory(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->io_packet_sg_memory;
}
static __inline fbe_u8_t * fbe_ldo_test_state_get_io_cmd_memory(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->io_cmd_sg_memory;
}
static __inline fbe_u8_t * fbe_ldo_test_state_get_pre_read_desc_sg_memory(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->pre_read_desc_sg_memory;
}
static __inline fbe_u32_t fbe_ldo_test_get_max_blks_per_sg(fbe_ldo_test_state_t * const state_p)
{
    return state_p->max_blocks_per_sg_entry;
}
static __inline void fbe_ldo_test_set_max_blks_per_sg(fbe_ldo_test_state_t * const state_p,
                                               fbe_u32_t max_blks)
{
    state_p->max_blocks_per_sg_entry = max_blks;
    return;
}
static __inline fbe_block_transport_negotiate_t * fbe_ldo_test_state_get_negotiate(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->negotiate;
}
static __inline fbe_u32_t fbe_ldo_test_get_sg_offset(fbe_ldo_test_state_t * const state_p)
{
    return state_p->sg_offset;
}
static __inline void fbe_ldo_test_set_sg_offset(fbe_ldo_test_state_t * const state_p,
                                         fbe_u32_t sg_offset)
{
    state_p->sg_offset = sg_offset;
}
static __inline fbe_u32_t fbe_ldo_test_get_sg_length(fbe_ldo_test_state_t * const state_p)
{
    return state_p->sg_length;
}
static __inline void fbe_ldo_test_set_sg_length(fbe_ldo_test_state_t * const state_p,
                                         fbe_u32_t sg_length)
{
    state_p->sg_length = sg_length;
}

static __inline fbe_payload_pre_read_descriptor_t *const 
fbe_ldo_test_get_pre_read_descriptor(fbe_ldo_test_state_t * const state_p)
{
    return &state_p->pre_read_descriptor;
}

/* The below is a function definition that uses the test state for 
 * testing setup sgs function. 
 */
typedef fbe_status_t (*fbe_ldo_test_count_setup_sgs_fn)(fbe_ldo_test_state_t * const test_state_p,
                                                        fbe_ldo_test_state_machine_task_t *const task_p );

/* This is a test case function prototype for setting up a state machine test.
 */
typedef fbe_status_t (*fbe_ldo_test_state_setup_fn_t)(fbe_ldo_test_state_t * const test_state_p,
                                                      fbe_ldo_test_state_machine_task_t *const task_p);

/* This is a test case function prototype for executing a single state.
 */
typedef fbe_status_t (*fbe_ldo_test_state_execute_fn_t)(fbe_ldo_test_state_t * const test_state_p);

/*************************
 *   API FUNCTIONS
 *************************/
fbe_ldo_test_state_t *fbe_ldo_test_get_state_object(void);

fbe_status_t fbe_ldo_test_state_init(fbe_ldo_test_state_t * const state_p);

void fbe_ldo_test_state_init_check_words(fbe_ldo_test_state_t * const state_p);

fbe_status_t fbe_ldo_test_state_validate_check_words(fbe_ldo_test_state_t * const state_p);

fbe_status_t fbe_ldo_test_state_destroy(fbe_ldo_test_state_t * const state_p);

#endif /* FBE_LOGICAL_DRIVE_TEST_STATE_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_state_object.h
 *****************************************************/
