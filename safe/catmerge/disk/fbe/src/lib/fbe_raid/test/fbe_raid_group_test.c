/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_raid_library_unit_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the main function for the logical drive unit tests.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sep.h"
#include "fbe_raid_library.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library_test_proto.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe_raid_test_private.h"

/******************************
 ** Imported functions.
 ******************************/
 
/*!***************************************************************
 *          fbe_raid_get_iots_size()
 ****************************************************************
 * @brief   Simply return the size of the iots.
 *
 * @param   None
 *
 * @return  size of iots
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_iots_size(void)
{
    return(sizeof(fbe_raid_iots_t));
}
/* end fbe_raid_get_iots_size */ 

/*!***************************************************************
 *          fbe_raid_get_siots_size()
 ****************************************************************
 * @brief   Simply return the size of the siots.
 *
 * @param   None
 *
 * @return  size of siots
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_siots_size(void)
{
    return(sizeof(fbe_raid_siots_t));
}
/* end fbe_raid_get_siots_size */

/*!***************************************************************
 *          fbe_raid_get_fruts_size()
 ****************************************************************
 * @brief   Simply return the size of the fruts.
 *
 * @param   None
 *
 * @return  size of fruts
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_fruts_size(void)
{
    return(sizeof(fbe_raid_fruts_t));
}
/* end fbe_raid_get_fruts_size */

/*!***************************************************************
 *          fbe_raid_get_vrts_size()
 ****************************************************************
 * @brief   Simply return the size of the vrts.
 *
 * @param   None
 *
 * @return  size of vrts
 *
 * @author
 *  8/14/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_vrts_size(void)
{
    return(sizeof(fbe_raid_vrts_t));
}
/* end fbe_raid_get_vrts_size */

/*!***************************************************************
 *          fbe_raid_get_vcts_size()
 ****************************************************************
 * @brief   Simply return the size of the vrts.
 *
 * @param   None
 *
 * @return  size of vrts
 *
 * @author
 *  8/14/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_vcts_size(void)
{
    return(sizeof(fbe_raid_vcts_t));
}
/* end fbe_raid_get_vcts_size */

/*!***************************************************************
 *          fbe_raid_get_common_size()
 ****************************************************************
 * @brief   Simply return the size of the common.
 *
 * @param   None
 *
 * @return  size of common
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_common_size(void)
{
    return(sizeof(fbe_raid_common_t));
}
/* end fbe_raid_get_common_size */

/*!***************************************************************
 *          fbe_raid_get_packet_size()
 ****************************************************************
 * @brief   Simply return the size of the iots.
 *
 * @param   None
 *
 * @return  size of an feb packet
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_packet_size(void)
{
    return(sizeof(fbe_packet_t));
}
/* end fbe_raid_get_packet_size */


/*!***************************************************************
 *          fbe_raid_get_payload_ex_size()
 ****************************************************************
 * @brief   Simply return the size of the iots.
 *
 * @param   None
 *
 * @return  size of payload
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_raid_get_payload_ex_size(void)
{
    return(sizeof(fbe_payload_ex_t));
}
/* end fbe_raid_get_payload_ex_size */
 
/*!***************************************************************
 * @fn fbe_raid_group_unit_tests_setup(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  05/21/09 - Created. RPF
 *
 ****************************************************************/
void 
fbe_raid_group_unit_tests_setup(void)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);
    return;
}
/* end fbe_raid_group_unit_tests_setup() */

/*!***************************************************************
 * @fn fbe_raid_group_unit_tests_teardown(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  05/21/09 - Created. RPF
 *
 ****************************************************************/
void 
fbe_raid_group_unit_tests_teardown(void)
{
    /* Set the trace level back to the default.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    /*! @note We might have modified the FBE infrastructure current methods, 
     * let's set them back to the defaults. 
     */
    //fbe_raid_group_reset_methods();

    /*! @note Make sure the handle count did not grow.  This would indicate that we 
     * leaked something. 
     */
    //fbe_raid_group_check_handle_count();
    return;
}
/* end fbe_raid_group_unit_tests_teardown() */


/*!***************************************************************
 *          fbe_raid_group_test_packet_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for io packets
 *  sent out by the logical drive.
 * 
 *  All we need to do is kick off the state machine for
 *  the io command that is completing.
 *
 * @param packet_p - The packet being completed.
 * @param context - The pdo io cmd operation.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * @author
 *  10/26/07 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_test_packet_completion(fbe_packet_t * packet_p, 
                                                             fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_status_t status;
    fbe_payload_block_operation_qualifier_t status_qualifier;
    fbe_lba_t media_error_lba;
    fbe_object_id_t pvd_object_id;

    /* Get the block operation and status.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_status(block_operation_p, &status);
    fbe_payload_block_get_qualifier(block_operation_p, &status_qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &media_error_lba); 
    fbe_payload_ex_get_pvd_object_id(payload_p, &pvd_object_id);

    /* Generate a trace with the packet status etc.
     */
    mut_printf(MUT_LOG_HIGH, "%s: Completed opcode %d lba %llx blocks %llu with status %d", 
               __FUNCTION__, opcode, (unsigned long long)lba,
	       (unsigned long long)block_count, status);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end fbe_raid_group_test_packet_completion() */

/*!***************************************************************
 *          fbe_raid_group_test_setup_sg_list()
 ****************************************************************
 * @brief
 *  This function sets up an sg list based on the input parameters.
 *
 * @param sg_p - Sg list to setup.
 * @param memory_p - Pointer to memory to use in sg.
 * @param bytes - Bytes to setup in sg.
 * @param max_bytes_per_sg - Maximum number of bytes per sg entry.
 *                           Allows us to setup longer sg lists.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/29/07 - Created. RPF
 *
 ****************************************************************/
void fbe_raid_group_test_setup_sg_list(fbe_sg_element_t *const sg_p,
                                       fbe_u8_t *const memory_p,
                                       fbe_u32_t bytes,
                                       fbe_u32_t max_bytes_per_sg)
{
    fbe_u32_t current_bytes;
    fbe_u32_t bytes_remaining = bytes;
    fbe_sg_element_t *current_sg_p = sg_p;
    
    if (max_bytes_per_sg == 0)
    {
        /* If the caller did not specify the max bytes per sg,
         * then assume we can put as much as we want in each sg entry.
         */
        max_bytes_per_sg = FBE_U32_MAX;
    }

    /* Loop over all the bytes that we are adding to this sg list.
     */
    while (bytes_remaining != 0)
    {
        if (max_bytes_per_sg > bytes_remaining)
        {
            /* Don't allow current_bytes to be negative.
             */
            current_bytes = bytes_remaining;
        }
        else
        {
            current_bytes = max_bytes_per_sg;
        }

        /* Setup this sg element.
         */
        fbe_sg_element_init(current_sg_p, current_bytes, memory_p);
        fbe_sg_element_increment(&current_sg_p);
        bytes_remaining -= current_bytes;

    } /* end while bytes remaining != 0 */

    /* Terminate the list.
     */
    fbe_sg_element_terminate(current_sg_p);
    return;
}
/* end fbe_raid_group_test_setup_sg_list */

/*!***************************************************************
 *  fbe_raid_group_test_build_packet()
 ****************************************************************
 * @brief
 *  This function builds a sub packet with the given parameters.
 * 
 * @param packet_p - Packet to fill in.
 * @param opcode - The opcode to set in the packet.
 * @param edge_p - The edge to put in the packet.
 * @param lba - The logical block address to put in the packet.
 * @param blocks - The block count.
 * @param block_size - The block size in bytes.
 * @param optimal_block_size - The optimal block size in blocks.
 * @param sg_p - Pointer to the sg list.
 * @param context - The context for the callback.
 *
 * @return
 *  NONE
 *
 * @author:
 *  05/19/09 - Created. RPF
 *
 ****************************************************************/
void fbe_raid_group_test_build_packet(fbe_packet_t * const packet_p,
                                      fbe_payload_block_operation_opcode_t opcode,
                                      fbe_block_edge_t *edge_p,
                                      fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_block_size_t block_size,
                                      fbe_block_size_t optimal_block_size,
                                      fbe_sg_element_t * const sg_p,
                                      fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;

    /* Initialize the packet and set the completion function.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_test_packet_completion,
                                          context);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    packet_p->base_edge = (fbe_base_edge_t*)edge_p;
    fbe_transport_set_cpu_id(packet_p, 0);
    /* First build the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      opcode,
                                      lba,
                                      blocks,
                                      block_size,
                                      optimal_block_size,
                                      NULL    /* no pre-read descriptor. */);

    /* Default our repeat count to 1.
     */
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);
    sg_desc_p->repeat_count = 1;
    return;
}
/* end fbe_raid_group_test_build_packet() */

/*!***************************************************************
 *  fbe_raid_group_test_build_packet_with_memory()
 ****************************************************************
 * @brief
 *  This function builds a sub packet with the given parameters.
 * 
 * @param packet_p - Packet to fill in.
 * @param opcode - The opcode to set in the packet.
 * @param lba - The logical block address to put in the packet.
 * @param blocks - The block count.
 * @param block_size - The block size in bytes.
 * @param optimal_block_size - The optimal block size in blocks.
 * @param sg_p - Pointer to the sg list.
 * @param context - The context for the callback.
 *
 * @return
 *  NONE
 *
 * @author:
 *  05/19/09 - Created. RPF
 *
 ****************************************************************/
void fbe_raid_group_test_build_packet_with_memory(fbe_packet_t * const packet_p,
                                      fbe_payload_block_operation_opcode_t opcode,
                                      fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_block_size_t block_size,
                                      fbe_block_size_t optimal_block_size,
                                      fbe_sg_element_t * const sg_p,
                                      fbe_packet_completion_context_t context)
{
    fbe_u32_t   bytes_to_transfer = 0;
    static void *memory_p = NULL;

    MUT_ASSERT_TRUE((blocks*block_size) <FBE_U32_MAX)

    /* Allocate the memory for the request.
     */
    bytes_to_transfer = (fbe_u32_t)(blocks * block_size);

    MUT_ASSERT_TRUE(bytes_to_transfer <= (1024 * 1024 * 10));

    if ((memory_p = malloc(bytes_to_transfer)) != NULL)
    {
        /* Populate the sg list.
         */
        fbe_raid_group_test_setup_sg_list(sg_p,
                                          (fbe_u8_t *)memory_p,
                                          bytes_to_transfer,
                                          (block_size * 32));
    }
    MUT_ASSERT_NOT_NULL(memory_p);

    fbe_raid_group_test_build_packet(packet_p, opcode, NULL, lba, blocks, block_size, optimal_block_size, sg_p, context);
    return;
}
/* end fbe_raid_group_test_build_packet_with_memory() */

/*!**************************************************************
 * fbe_raid_group_test_increment_stack_level()
 ****************************************************************
 * @brief
 *  A test function helper that increments the stack level for us.
 *
 * @param  packet_p - The packet to increment the stack level for.              
 *
 * @return None.   
 *
 * HISTORY:
 *  10/2/2008 - Created. RPF
 *
 ****************************************************************/
void fbe_raid_group_test_increment_stack_level(fbe_packet_t *const packet_p)
{
    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_status_t status;
    fbe_payload_ex_t *payload_p;
    payload_p = fbe_transport_get_payload_ex(packet_p);

    status = fbe_payload_ex_increment_block_operation_level(payload_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/*****************************************************
 * end fbe_raid_group_test_increment_stack_level()
 *****************************************************/

/*!***************************************************************************
 *      fbe_raid_library_test_raid_type_to_class_id()
 *****************************************************************************
 * @brief   Class id is used to validate raid type.  So create class id from
 *          desired raid type
 *
 * @param   raid_type - The raid type to initialize geometry with
 *
 * @return  class id - The class id that is valid for this raid type
 *
 * @author
 *  02/05/2010  Ron Proulx - Created
 *
 ****************************************************************/
static fbe_class_id_t fbe_raid_library_test_raid_type_to_class_id(fbe_raid_group_type_t raid_type)
{
    fbe_class_id_t  class_id = FBE_CLASS_ID_INVALID;

    /* Switch on raid type.
     */
    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID10:
            class_id = FBE_CLASS_ID_STRIPER;
            break;

        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            class_id = FBE_CLASS_ID_MIRROR;
            break;

        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID6:
            class_id = FBE_CLASS_ID_PARITY;
            break;

        case FBE_RAID_GROUP_TYPE_SPARE:
            class_id = FBE_CLASS_ID_VIRTUAL_DRIVE;
            break;

        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            class_id = FBE_CLASS_ID_PROVISION_DRIVE;
            break;

        default:
            /* defaulted to invalid */
            MUT_ASSERT_TRUE(FALSE);
            break;
    }

    /* Always return the class id.
     */
    return(class_id);
}
/* end of fbe_raid_library_test_raid_type_to_class_id() */

/*!***************************************************************
 *      fbe_raid_library_test_initialize_geometry()
 ****************************************************************
 * @brief   Initialize the mirror object for use in a testing 
 *          environment that doesn't setup the entire fbe infrastrcture.
 *
 * @param   raid_geometry_p - Pointer to raid_group object to initialize
 * @param   block_edge_p - Pointer to block edge for raid group
 * @param   raid_type - The raid type to initialize geometry with
 * @param   width - The the width of the raid group
 * @param   element_size - The element size of raid group
 * @param   elements_per_parity - Number of elements (stripes) in a parity stripe
 * @param   capacity - The capacity of the raid group
 * @param   generate_state - The method to invoke for generating requests
 *
 * @param   The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/30/2009  Rob Foley  - Created from fbe_raid_group_test_initialize_mirror
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_test_initialize_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_block_edge_t *block_edge_p,
                                                       fbe_raid_group_type_t raid_type,
                                                       fbe_u32_t width,
                                                       fbe_element_size_t element_size,
                                                       fbe_elements_per_parity_t elements_per_parity,
                                                       fbe_lba_t capacity,
                                                       fbe_raid_common_state_t generate_state)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_block_count_t max_blocks_per_drive_request = 0x800;  /* Default max per-drive request is 2048 blocks */

    /* First initialize the geometry.
     */
    status = fbe_raid_geometry_init(raid_geometry_p, 
                                    0 /* Set raid group object id to 0 */,
                                    fbe_raid_library_test_raid_type_to_class_id(raid_type),
                                    0 /* Set metadata element to 0 */,
                                    block_edge_p,
                                    generate_state);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Now configure the raid group geometry.
     */
    status = fbe_raid_geometry_set_configuration(raid_geometry_p,
                                                 width,
                                                 raid_type,
                                                 element_size,
                                                 elements_per_parity,
                                                 capacity,
                                                 max_blocks_per_drive_request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return(status);
}
/* end fbe_raid_library_test_initialize_geometry() */

/*********************************************************************
 * fbe_raid_group_test_memory_error_callback()
 *********************************************************************
 * @brief
 *   This is the callback form the raid memory mock functions when a
 *   major error has occurred.  We simply force a MUT assert.
 *  
 * @param None.
 *  
 * @return None.
 *
 * @author
 *  10/20/2010  Ron Proulx  - Created
 * 
 ********************************************************************/
static void fbe_raid_group_test_memory_error_callback(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* Simply force a MUT assert
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/*************************************************
 * end fbe_raid_group_test_memory_error_callback()
 *************************************************/

/*!***************************************************************
 * @fn fbe_raid_group_test_init_memory(void)
 ****************************************************************
 * @brief
 *  
 *
 *
 ****************************************************************/
void fbe_raid_group_test_init_memory(void)
{
    fbe_packet_t *packet = malloc(sizeof(fbe_packet_t));

    fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *						payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_status_t fbe_memory_control_entry(fbe_packet_t * packet);

    fbe_memory_dps_init_parameters_t dps_init_parameters = 
    {
        (10 + 10 + 20), /* main chunks (must be >= total other types)*/
        10, /* packet chunks */
        20  /* 64 chunks */
    };

    /* Need to initialize both the dps and standard memory pools
     */
    status =  fbe_memory_init_number_of_chunks(10);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*! @note Why does dps complain about double initialization?
     *
     *  @note Without the call below now dps pages get allocated which results
     *        in an unexpected state0 state status of FBE_RAID_STATE_STATUS_WAITING
     */
    status =  fbe_memory_dps_init_number_of_chunks(&dps_init_parameters);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_NOT_NULL(packet);

    /* Allocate packet.*/
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_SERVICE_CONTROL_CODE_INIT,
                                        NULL, 0);

	fbe_transport_set_completion_function(packet, NULL, NULL);
	fbe_payload_ex_increment_control_operation_level(payload);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_MEMORY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 

    status = fbe_memory_control_entry(packet);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    free(packet);

    /*! Now configure then raid's memory.  We configure the raid memory as
     *  follows:
     *  o b_use_raid_mock_memory_service - FBE_TRUE  (@note None of the unit tests can handle deferred allocations!!)
     *  o b_allow_deferred_allocations   - FBE_FALSE (@note None of the unit tests can handle deferred allocations!!)
     *  o error_callback                 - Set to method that will execute a MUT_ASSERT
     *
     *  @note Must call configure mocks BEFORE calling fbe_raid_library_initialize!!
     */
    status = fbe_raid_memory_mock_configure_mocks(FBE_TRUE, FBE_FALSE,
                                                  (fbe_raid_memory_mock_allocate_mock_t)NULL, /* Use mock memory service allocate*/
                                                  (fbe_raid_memory_mock_abort_mock_t)NULL,    /* Use mock memory service abort*/
                                                  (fbe_raid_memory_mock_free_mock_t)NULL,     /* Use mock memory service free */
                                                  fbe_raid_group_test_memory_error_callback);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/* end fbe_raid_group_test_init_memory() */

/*!***************************************************************
 * @fn fbe_raid_group_test_destroy_memory(void)
 ****************************************************************
 * @brief
 *  
 *
 *
 ****************************************************************/
void fbe_raid_group_test_destroy_memory(void)
{
    fbe_packet_t *packet = malloc(sizeof(fbe_packet_t));

    fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *						payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_status_t fbe_memory_control_entry(fbe_packet_t * packet);

    MUT_ASSERT_NOT_NULL(packet);

    /* Allocate packet.*/
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_SERVICE_CONTROL_CODE_DESTROY,
                                        NULL, 0);

	fbe_transport_set_completion_function(packet, NULL, NULL);
	fbe_payload_ex_increment_control_operation_level(payload);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_MEMORY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 

    status = fbe_memory_control_entry(packet);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    free(packet);
    return;
}
/* end fbe_raid_group_test_destroy_memory() */

/*!***************************************************************
 * fbe_raid_library_add_io_tests()
 ****************************************************************
 * @brief
 *  This function adds the I/O unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  05/26/2009  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_raid_library_add_io_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    return;
}
/* end fbe_raid_library_add_io_tests() */

/*!**************************************************************
 * fbe_raid_group_test_lifecycle()
 ****************************************************************
 * @brief
 *  Validates the raid_group's lifecycle.
 *
 * @param - none.          
 *
 * @return none
 *
 * HISTORY:
 *  7/16/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_raid_group_test_lifecycle(void)
{
    fbe_status_t status;
    fbe_status_t fbe_raid_group_monitor_load_verify(void);

    /* Make sure this data is valid.
     */
    status = fbe_raid_group_monitor_load_verify();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_raid_group_test_lifecycle()
 ******************************************/
/*!***************************************************************
 * fbe_raid_library_add_unit_tests()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  05/21/09 - Created. RPF
 *
 ****************************************************************/
void fbe_raid_library_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    fbe_raid_library_test_add_sg_util_tests(suite_p);

    //fbe_raid_memory_test_add_tests(suite_p);
    
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_lifecycle, NULL, NULL);
    return;
}
/* end fbe_raid_library_add_unit_tests() */

/*****************************************
 * end fbe_raid_library_unit_test.c
 *****************************************/
