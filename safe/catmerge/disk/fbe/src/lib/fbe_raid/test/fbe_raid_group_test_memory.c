/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the memory test.
 *
 * @version
 *   8/6/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "mut.h"
#include "fbe_raid_library_test.h"
#include "fbe_raid_library_test_proto.h"
#include "fbe_raid_test_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!*******************************************************************
 * @def FBE_RAID_MEMORY_TEST_MAX_NUM_SGS
 *********************************************************************
 * @brief Max number of sg entries we will test with.
 *
 *********************************************************************/
#define FBE_RAID_MEMORY_TEST_MAX_NUM_SGS FBE_RAID_MAX_DISK_ARRAY_WIDTH

fbe_memory_id_t fbe_raid_test_sg_id_array[FBE_RAID_MAX_SG_TYPE][FBE_RAID_MEMORY_TEST_MAX_NUM_SGS];



/*!*******************************************************************
 * @struct fbe_raid_test_sg_allocation_record_t
 *********************************************************************
 * @brief One test case per sg allocation test.
 *
 *********************************************************************/
typedef struct fbe_raid_test_sg_allocation_record_s
{
    fbe_u16_t sg_count[FBE_RAID_MAX_SG_TYPE];
}
fbe_raid_test_sg_allocation_record_t;

/*!*******************************************************************
 * @var fbe_raid_memory_test_sg_allocation_cases
 *********************************************************************
 * @brief Test cases with one number per sg type to indicate
 *        how many sgs to allocate per sg type.
 *
 *********************************************************************/
fbe_raid_test_sg_allocation_record_t fbe_raid_memory_test_sg_allocation_cases[] =
{
    /* Just one per sg type.
     */
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},

    /* Two: one per sg type.
     */
    {1, 1, 0, 0},
    {1, 0, 0, 1},
    {1, 0, 1, 0},
    {0, 1, 0, 1},
    {0, 1, 1, 0},
    {0, 0, 1, 1},

    /* Two: two per sg type.
     */
    {2, 0, 0, 0},
    {0, 2, 0, 0},
    {0, 0, 2, 0},
    {0, 0, 0, 2},

    /* Three: three per sg type.
     */
    {3, 0, 0, 0},
    {0, 3, 0, 0},
    {0, 0, 3, 0},
    {0, 0, 0, 3},

    /* Three: two and one per sg type.
     */
    {2, 1, 0, 0},
    {0, 2, 1, 0},
    {0, 0, 2, 1},
    {1, 0, 0, 2},

    /* N: Each one allocates the same number.
     */
    {1, 1, 1, 1},
    {2, 2, 2, 2},
    {3, 3, 3, 3},
    {4, 4, 4, 4},
    {8, 8, 8, 8},
    {16, 16, 16, 16},

    /* N: Varous combinations of N with 0.
     */
    {1, 1, 1, 0},
    {0, 1, 1, 1},

    {2, 2, 2, 0},
    {0, 2, 2, 2},
    {2, 0, 0, 2},
    {0, 2, 2, 0},

    {3, 3, 3, 0},
    {0, 3, 3, 3},
    {3, 0, 0, 3},
    {0, 3, 3, 0},

    {4, 4, 4, 0},
    {0, 4, 4, 4},
    {4, 0, 0, 4},
    {0, 4, 4, 0},

    {8, 8, 8, 0},
    {0, 8, 8, 8},
    {8, 0, 0, 8},
    {0, 8, 8, 0},

    {16, 16, 16, 0},
    {0, 16, 16, 16},
    {16, 0, 0, 16},
    {0, 16, 16, 0},

    /* N: Varous combinations of N with 1.
     */
    {1, 1, 1, 1},
    {1, 1, 1, 1},

    {2, 2, 2, 1},
    {1, 2, 2, 2},
    {2, 1, 1, 2},
    {1, 2, 2, 1},

    {3, 3, 3, 1},
    {1, 3, 3, 3},
    {3, 1, 1, 3},
    {1, 3, 3, 1},

    {4, 4, 4, 1},
    {1, 4, 4, 4},
    {4, 1, 1, 4},
    {1, 4, 4, 1},

    {8, 8, 8, 1},
    {1, 8, 8, 8},
    {8, 1, 1, 8},
    {1, 8, 8, 1},

    {16, 16, 16, 1},
    {1, 16, 16, 16},
    {16, 1, 1, 16},
    {1, 16, 16, 1},
    {FBE_U16_MAX, 0, 0, 0}
};

const char fbe_raid_memory_test_fruts_allocation_short_desc[] = 
"Allocates memory for just fruts";
const char fbe_raid_memory_test_fruts_allocation_long_desc[] = 
"Allocates memory for just fruts.";

/*********************************************************************
 * fbe_raid_memory_test_fruts_allocation()
 *********************************************************************
 * @brief
 *   Tests allocation of fruts.
 *  
 * @param None.
 *  
 * @return
 *   None.
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 ********************************************************************/
static void fbe_raid_memory_test_fruts_allocation(void)
{    
    fbe_u32_t num_fruts;
    fbe_raid_iots_t iots;
    fbe_raid_iots_t *iots_p = &iots;
    fbe_raid_siots_t siots;
    fbe_raid_siots_t *siots_p = &siots;
    fbe_status_t status;
    fbe_raid_geometry_t geo;
    fbe_payload_block_operation_t block_operation;
    fbe_packet_t packet;

    /* Zero the siots so we clear out the siots mem id. 
     * This prevents us from freeing the siots below when we free siots resources. 
     * We don't need to free the siots since we allocated it on the stack. 
     */
    fbe_zero_memory(siots_p, sizeof(fbe_raid_siots_t));
    fbe_zero_memory(iots_p, sizeof(fbe_raid_iots_t));
    status = fbe_transport_initialize_packet(&packet);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_raid_iots_init(iots_p, &packet, &block_operation, &geo, 0, 1);
    //status = fbe_raid_iots_init_common(iots_p, lba, blocks);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                            "raid: memory tests: failed to init iots \n");
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_RAID_LIBRARY_TEST);

    /* Allocate fruts and then free the fruts from the siots freed list..
     */
    for (num_fruts = 1; num_fruts <= (2 * FBE_RAID_MAX_DISK_ARRAY_WIDTH); num_fruts++)
    {
        fbe_memory_hptr_t      *memory_queue_p = NULL;
        fbe_memory_request_t   *memory_request_p = NULL;
        fbe_u32_t               num_pages = 0;

        /* Perform a partial init of the siots.
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);
        status = fbe_raid_siots_embedded_init(siots_p, iots_p); 
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                                 "raid: memory tests: failed to init siots \n");

        fbe_raid_common_enqueue_tail(&iots_p->siots_queue, &siots_p->common);

        /* Need to populate page size 
         */
        fbe_raid_siots_set_optimal_page_size(siots_p, num_fruts, 0);

        /* Take the information we calculated above and determine how this translates 
         * into a number of pages. 
         */
        status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, NULL, /* No sgs to allocate */
                                                    FBE_FALSE /* No nested siots required*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        num_pages = siots_p->num_ctrl_pages;

        status = fbe_raid_siots_allocate_memory(siots_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Make sure the chain has the number we expect.
         */
        memory_request_p = fbe_raid_siots_get_memory_request(siots_p);
        status = fbe_raid_memory_request_get_master_element(memory_request_p, &memory_queue_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(fbe_memory_get_queue_length(memory_request_p), num_pages);

        /* First destroy any resources
         */
        status = fbe_raid_siots_destroy_resources(&siots);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Now free the allocated memory
         */
        status = fbe_raid_siots_free_memory(siots_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_raid_common_queue_remove(&siots_p->common);

        /* make sure we did not leak memory.
         */
        fbe_raid_memory_mock_validate_memory_leaks();
    }
    fbe_raid_iots_destroy(&iots);
    fbe_transport_destroy_packet(&packet);
    return;
}
/*********************************************
 * end fbe_raid_memory_test_fruts_allocation()
 ********************************************/

const char fbe_raid_memory_test_sg_allocation_short_desc[] = 
"Allocates and de-allocates scatter gather lists.";
const char fbe_raid_memory_test_sg_allocation_long_desc[] = 
"Allocates and de-allocates scatter gather lists.";
/*********************************************************************
 * fbe_raid_memory_test_sg_allocation_case()
 *********************************************************************
 * @brief
 *   Tests allocation of fruts.
 *  
 * @param num_sgs_array - An array indexed by sg type,
 *                        it holds number of sgs to allocate for each type.
 *  
 * @return
 *   None.
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 ********************************************************************/
static void fbe_raid_memory_test_sg_allocation_case(fbe_raid_test_sg_allocation_record_t *case_p)
{
    fbe_lba_t lba = 0x7890;
    fbe_block_count_t blocks = 0x12;
    fbe_raid_iots_t iots;
    fbe_raid_iots_t *iots_p = &iots;       
    fbe_raid_siots_t siots;
    fbe_raid_siots_t *siots_p = &siots;
    fbe_status_t status;
    fbe_u16_t *num_sgs_array = &case_p->sg_count[0];
    fbe_memory_hptr_t *memory_queue_p = NULL;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_u32_t num_fruts = 1;
    fbe_u32_t num_pages = 0;

    /* Zero the siots so we clear out the siots mem id. 
     * This prevents us from freeing the siots below when we free siots resources. 
     * We don't need to free the siots since we allocated it on the stack. 
     */
    fbe_zero_memory(siots_p, sizeof(fbe_raid_siots_t));
    fbe_zero_memory(iots_p, sizeof(fbe_raid_iots_t));
    status = fbe_raid_iots_init_common(iots_p, lba, blocks);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                            "raid: memory tests: failed to init iots\n");  
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_RAID_LIBRARY_TEST);   
    status = fbe_raid_siots_embedded_init(siots_p, iots_p); 
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                            "raid: memory tests: failed to init siots\n");

    /* Must setup page size
     */
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, num_sgs_array,    /* No sgs to allocate */
                                                FBE_FALSE /* No nested siots required*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    num_pages = siots_p->num_ctrl_pages;

    status = fbe_raid_siots_allocate_memory(siots_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the chain has the number we expect.
     */
    memory_request_p = fbe_raid_siots_get_memory_request(siots_p);
    status = fbe_raid_memory_request_get_master_element(memory_request_p, &memory_queue_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(fbe_memory_get_queue_length(memory_request_p), num_pages);

    /* First destroy any resources
     */
    status = fbe_raid_siots_destroy_resources(&siots);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now free the allocated memory
     */
    status = fbe_raid_siots_free_memory(siots_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* make sure we did not leak memory.
     */
    fbe_raid_memory_mock_validate_memory_leaks();
    
    return;
}
/*********************************************
 * end fbe_raid_memory_test_sg_allocation_case()
 ********************************************/
/*********************************************************************
 * fbe_raid_memory_test_sg_allocation()
 *********************************************************************
 * @brief
 *   Tests allocation of sgs.
 *   We iterate over the number of cases we are interested in.
 *  
 * @param None.
 *  
 * @return
 *   None.
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 ********************************************************************/
static void fbe_raid_memory_test_sg_allocation(void)
{    
    fbe_raid_test_sg_allocation_record_t *case_p = NULL;

    case_p = &fbe_raid_memory_test_sg_allocation_cases[0];

    /* Loop over test cases until we hit the terminator.
     */
    while (case_p->sg_count[0] != FBE_U16_MAX)
    {
        fbe_raid_memory_test_sg_allocation_case(case_p);
        case_p++;
    }
    return;
}
/*********************************************
 * end fbe_raid_memory_test_sg_allocation()
 ********************************************/

/*!***************************************************************
 * @fn fbe_raid_memory_test_setup(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_memory_test_setup(void)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! @note Increase the trace level below to see debug info
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);

    fbe_raid_group_test_init_memory();

    status = fbe_raid_library_initialize(FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/* end fbe_raid_memory_test_setup() */

/*!***************************************************************
 * @fn fbe_raid_memory_test_teardown(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_memory_test_teardown(void)
{
    fbe_status_t    status = FBE_STATUS_OK;

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

    status = fbe_raid_library_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_raid_group_test_destroy_memory();
    return;
}
/* end fbe_raid_memory_test_teardown() */

/****************************************************************
 * fbe_raid_memory_test_add_tests()
 ****************************************************************
 * @brief
 *   This creates and runs the memory test's suite.
 *
 * @return
 *  None.
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_memory_test_add_tests(mut_testsuite_t *suite_p)
{
    MUT_ADD_TEST_WITH_DESCRIPTION(suite_p, fbe_raid_memory_test_fruts_allocation, fbe_raid_memory_test_setup, fbe_raid_memory_test_teardown,
                                  fbe_raid_memory_test_fruts_allocation_short_desc,
                                  fbe_raid_memory_test_fruts_allocation_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(suite_p, fbe_raid_memory_test_sg_allocation, fbe_raid_memory_test_setup, fbe_raid_memory_test_teardown,
                                  fbe_raid_memory_test_sg_allocation_short_desc,
                                  fbe_raid_memory_test_sg_allocation_long_desc);
    return;
}
/******************************************
 * end fbe_raid_memory_test_add_tests()
 ******************************************/

/*************************
 * end file fbe_raid_memory_test.c
 *************************/
