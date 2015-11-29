/***************************************************************************
 * Copyright (C) EMC Corporation 2008 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_raid_test_sg_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains code to test the following functions:
 *      - fbe_raid_sg_count_sg_blocks() 
 *      - fbe_raid_sg_get_sg_total() 
 *      - fbe_raid_sg_determine_sg_uniform() 
 *      - rg_push_sgid_addresses() 
 *      - fbe_raid_sg_scatter_fed_to_bed() 
 *      - fbe_raid_scatter_cache_to_bed(() 
 *      - fbe_raid_scatter_sg_to_bed()
 *      - fbe_raid_sg_count_uniform_blocks() 
 *      - fbe_raid_sg_count_nonuniform_blocks() 
 *      - fbe_raid_sg_clip_sg_list() 
 *      - rg_clip_buffer_list() 
 *      - fbe_raid_copy_sg_list() 
 *      - fbe_raid_adjust_sg_desc() 
 *      - raid_get_sg_ptr_offset() 
 *      - fbe_raid_fill_invalid_sectors() 
 *
 *  This file also contains following utility functions:
 *      - util_rg_init_sg_list_ap()
 *      - util_rg_init_sg_list_gp()
 *      - util_rg_init_sg_list_seq()
 *      - util_get_block_count_for_sg_type()
 *
 * @author
 *  06/03/08 JR Created.
 *
 ***************************************************************************/

/*************************
 *  INCLUDE FILES 
 ************************/


#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "mut.h"
#include "fbe_raid_test_private.h"
#include "fbe_raid_library.h"


/* This data structure defines the various element size against which 
 * our unit test case will be run. 
 */
fbe_u32_t utest_elsz_type[MAX_TYPE_OF_ELSZ] = {   0x1,   /* small element size */
                                                0x2,   /* small element size */
                                                0x6,   /* small element size */
                                                0x25,  /* small element size */
                                                0x80,  /* std element size */ 
                                                0x100, /* large element size */
                                                0x120  /* large element size */
                                            }; 

fbe_u32_t utest_std_offset_pos[MAX_OFFSET_POSITION] = { 0, 1, 25, 75, 100, 127 };


fbe_sg_element_t utest_sg_pool[FBE_RAID_MAX_DISK_ARRAY_WIDTH][FBE_RAID_MAX_SG_ELEMENTS];
fbe_raid_memory_header_t utest_mv_pool[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
fbe_sector_t utest_sector_pool[FBE_RAID_MAX_DISK_ARRAY_WIDTH];


/*****************************************************************************
 * util_rg_init_sg_list_ap()
 *****************************************************************************
 * @brief
 *   Initializes the SG list's elements with block count. Block count 
 *   is decided as per the arithmetic progression except the last
 *   element of list which will be used as terminator.
 *  
 *

 *   sgl [IO] - list of SG elements. last element of the list as terminator.
 *   sg_count [I] - number of SG element(s). 
 *   start [I] - block count of first SG element
 *   delta [I] - differnce of arithmetic progression
 *   total_blocks [O] - total number of blocks addressed by the SG list.
 *   
 *  
 * @return
 *   None.
 *   
 * @author
 *  6/17/2008 - Created. JR
 * 
 ****************************************************************************/
void util_rg_init_sg_list_ap(fbe_sg_element_t *sgl,
                             const fbe_u32_t sg_count,                                                                      
                             fbe_u32_t start,
                             fbe_s32_t delta,
                             fbe_u32_t *total_blocks)
{
    fbe_u32_t sg_index, num_of_elements, curr_val;
   
    num_of_elements = sg_count-1; /* last SG element will be used as terminator */    
    curr_val = start;
    for(sg_index = 0, *total_blocks = 0; sg_index < num_of_elements; sg_index++)
    {                       
        sgl[sg_index].count = curr_val*FBE_BE_BYTES_PER_BLOCK;
        sgl[sg_index].address = (fbe_u8_t*)&sgl[sg_index];

        *total_blocks += curr_val;
        curr_val = start  + (sg_index+1) * delta;        
    }

    sgl[sg_index].count = 0; /* last element should have count as zero. */
    sgl[sg_index].address = NULL; /* last element should have address as NULL. */

    return;
}
/******************************************
 * end util_rg_init_sg_list_ap()
 ******************************************/

/*****************************************************************************
 * util_rg_init_sg_list_gp()
 *****************************************************************************
 * @brief
 *   Initializes the SG list's elements with block count. Block count 
 *   is decided as per the geometric progression except the last
 *   element of list which will be marked as terminator.
 *  
 *

 *   sgl [IO] - list of SG elements. last element of the list as terminator.
 *   sg_count [I] - number of SG element(s). 
 *   start [I] - block count of first SG element
 *   ratio [I] - ratio of geometric progression
 *   total_blocks [O] - total number of blocks addressed by the SG list.
 *   
 *  
 * @return
 *   None.
 *   
 * @author
 *  6/15/2008 - Created. JR
 * 
 ****************************************************************************/
void util_rg_init_sg_list_gp(fbe_sg_element_t *sgl,
                             const fbe_u32_t sg_count,                                                                      
                             fbe_u32_t start,
                             fbe_u32_t ratio,
                             fbe_u32_t *total_blocks)
{
    fbe_u32_t sg_index, num_of_elements, curr_val;
    
    num_of_elements = sg_count-1; /* last SG element will be used as terminator */    
    curr_val = start;
    for(sg_index = 0, *total_blocks = 0; sg_index < num_of_elements; sg_index++)
    {               
        sgl[sg_index].count = curr_val*FBE_BE_BYTES_PER_BLOCK;        
        sgl[sg_index].address = (fbe_u8_t*)&sgl[sg_index];

        *total_blocks += curr_val;
        curr_val = curr_val * ratio;        
    }
    
    sgl[sg_index].count = 0; /* last element should have count as zero. */
    sgl[sg_index].address = NULL; /* last element should have address as NULL. */

    return;
}
/******************************************
 * end util_rg_init_sg_list_gp()
 ******************************************/



/*****************************************************************************
 * util_rg_init_sg_list_seq()
 *****************************************************************************
 * @brief 
 *   Initializes the SG list's elements with block count. Block count is 
 *   decided as per the series provided. Series will be repeated if number 
 *   of elements are more than number of elements of the series.
 *  
 *

 *   sgl [IO] - list of SG elements. last element of the list as terminator.
 *   sg_count [I] - number of SG element(s). 
 *   sequence [I] - series of the block counts
 *   count [I] - number of the elements of the sequence
 *   total_blocks [O] - total number of blocks addressed by the SG list.
 *   
 *  
 * @return
 *   None.
 *   
 * @author
 *  6/20/2008 - Created. JR
 * 
 ****************************************************************************/
void util_rg_init_sg_list_seq(fbe_sg_element_t *sgl,
                              const fbe_u32_t sg_count,                                                                      
                              fbe_u32_t sequence[],
                              fbe_u32_t count,
                              fbe_u32_t *total_blocks)
{
    fbe_u32_t sg_index, num_of_elements, s_index;
    
    num_of_elements = sg_count-1; /* last SG element will be used as terminator */        
    s_index = 0;
    for(sg_index = 0, *total_blocks = 0; sg_index < num_of_elements; sg_index++)
    {               
        sgl[sg_index].count = sequence[s_index] * FBE_BE_BYTES_PER_BLOCK;                
        *total_blocks += sequence[s_index];       
        s_index = (s_index == count)? 0 : s_index;
    }
    
    sgl[sg_index].count = 0; /* last element should have count as zero. */
    sgl[sg_index].address = NULL; /* last element should have address as NULL. */

    return;
}
/******************************************
 * end util_rg_init_sg_list_seq()
 ******************************************/


/*********************************************************************
 * util_get_block_count_for_sg_type()
 *********************************************************************
 * @brief
 *   The function determines the minimum block transfer count for 
 *   a given SG type, block per buffer and remaining blocks in 
 *   previous SG list. It is used to get the number of blocks for 
 *   which we will need to create SG list of given type.
 *

 *   sg_type - type of SG list. RAID driver maintains 4 type of SG 
 *             list (sg list type - 1, 9, 33 and 129).
 *   blks_per_buffer - buffer size in number of blocks
 *   blks_ramining - number of blocks which can be adjusted in 
 *                   previous SG list.
 *  
 * @return
 *   Number of blocks which will require SG list of give SG_type,
 *   blks_per_buffer and blks_remaining.
 *   
 *
 * @author
 *  6/12/2008 - Created. JR
 * 
 ********************************************************************/
fbe_u32_t util_get_block_count_for_sg_type(const fbe_u32_t sg_type,
                                         const fbe_u16_t blks_per_buffer,
                                         const fbe_u16_t blks_remaining)
{
    fbe_u32_t sg_count, blks, curr_sg_type;
    fbe_bool_t b_search_complete = FBE_FALSE;

    
    blks = 1;
    while(!b_search_complete)
    {
         sg_count = 0; /* the function fbe_raid_sg_count_uniform_blocks() will add
                        * SG elements needed in sg_count irrespective of its 
                        * initial values. But we are interested in difference
                        * only.
                        */
         fbe_raid_sg_count_uniform_blocks(blks,
                                    blks_per_buffer,
                                    blks_remaining,
                                    &sg_count);
         
         curr_sg_type = fbe_raid_memory_sg_count_index(sg_count); 
         if (curr_sg_type == sg_type)
         {
             b_search_complete = FBE_TRUE;
         }
         else
         {
             /* we need more blocks to create an SG list of given SG type 
              * (i.e. sg_type). And hence search further with incremental
              * blocks.
              */            
             blks += blks_per_buffer; 
         }
    }

    return blks;
}
/******************************************
 * end util_get_block_count_for_sg_type()
 ******************************************/

/*********************************************************************
 * fbe_raid_group_test_count_sg_blocks()
 *********************************************************************
 * @brief
 *   runs unit test case(s) against fbe_raid_sg_count_sg_blocks().
 *  
 *

 *  None.
 *  
 * @return
 *  None.
 *   
 *
 * @author
 *  6/12/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_count_sg_blocks(void)
{
    fbe_u32_t count;
    fbe_u32_t total;
    fbe_block_count_t num_of_blocks;
    fbe_status_t status = FBE_STATUS_OK;
    for(count=1; count<FBE_RAID_MAX_SG_ELEMENTS; count++)
    {
        /* initialize the SG list so that sg elements contain 
         * the blocks as per the pattern. Also match the 
         * validate the implementation of fbe_raid_sg_count_sg_blocks().
         */
        util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                                count,
                                1,
                                2,                                
                                &total);

         status = fbe_raid_sg_count_sg_blocks(&utest_sg_pool[0][0],&count, &num_of_blocks);
         MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "raid_group: Bad status from fbe_raid_sg_count_sg_blocks");
         if (num_of_blocks != total)
         {
             MUT_FAIL_MSG("num_of_blocks did not match with total.");        
         }
    }

    return;
}
/******************************************
 * end fbe_raid_group_test_count_sg_blocks()
 ******************************************/



/*********************************************************************
 * fbe_raid_group_test_sg_count_nonuniform_blocks()
 *********************************************************************
 * @brief
 *   runs unit test case(s) against fbe_raid_sg_count_nonuniform_blocks().
 *

 *  None.
 *  
 * @return
 *  None.
 *   
 *
 * @author
 *  6/12/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_sg_count_nonuniform_blocks(void)
{
    fbe_u32_t count, total, blks_remaining;
    
    /* the function fbe_raid_sg_count_nonuniform_blocks() presumes the following:
     *     1. number of blocks to be scattered i.e. blks_to_count will 
     *        not exceed the number which can be addressed by given 
     *        SG list.
     *     2. SG list must be valid one.
     * And, hence we will not executes the test case to validate above
     * points.
     */


    /* we must pass a valid SG list i.e. SG list having at least two element
     * one SG element for data address + second terminator).
     */
    for(count = 2; count < FBE_RAID_MAX_SG_ELEMENTS; count++)
    {
        UINT32 start, delta, end_val;

        start = 5,
        delta = 2;
        end_val = start + (count - 1) * delta;

        /* iniitalise SG list with incremental number of blocks as per 
         * the arithemetic progression. 
         */
        util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                                count,
                                start,
                                delta,
                                &total);
        
        /* first SG element contains 'start' no of blocks and hence 
         * blks_remaining has to between 1 and start.
         */
        for(blks_remaining = 1; blks_remaining <= start; blks_remaining++)
        {
            fbe_status_t status = FBE_STATUS_OK;
            fbe_u32_t blks;
            fbe_u32_t blks_to_scatter = total - blks_remaining; /* number of blocks to be 
                                                               * scattered by SG list
                                                               * reduces as per the 
                                                               * value of blks_remaining.
                                                               */
            for(blks = 0; blks < blks_to_scatter; blks++)
            {
                fbe_sg_element_with_offset_t sg_descriptor;
                fbe_u32_t sg_total = 0;
                fbe_block_count_t remainder = 0;
                fbe_sg_element_t *src_sg_ptr = &utest_sg_pool[0][0];

                fbe_sg_element_with_offset_init(&sg_descriptor, 0, src_sg_ptr, NULL);
                remainder = blks_remaining;
                status = fbe_raid_sg_count_nonuniform_blocks(blks,
                                                             &sg_descriptor,
                                                             FBE_BE_BYTES_PER_BLOCK,
                                                             &remainder,
                                                             &sg_total);

                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "raid_group: Bad status from fbe_raid_sg_count_nonuniform_blocks");
                if (blks <= 0)
                {
                    MUT_ASSERT_TRUE(sg_total == 0);
                    MUT_ASSERT_TRUE(remainder == blks_remaining);
                }
                else
                {
                    MUT_ASSERT_TRUE((sg_total >= 1) && (sg_total < count));                    
                    /* we can not have a scenario where remainder is more than
                     * maximum block number addressed by a SG element in given 
                     * SG list.
                     */
                    MUT_ASSERT_TRUE(remainder <= end_val);                                                           
                }
            } /* end for(blks = 1; blks < blks_to_scatter; block++) */
         } /* end for(blks_remaining = 1, blks_remaining <= start; blks_remaining++) */
    } /* end for(count = 2; count < FBE_RAID_MAX_SG_ELEMENTS; count++) */
   
    return;
}
/***********************************************
 * end fbe_raid_group_test_sg_count_nonuniform_blocks()
 **********************************************/

/*********************************************************************
 * fbe_raid_group_test_sg_get_sg_total()
 *********************************************************************
 * @brief
 *  runs unit test case against fbe_raid_sg_get_sg_total(). 
 *  
 *

 *  None.
 *  
 * @return
 *  None.
 *   
 * @author
 *  6/15/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_sg_get_sg_total(void)
{         
    fbe_u32_t sg_type, sg_list;    
    fbe_memory_id_t *sg_id_list[FBE_RAID_MAX_SG_TYPE];
    fbe_memory_id_t sg_id_address[MAX_BM_MEMORY_ID]; 
    fbe_status_t status = FBE_STATUS_OK;

    /* initialize SG list of each type to NULL.
     */
    for(sg_type = 0; sg_type < FBE_RAID_MAX_SG_TYPE; sg_type++)
    {
        sg_id_list[sg_type] = NULL;
    }


    for(sg_type = 0; sg_type < FBE_RAID_MAX_SG_TYPE; sg_type++)
    {
        fbe_u32_t blocks = 0;

        MUT_ASSERT_TRUE(fbe_raid_sg_get_sg_total(sg_id_list, sg_type) == 0);

        /* reset the content of elements of sg_id_address[] before 
         * running the test case.
         */
        for(sg_list = 0; sg_list < MAX_BM_MEMORY_ID; sg_list++)
        {
            sg_id_address[sg_list] = 0;
        }

        /* plant or push SG list in the chain maintained in sg_id_list[]
         */
        for(sg_list = 0; sg_list < MAX_BM_MEMORY_ID; sg_list++)
        {
            fbe_block_count_t remaining_count = 0;
            /* push an sg list into the sg_id chain maintianed 
             * in sg_id_list[] for the given type.
             */
            blocks = util_get_block_count_for_sg_type(sg_type, 
                                                      fbe_raid_get_std_buffer_size(),
                                                      0);    
            status = fbe_raid_sg_determine_sg_uniform(blocks,
                                    &sg_id_address[sg_list],
                                    0, /* choosen 0, its redundant as sg_count_vec[] is passed as NULL. */
                                    fbe_raid_get_std_buffer_size(),
                                    &remaining_count, /* assumption: there were no previous sg list with empty slot. */
                                    sg_id_list,
                                    NULL);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "raid_group: bad status from fbe_raid_sg_determine_sg_uniform");
            MUT_ASSERT_INT_EQUAL_MSG(sg_list+1,
                                      fbe_raid_sg_get_sg_total(sg_id_list, sg_type),
                                      "$sg_id_list[] does not contain the expected number of sg list.");                                   
        } /* end for(sg_list = 1; sg_list <= MAX_SG_LIST; sg_list++) */
    } /* end for(sg_type = 0; sg_type < FBE_RAID_MAX_SG_TYPE; sg_type++) */
    
    return;
}
/******************************************
 * end fbe_raid_group_test_sg_get_sg_total()
 ******************************************/



/*********************************************************************
 * fbe_raid_group_test_determine_sg_uniform()
 *********************************************************************
 * @brief
 *   runs unit test case against fbe_raid_sg_determine_sg_uniform().
 *  
 *

 *  NONE
 *  
 * @return
 *   NONE
 *   
 *
 * @author
 *  5/19/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_determine_sg_uniform(void)
{    
    fbe_memory_id_t *sg_id_list[FBE_RAID_MAX_SG_TYPE];
    fbe_memory_id_t sg_id_address[MAX_BM_MEMORY_ID]; 
    fbe_u32_t sg_count_vec[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t sg_type, sg_id, fru_pos, sg_list;
    fbe_status_t status = FBE_STATUS_OK;
    
    /* initialize SG list of each type to NULL.
     */
    for(sg_type = 0; sg_type < FBE_RAID_MAX_SG_TYPE; sg_type++)
    {
        sg_id_list[sg_type] = NULL;
    }


    /* We will push the SG id in a given SG list type (1/9/33/129).
     * We will push fixed number of SG list and we will validate
     * whether it is  inserted in expected way or not.
     */
    for (sg_type = 0; sg_type < FBE_RAID_MAX_SG_TYPE; sg_type++)
    {   
        fbe_u32_t blocks = 0;
        fbe_u32_t curr_bm_id_count = 0;
        fbe_memory_id_t *curr_sg_id = NULL;


        sg_count_vec[0] = 0;
        sg_id_list[sg_type] = NULL;
        fru_pos = 0;

        blocks = util_get_block_count_for_sg_type(sg_type,
                                                  fbe_raid_get_std_buffer_size(),
                                                  0);
            
        /* Initialize our sg id addresses.
         */
        for(sg_list = 0; sg_list < MAX_BM_MEMORY_ID; sg_list++)
        {
            sg_id_address[sg_list] = 0;
        }
            
        /* iterate the loop to push the BM_MEMORY_ID into 
         * appropriate SG list maintianed in sg_id_list[]. 
         * The appropriate position is determined by the type 
         * of SG list (viz. 1, 9, 33 and 129).
         */
         for(sg_id = 0; sg_id < MAX_BM_MEMORY_ID; sg_id++)
         {    
            fbe_block_count_t remaining_count = 0;
            status = fbe_raid_sg_determine_sg_uniform(blocks,
                                   &sg_id_address[sg_id],
                                   fru_pos,
                                   fbe_raid_get_std_buffer_size(),
                                   &remaining_count,
                                   sg_id_list,
                                   sg_count_vec);
                
             MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                                      "raid_group: bad status from fbe_raid_sg_determine_sg_uniform");
             /* compute the number of bm_id present in the given SG list.
              */
             curr_sg_id = (fbe_memory_id_t*)sg_id_list[sg_type];
             curr_bm_id_count = 0;
             while(curr_sg_id != NULL)
             {
                 curr_bm_id_count += 1;
                 curr_sg_id = (fbe_memory_id_t*)(*curr_sg_id);
             }

             /* the number of inserted bm_id for the given list of SG_type must 
              * be equal to sg_id.
              */
             MUT_ASSERT_INT_EQUAL_MSG(sg_id+1, 
                                      curr_bm_id_count,
                                      "unexpected number of inserted bm_id."); 

            /* the current fbe_memory_id_t must be inserted in SG list 
             * indicated by sg_type. 
             */
             MUT_ASSERT_TRUE(&sg_id_address[sg_id] == (fbe_memory_id_t*)(sg_id_list[sg_type])); 

        } /* end for(sg_id = 0; sg_id < MAX_BM_MEMORY_ID; sg_id++) */
    }

    return;
}
/******************************************
 * end fbe_raid_group_test_determine_sg_uniform()
 ******************************************/




/*********************************************************************
 * fbe_raid_group_test_scatter_fed_to_bed()
 *********************************************************************
 * @brief
 *   runs unit test case against fbe_raid_sg_scatter_fed_to_bed(). 
 *  
 *

 *  None.
 *  
 * @return
 *  None.   
 *
 * @author
 *  5/19/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_scatter_fed_to_bed(void)
{           
    fbe_lba_t start_lba;
    fbe_u32_t data_disk, data_pos, blocks;   
    fbe_u32_t num_sg_elements[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t elsz_index, sg_index;
    fbe_u32_t current_data_disk;

    /* the unit test case written below executes the test case 
     * for blocks of different size for a given lun geometry.
     * element size is also varied although customer array is 
     * configured for standard element size. 
     */     
    for(data_disk = 1; data_disk <= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; data_disk++)
    {
        for (current_data_disk = 0; current_data_disk < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; current_data_disk++)
        {
            num_sg_elements[current_data_disk] = INVALID_VAL;
        }

        /* vary the data position within the given data width.
         */
        for(data_pos = 0; data_pos < data_disk; data_pos++)
        {            
            /* vary the element size of the lun geometry. 
             */
            for(elsz_index = 0; elsz_index < MAX_TYPE_OF_ELSZ; elsz_index++)
            {
                /* vary the start_lba within the given element. unit test case 
                 * is run for the folloiwng case:
                 *      1. start_lba is at the begining of given element
                 *      2. start_lba is at the middle of given element
                 */
                fbe_u32_t elsz = utest_elsz_type[elsz_index];
                for(start_lba = 0; start_lba < elsz; start_lba += ((elsz>4) ? (elsz/4) : 1))
                {
                    for(blocks = 0; blocks <= MAX_TRANSFER_BLOCKS; blocks += 8)
                    {
                        fbe_raid_sg_scatter_fed_to_bed(start_lba,
                                              (fbe_block_count_t)blocks,
                                              data_pos,
                                              data_disk, 
                                              elsz,
                                              (fbe_block_count_t)(fbe_raid_get_std_buffer_size()),
                                              0, /* fixed to 0 */
                                              num_sg_elements);
                    } /* end for(blocks = 1; blocks <= MAX_TRANSFER_BLOCKS; blocks++) */
                } /* end for(esize = 1; esize <= MAX_ELEMENT_SIZE; esize++) */
            } /* end for(start_lba = 0; start_lba < element_size; start_lba++) */
        } /* end for(data_pos = 0; data_pos < data_disks; data_pos++) */

        /* validate that no element is scattered beyond the data width
         */
        for(sg_index = data_disk; sg_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; sg_index++)
        {
            MUT_ASSERT_TRUE(num_sg_elements[sg_index] == INVALID_VAL);
        }
    } /* end for(data_disks = 1; data_disks <= MAX_DATA_DISKS; data_disks++) */
    
    return;
}
/******************************************
 * end fbe_raid_group_test_scatter_fed_to_bed()
 *****************************************/





/*********************************************************************
 * fbe_raid_group_test_scatter_cache_to_bed()
 *********************************************************************
 * @brief
 *   runs unit test case against fbe_raid_scatter_cache_to_bed((). 
 *  
 *

 *  None.
 *  
 * @return
 *  None.   
 *
 * @author
 *  5/19/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_scatter_cache_to_bed(void)
{           
    #define NUM_SG_ELEMENTS 6
  
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t start_lba;
    fbe_u32_t data_disk, data_pos, blocks, total;   
    fbe_u32_t num_sg_elements[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t elsz_index, sg_index;
    fbe_sg_element_t sg_list[NUM_SG_ELEMENTS];
    fbe_u32_t current_data_disk;
    
    
    /* Define the offset test data keeping in mind the block count
     * addressed by above SG list. Remember that block counts in 
     * successive SG element follows arithmetic progression.
     */
    fbe_s32_t offset_vec[]={  FBE_BE_BYTES_PER_BLOCK*0,
                           FBE_BE_BYTES_PER_BLOCK*1,
                           FBE_BE_BYTES_PER_BLOCK*5,
                           FBE_BE_BYTES_PER_BLOCK*12,
                           FBE_BE_BYTES_PER_BLOCK*22
                        };

    /* Initialise SG list as per arithmetic progression. 
     */
    util_rg_init_sg_list_ap(sg_list,
                            NUM_SG_ELEMENTS,
                            1,
                            3,
                            &total); 

    /* the unit test case written below executes the test case 
     * for blocks of different size for a given lun geometry.
     * element size is also varied although customer array is 
     * configured for standard element size. 
     */     
    for(data_disk = 1; data_disk <= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; data_disk++)
    {
        for (current_data_disk = 0; current_data_disk < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; current_data_disk++)
        {
            num_sg_elements[current_data_disk] = INVALID_VAL;
        }
        /* Vary the data position within the given data width.
         */
        for(data_pos = 0; data_pos < data_disk; data_pos++)
        {            
            /* Vary the element size of the lun geometry. 
             */
            for(elsz_index = 0; elsz_index < MAX_TYPE_OF_ELSZ; elsz_index++)
            {
                /* Vary the start_lba within the given element. 
                 */
                fbe_u32_t elsz = utest_elsz_type[elsz_index];
                for(start_lba = 0; 
                    start_lba < elsz; 
                    start_lba +=((elsz>4)?(elsz/4):1))
                {
                    /* We can not scatter number of blocks MORE than
                     * provided be cache SG list. Number of blocks
                     * should be computed keeping offset in mind.
                     */
                    fbe_u32_t offset_index;
                    fbe_u32_t max_offset_count = sizeof(offset_vec) / sizeof(fbe_u32_t);

                    for(offset_index = 0;
                        offset_index < max_offset_count; 
                        offset_index++)
                    {
                        fbe_u32_t max_blocks_to_scatter = (total * 
                                                 FBE_BE_BYTES_PER_BLOCK - 
                                                 offset_vec[offset_index]) / 
                                                 FBE_BE_BYTES_PER_BLOCK;
                        for(blocks = 0; blocks <= max_blocks_to_scatter; blocks++)
                        {
                            fbe_sg_element_with_offset_t cache_sg_desc;
                            fbe_sg_element_with_offset_init(&cache_sg_desc,
                                                            offset_vec[offset_index],
                                                            &sg_list[0],
                                                            NULL);
                            status = fbe_raid_scatter_cache_to_bed(start_lba,
                                                    (fbe_block_count_t)blocks,
                                                    data_pos,
                                                    data_disk, 
                                                    elsz,
                                                    &cache_sg_desc,
                                                    num_sg_elements);
                            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
                                                    status,
                                                    "Bad status from fbe_raid_scatter_cache_to_bed"); 
                        } /* end for(blocks = 1; blocks <= max_blocks_to_scatter; blocks++) */
                    } /* end for(offset_index = 0; offset_index < offset_count; offset_index++) */
                } /* end  for(esize = 1; esize <= MAX_ELEMENT_SIZE; esize++) */
            } /* end for(start_lba = 0; start_lba < element_size; start_lba++) */

            /* validate that no element is scattered beyond the data width
             */
            for (sg_index = data_disk + 1; sg_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; sg_index++)
            {
                MUT_ASSERT_TRUE(num_sg_elements[sg_index] == INVALID_VAL);
            } 
        } /* end for(data_pos = 0; data_pos < data_disks; data_pos++) */
    } /* end for(data_disks = 1; data_disks <= MAX_DATA_DISKS; data_disks++) */
    
    return;
}
/******************************************
 * end fbe_raid_group_test_scatter_cache_to_bed()
 ******************************************/


/****************************************************************************
 * fbe_raid_group_test_scatter_sg_to_bed()
 ****************************************************************************
 * @brief
 *   executes unit test case against fbe_raid_scatter_sg_to_bed().
 *  

 *  None.
 *  
 * @return
 *  None.
 *  
 * NOTE:
 * the function fbe_raid_scatter_sg_to_bed()presumes that we pass valid 
 * address of SG list. It does not take care if caller 
 * passes NULL instead of pointer to SG list. And hence we do not
 * execute the test case to validate these scenario.
 *
 * @author
 *  6/18/2008 - Created. MA
 * 
 ****************************************************************************/
static void fbe_raid_group_test_scatter_sg_to_bed(void)
{
    #define MAX_DATA_DISKS      (FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH-1)
    #define MAX_BLOCKS_PER_IO   100
    #define MAX_fbe_lba_tEST        0xFFFFFFFF
    #define BLOCKS_PER_ELEMENT  0x80

    fbe_lba_t lda;
    fbe_u16_t data_pos;
    fbe_u16_t data_disks;
    fbe_u32_t sg_index,index;
    fbe_lba_t blks_per_element;

    fbe_sg_element_t * sgl_ptr;
    fbe_sg_element_with_offset_t src_sgd_ptr[1];
    fbe_u32_t blks_to_scatter, total;
    fbe_sg_element_t *dest_sgl_ptrv[MAX_DATA_DISKS];

    fbe_sg_element_t utest_src_sg_pool[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH][FBE_RAID_MAX_SG_ELEMENTS];

    /* Testing the functions by varying followings: 
            (1) lda.
            (2) blks_to_scatter.
            (3) data_pos.
            (4) data_disks.
     */

    blks_per_element = BLOCKS_PER_ELEMENT;

    /* Source SG list
     */
    util_rg_init_sg_list_ap(&utest_src_sg_pool[0][0],
                              FBE_RAID_MAX_SG_ELEMENTS,
                                    2,
                                    4,
                                  &total);

    sgl_ptr = &utest_src_sg_pool[0][0];

    /* Destination SG list Vector 
     */
    for (index=0; index<15; index++)
    {
        util_rg_init_sg_list_ap(&utest_sg_pool[index][0],
                                        1,
                                        0,
                                        0,
                                      &total);
    }

    /* we need to initalize the dest_sgl_ptrv here
     */
    for(sg_index=0; sg_index< MAX_DATA_DISKS ; sg_index++)
    {
        dest_sgl_ptrv[sg_index] = utest_sg_pool[sg_index];
    }

    for(data_disks=1;data_disks<=MAX_DATA_DISKS;data_disks++)
    {
        for(blks_to_scatter=1;blks_to_scatter<=total;blks_to_scatter++)
        {
            for(lda=0x0;lda<= MAX_fbe_lba_tEST;lda++)
            {
                for(data_pos=0;data_pos<=data_disks;data_pos++)
                {
                    /* initialize sg descriptor
                     */
                    fbe_sg_element_with_offset_init(&src_sgd_ptr[0],
                                                    0, sgl_ptr, NULL);

                    fbe_raid_scatter_sg_to_bed(lda,
                                         blks_to_scatter,
                                         data_pos,
                                         data_disks,
                                         blks_per_element,
                                         &src_sgd_ptr[0],
                                         dest_sgl_ptrv);

                    MUT_ASSERT_TRUE(dest_sgl_ptrv[data_pos]!= NULL);

                }/* end for(data_pos=0;data_p...) */
            }/* end for(lda=0x0;lda<=...) */
        }/* end for(blks_to_scatter=1;blks_to_scatter........)*/

        for(index = (data_disks+1) ;index <MAX_DATA_DISKS ; index++)
        {
            MUT_ASSERT_TRUE(dest_sgl_ptrv[index][0].count == 0);
            MUT_ASSERT_TRUE(dest_sgl_ptrv[index][0].address == NULL);
        }

    }/* end for(data_disks=1;data_d...)*/

    return;
} 
/****************************************************************************
 * End of fbe_raid_group_test_scatter_sg_to_bed()
*****************************************************************************/




/*********************************************************************
 * fbe_raid_group_test_sg_count_uniform_blocks()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_sg_count_uniform_blocks().
 *  
 *

 *  NONE
 *  
 * @return
 *   NONE
 *   
 *
 * @author
 *  5/19/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_sg_count_uniform_blocks(void)
{    
    fbe_block_count_t blk, blk_remain;
    fbe_u32_t max_remaining_blks;
    fbe_u32_t sg_init_val, sg_count; 

    for(blk = 0; blk < MAX_TRANSFER_BLOCKS; blk++)
    {      
        fbe_block_count_t remainder;
  
        sg_count = 0;

        /* we will test the function for all the case of remaining
         * blocks viz 0..(fbe_raid_get_std_buffer_size()-1).
         */
        max_remaining_blks = blk % fbe_raid_get_std_buffer_size(); 
        for(blk_remain = 0; blk_remain <= max_remaining_blks; blk_remain++)
        {
            sg_init_val = sg_count;
            remainder = fbe_raid_sg_count_uniform_blocks(blk,
                                                   (fbe_block_count_t)fbe_raid_get_std_buffer_size(),
                                                   blk_remain,
                                                   &sg_count);
            
            if (blk > 0)
            {
                /* sg_count must be greater than sg_init_val if number 
                 * of block is non-zero.
                 */
                MUT_ASSERT_TRUE(sg_count > sg_init_val);
            }
            else
            {
                /* sg_count must be equal to sg_init_val if number of 
                 * block is less than or equal to zero.
                 */
                MUT_ASSERT_TRUE(sg_count == sg_init_val);
                MUT_ASSERT_TRUE(blk_remain == remainder);
            }

            MUT_ASSERT_TRUE_MSG((remainder < fbe_raid_get_std_buffer_size()),
                                "blks remaining can not be greater than fbe_raid_get_std_buffer_size().");
        }
    }

    return;
}
/******************************************
 * end fbe_raid_group_test_sg_count_uniform_blocks()
 ******************************************/



/*********************************************************************
 * fbe_raid_group_test_copy_sg_list()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_copy_sg_list().
 *  

 *  None.
 *  
 * @return
 *  None.
 *  
 * NOTE:
 * the function fbe_raid_copy_sg_list() presumes that we pass valid 
 * address of SG list. It does not take care if caller 
 * passes NULL instead of pointer to SG list. And hence we do not
 * execute the test case to validate these scenario.
 *
 * @author
 *  6/18/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_copy_sg_list(void)
{
    fbe_u32_t sg_test_count, element_count, sg_index,total_blocks;
    fbe_sg_element_t src_sg_list[FBE_RAID_MAX_SG_ELEMENTS], dest_sg_list[FBE_RAID_MAX_SG_ELEMENTS];
    fbe_sg_element_t *end_sg_element;
    fbe_u32_t sg_test_vec[] = {1, 25, 129};
    
        
    sg_test_count = sizeof(sg_test_vec)/sizeof(fbe_u32_t);
    for(sg_index = 0; sg_index < sg_test_count; sg_index++)
    {
        /* initialize the source SG list as per the arithmetic 
         * progression.
         */
        util_rg_init_sg_list_ap(src_sg_list, 
                                sg_test_vec[sg_index], 
                                1,
                                2, 
                                &total_blocks);

        end_sg_element = fbe_raid_copy_sg_list(src_sg_list, dest_sg_list); 
       
        MUT_ASSERT_TRUE_MSG(end_sg_element->count == 0, "last element in SG list must have count as 0.");        
        MUT_ASSERT_TRUE_MSG(end_sg_element->address == NULL, "last element in SG list must have address as NULL.");    

        /* validate the data of destination list with respect to source list
         */        
        for(element_count = 0; element_count < sg_test_vec[sg_index]; element_count++)
        {
             MUT_ASSERT_INT_EQUAL(src_sg_list[element_count].count, dest_sg_list[element_count].count);
             MUT_ASSERT_TRUE(src_sg_list[element_count].address == dest_sg_list[element_count].address);           
        }
    } /* end for(sg_index = 0; sg_index < sg_test_count; sg_index++) */

    return;
}
/******************************************
 * end fbe_raid_group_test_copy_sg_list()
 ******************************************/


/*********************************************************************
 * fbe_raid_group_test_adjust_sg_desc()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_adjust_sg_desc().
 *  

 *  None.
 *  
 * @return
 *  None.
 *  
 * NOTE:
 *
 * @author
 *  6/18/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_adjust_sg_desc(void)
{
    fbe_u32_t sg_index, total_blocks;
    fbe_u32_t start, delta;
   
    total_blocks = 0;
    start = 2, delta = 3;
    util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                            FBE_RAID_MAX_SG_ELEMENTS, 
                            start,
                            delta, 
                            &total_blocks); 

    /* the test case will be executed with an offset for the
     * given SG element and will expect the function 
     * fbe_raid_adjust_sg_desc() to adjust starting pointer of 
     * sg element of the list.
     */
    for(sg_index = 0; sg_index < FBE_RAID_MAX_SG_ELEMENTS - 1; sg_index++)
    {
        fbe_u32_t nth_element, sum;
        fbe_sg_element_with_offset_t sg_desc;   

        /* compute the block count addressed by nth SG element of the
         * list as well as total no of block addressed by SG list till
         * nth element.
         */
        nth_element = start + sg_index*delta; 
        sum = ((start + nth_element)*(sg_index+1))/2; 

        fbe_sg_element_with_offset_init(&sg_desc,
                                        (sum-1)* FBE_BE_BYTES_PER_BLOCK,
                                        &utest_sg_pool[0][0],
                                        NULL /* use default get next fn */);
        fbe_raid_adjust_sg_desc(&sg_desc); /* the function presumes that caller will never
                                        * pass sg_desc.offset greater than number
                                        * of bytes addressed by a given SG list. And
                                        * we will not run the test case where
                                        * sg_desc.offset > sum of all bytes addressed
                                        * by given SG list.
                                        */
 
        MUT_ASSERT_TRUE(sg_desc.sg_element == &utest_sg_pool[0][sg_index]);
        MUT_ASSERT_TRUE(nth_element == utest_sg_pool[0][sg_index].count/FBE_BE_BYTES_PER_BLOCK);
     }

        
    return;
}
/******************************************
 * end fbe_raid_group_test_adjust_sg_desc()
 ******************************************/



/*********************************************************************
 * fbe_raid_group_test_get_sg_ptr_offset()
 *********************************************************************
 * @brief
 *   executes unit test case against raid_get_sg_ptr_offset().
 *  

 *  None.
 *  
 * @return
 *  None.
 *  
 * NOTE:
 *  None.
 *
 * @author
 *  7/8/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_get_sg_ptr_offset(void)
{
    fbe_u32_t element_no;
    fbe_u32_t start, delta, total_blocks;
   
    total_blocks = 0;
    start = 2, delta = 3;
    util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                            FBE_RAID_MAX_SG_ELEMENTS, 
                            start,
                            delta, 
                            &total_blocks); 

    
    for(element_no = 1; element_no < FBE_RAID_MAX_SG_ELEMENTS; element_no++)
    {
        fbe_u32_t nth_element, sum_of_elements;
        fbe_u32_t curr_block_no, start_block_no, end_block_no;
        
        nth_element = start + (element_no - 1) * delta;    
        sum_of_elements = ((element_no) * (start+nth_element)) / 2;

        start_block_no = sum_of_elements - nth_element;        
        end_block_no = sum_of_elements - 1;
        
        for(curr_block_no = start_block_no; 
            curr_block_no <= end_block_no; 
            curr_block_no++)
        {
            fbe_u32_t block_offset;
            fbe_sg_element_t *sg_ptr;
            
            sg_ptr = &utest_sg_pool[0][0];        
            block_offset = fbe_raid_get_sg_ptr_offset(&sg_ptr, curr_block_no); 

            MUT_ASSERT_TRUE(&utest_sg_pool[0][element_no-1] == sg_ptr);                              
            MUT_ASSERT_INT_EQUAL(block_offset, curr_block_no-start_block_no);
        } /* end for(blocks = nth_element - delta; blocks <= nth_element; blocks++) */
    } /* end for(element_no = 1; element_no < BM_MAX_SG_ELELEMENTS; element++) */

    return;
}
/******************************************
 * end fbe_raid_group_test_get_sg_ptr_offset()
 ******************************************/



/*********************************************************************
 * fbe_raid_group_test_clip_sg_list_empty_source()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_sg_clip_sg_list().
 *   it tests for the case when we each the end of the source
 *   SG list because of the offset and hence we have nothing 
 *   to copy into destination SG list.
 *  

 *   None.
 *  
 * @return
 *   None.
 *  
 * NOTE:
 *   None.
 *
 * @author
 *  7/11/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_clip_sg_list_empty_source(void)
{    
    fbe_u16_t sg_count;
    fbe_sg_element_with_offset_t src_sg_desc;
    fbe_sg_element_t dest_sg_list[1], src_sg_list[1];
    fbe_status_t status = FBE_STATUS_OK;
    src_sg_list[0].address = dest_sg_list[0].address = NULL;
    src_sg_list[0].count = dest_sg_list[0].count = 0;
    
    fbe_sg_element_with_offset_init(&src_sg_desc, 0, &src_sg_list[0], NULL);

    status = fbe_raid_sg_clip_sg_list(&src_sg_desc,
                               dest_sg_list,
                               0,
                               &sg_count);         
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Bad status from fbe_raid_sg_clip_sg_list");
    MUT_ASSERT_TRUE(sg_count == 0);
    MUT_ASSERT_TRUE(dest_sg_list[0].address == NULL);
    MUT_ASSERT_TRUE(dest_sg_list[0].count == 0);
    
    return;
}
/*********************************************
 * end fbe_raid_group_test_clip_sg_list_empty_source()
 ********************************************/



/*********************************************************************
 * fbe_raid_group_test_clip_sg_list_src_and_des_same()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_sg_clip_sg_list().
 *   it tests for the case when we have same source and 
 *   destination SG list.
 *  

 *   None.
 *  
 * @return
 *   None.
 *  
 * NOTE:
 *   None.
 *
 * @author
 *  7/8/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_clip_sg_list_src_and_des_same(void)
{   
    #define MAX_NUM_SG_ELEMENTS 5
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t element_no;
    fbe_u16_t sg_count;
    fbe_u32_t start, delta, total_blocks;
    fbe_sg_element_with_offset_t src_sg_desc;

    /* initialize the source SG list with successive
     * blocks following arithmetic progression.
     */
   
    start = 3, delta = 2;    
    total_blocks = ((FBE_RAID_MAX_SG_ELEMENTS - 1) * (2 * start + (FBE_RAID_MAX_SG_ELEMENTS - 2) * delta)) / 2;

    for(element_no = 1; element_no < FBE_RAID_MAX_SG_ELEMENTS-1; element_no++)
    {        
        fbe_u32_t nth_element, sum_of_elements;
        fbe_u32_t curr_block_no, start_block_no, end_block_no;
        
        nth_element = start + (element_no - 1) * delta;    
        sum_of_elements = ((element_no) * (start + nth_element)) / 2;

        start_block_no = sum_of_elements - nth_element;        
        end_block_no = sum_of_elements - 1;
        
        for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no += 8 )
        {
            fbe_u32_t left_count, max_left_blocks, dest_byte_count = 0;
            fbe_sg_element_t *dest_sg_ptr;
           
         
            max_left_blocks = total_blocks - (curr_block_no + 1);
            for(left_count = 1; left_count < max_left_blocks; left_count+= 8)
            {
                util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                                        FBE_RAID_MAX_SG_ELEMENTS, 
                                        start,
                                        delta, 
                                        &total_blocks); 
    
                fbe_sg_element_with_offset_init(&src_sg_desc, 
                                                (curr_block_no + 1) * FBE_BE_BYTES_PER_BLOCK, 
                                                &utest_sg_pool[0][0], NULL);
                dest_sg_ptr = &utest_sg_pool[0][element_no - 1];
                dest_byte_count = left_count * FBE_BE_BYTES_PER_BLOCK;
                status = fbe_raid_sg_clip_sg_list(&src_sg_desc,
                                           dest_sg_ptr,
                                           dest_byte_count,
                                           &sg_count);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Bad status from fbe_raid_sg_clip_sg_list");
                /* destination SG list must be terminated properly. 
                 */
                MUT_ASSERT_TRUE(dest_sg_ptr[sg_count].address == NULL);
                MUT_ASSERT_INT_EQUAL(dest_sg_ptr[sg_count].count, 0);
            } /* end for(left_count = 0; left_count <= max_left_blocks; left_count++) */
        } /* end for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no++) */
    } /* end for(element_no = 1; element_no < BM_MAX_SG_ELELEMENTS; element++) */

    return;
}
/****************************************************
 * end fbe_raid_group_test_clip_sg_list_src_and_des_same()
 ***************************************************/

          
 
/*********************************************************************
 * fbe_raid_group_test_clip_sg_list_src_and_des_different()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_sg_clip_sg_list().
 *   it tests for the case when we have different source and 
 *   destination SG list.
 *  

 *   None.
 *  
 * @return
 *   None.
 *  
 * NOTE:
 *   None.
 *
 * @author
 *  7/8/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_clip_sg_list_src_and_des_different(void)
{    
    fbe_u32_t element_no;
    fbe_u16_t sg_count;
    fbe_u32_t start, delta, total_blocks;
    fbe_sg_element_t dest_sg_list[FBE_RAID_MAX_SG_ELEMENTS];
    fbe_sg_element_with_offset_t src_sg_desc;
    fbe_status_t status = FBE_STATUS_OK;
    /* initialize the source SG list with successive
     * blocks following arithmetic progression.
     */
    total_blocks = 0;
    start = 3, delta = 2;    
    util_rg_init_sg_list_ap(&utest_sg_pool[0][0],
                            FBE_RAID_MAX_SG_ELEMENTS, 
                            start,
                            delta, 
                            &total_blocks); 

    for(element_no = 1; element_no < FBE_RAID_MAX_SG_ELEMENTS; element_no++)
    {        
        fbe_u32_t nth_element, sum_of_elements;
        fbe_u32_t curr_block_no, start_block_no, end_block_no;
        
        nth_element = start + (element_no - 1) * delta;    
        sum_of_elements = ((element_no) * (start + nth_element)) / 2;

        start_block_no = sum_of_elements - nth_element;        
        end_block_no = sum_of_elements - 1;
        
        for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no++)
        {
            fbe_u32_t sg_index;
            fbe_u32_t dest_byte_count = 0;
           
            fbe_sg_element_with_offset_init(&src_sg_desc, 0, &utest_sg_pool[0][0], NULL);
    
            dest_byte_count = (curr_block_no + 1) * FBE_BE_BYTES_PER_BLOCK;
            status = fbe_raid_sg_clip_sg_list(&src_sg_desc,
                                       dest_sg_list,
                                       dest_byte_count,
                                       &sg_count);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Bad status from fbe_raid_sg_clip_sg_list");
            MUT_ASSERT_INT_EQUAL(sg_count, element_no); 

            /* address and count of destination of SG elements of src_sg_list[]
             * list must be same as the corresponding address and count of 
             * elements of dest_sg_list[].
             */
            for(sg_index = 0; sg_index < sg_count; sg_index++)
            {
                MUT_ASSERT_TRUE(dest_sg_list[sg_index].address == utest_sg_pool[0][sg_index].address);
                if (sg_index != (sg_count - 1))
                {
                    MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_index].count, utest_sg_pool[0][sg_index].count);
                }
                else
                {
                    MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_index].count, (curr_block_no - start_block_no + 1) * FBE_BE_BYTES_PER_BLOCK);
                }
            }

            /* destination SG list must be terminated properly.
             */
            MUT_ASSERT_TRUE(dest_sg_list[sg_count].address == NULL);
            MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_count].count, 0);
        } /* end for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no++) */
    } /* end for(element_no = 1; element_no < BM_MAX_SG_ELELEMENTS; element++) */

    return;
}
/******************************************************
 * end fbe_raid_group_test_clip_sg_list_src_and_des_different()
 *****************************************************/

          

#if 0

/*********************************************************************
 * fbe_raid_group_test_clip_buffer_list()
 *********************************************************************
 * @brief
 *   executes unit test case against rg_clip_buffer_list().
 *  

 *   None.
 *  
 * @return
 *   None.
 *  
 * NOTE:
 *   None.
 *
 * @author
 *  7/8/2008 - Created. JR
 * 
 ********************************************************************/
static void fbe_raid_group_test_clip_buffer_list(void)
{    
    fbe_u32_t bm_index;
    fbe_u32_t sg_count, element_no;
    fbe_u32_t start, delta, total_blocks;
    BM_MEMORY_GROUP bm_group[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_sg_element_t dest_sg_list[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH+1], src_sg_list[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH+1];
    fbe_sg_element_with_offset_t src_sg_desc;

    
    /* Create a chain of BM_MEMORY_GROUP (repersented by bm_group[] in
     * our code to be used in the chain  of fbe_raid_memory_header_t 
     * reperesented by utest_mv_pool[].
    
     * Create a chain of fbe_raid_memory_header_t to test the functionality of  
     * rg_clip_sg_buffer_list(). Each fbe_raid_memory_header_t is initialized with 
     * such that its:
     *     1. memory_ptr points to SG elements indicated by fbe_sg_element_t
     *     2. next points to successive fbe_raid_memory_header_t on the stack.
     */    
    total_blocks = 0;
    start = 2, delta = 3;    
    util_rg_init_sg_list_ap(src_sg_list,
                            FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH + 1, 
                            start,
                            delta, 
                            &total_blocks); 
        
    for(bm_index = 0; bm_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; bm_index++)
    {           
        bm_group[bm_index].size = (src_sg_list[bm_index].count) / sizeof(fbe_u32_t);
        if (bm_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH - 1)
        {
            bm_group[bm_index].next_p = &bm_group[bm_index + 1];
            utest_mv_pool[bm_index].next = &utest_mv_pool[bm_index + 1];
        }
        else
        {
            bm_group[bm_index].next_p = NULL;
            utest_mv_pool[bm_index].next = NULL;
        }

        utest_mv_pool[bm_index].is_allocated = FBE_TRUE;
        utest_mv_pool[bm_index].group_ptr = &bm_group[bm_index];
        utest_mv_pool[bm_index].memory_ptr = (fbe_u32_t*)(src_sg_list[bm_index].address);
    } 


    /* Executes the test case where src_sgd_ptr is NULL. 
     */
   /* sg_count = rg_clip_buffer_list(NULL, dest_sg_list, 0);                    
    MUT_ASSERT_INT_EQUAL(sg_count, 0);     /* [TEST_CASE_ISSUE | BUG | NEW]
                                            * there is a bug in the code of 
                                            * rg_clip_buffer_list() as it tries
                                            * to access the list without 
                                            * ensuring that the argument 
                                            * passed to it is non-null or not.
                                            */  

    
    for(element_no = 1; element_no <= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; element_no++)
    {        
        fbe_u32_t nth_element, sum_of_elements;
        fbe_u32_t curr_block_no, start_block_no, end_block_no;
        
        nth_element = start + (element_no - 1) * delta;    
        sum_of_elements = ((element_no) * (start + nth_element)) / 2;

        start_block_no = sum_of_elements - nth_element;        
        end_block_no = sum_of_elements - 1;
        
        for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no++)
        {
            fbe_u32_t sg_index;
            fbe_u32_t dest_byte_count = 0;
           
            src_sg_desc.sg_element = (fbe_sg_element_t*) &utest_mv_pool[0];
            src_sg_desc.offset = 0;
    
            dest_byte_count = (curr_block_no + 1) * FBE_BE_BYTES_PER_BLOCK;
            sg_count = rg_clip_buffer_list(&src_sg_desc,
                                           dest_sg_list,
                                           dest_byte_count);

            MUT_ASSERT_INT_EQUAL(sg_count, element_no); 

            /* address and count of destination of SG elements of src_sg_list[]
             * list must be same as the corresponding address and count of 
             * elements of dest_sg_list[].
             */
            for(sg_index = 0; sg_index < sg_count; sg_index++)
            {
                MUT_ASSERT_TRUE(dest_sg_list[sg_index].address == src_sg_list[sg_index].address);
                if (sg_index != (sg_count - 1))
                {
                    MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_index].count, src_sg_list[sg_index].count);
                }
                else
                {
                    MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_index].count, (curr_block_no - start_block_no + 1) * FBE_BE_BYTES_PER_BLOCK);
                }
            }

            /* destination SG list must be terminated properly.
             */
            MUT_ASSERT_TRUE(dest_sg_list[sg_count].address == NULL);
            MUT_ASSERT_INT_EQUAL(dest_sg_list[sg_count].count, 0);
        } /* end for(curr_block_no = start_block_no; curr_block_no <= end_block_no; curr_block_no++) */
    } /* end for(element_no = 1; element_no < BM_MAX_SG_ELELEMENTS; element++) */


    return;
}
/******************************************
 * end fbe_raid_group_test_clip_buffer_list()
 ******************************************/

/*********************************************************************
 * utest_raid_fill_invalid_sectors()
 *********************************************************************
 * @brief
 *   executes unit test case against fbe_raid_fill_invalid_sectors().
 *  

 *   None.
 *  
 * @return
 *   None.
 *  
 * NOTE:
 *   None.
 *
 * @author
 *  7/8/2008 - Created. JR
 * 
 ********************************************************************/
static void utest_raid_fill_invalid_sectors(void)
{  
    #define MAX_FILL_SG_ELEMENTS 3

    fbe_u32_t sg_index, xor_index, sec_index;
    fbe_u32_t total_blocks, start, delta, blocks;
    fbe_sg_element_t sg_list[MAX_FILL_SG_ELEMENTS];
    
    /* Iniatilize the SG list with the following characteristics:
     *      1. number of blocks addressed by successive elements will follow
     *         arithmetic progression.
     *      2. the address field of SG element will be initialized to sector
     *         pool repersented by utest_sg_pool[].
     */
    total_blocks = 0;
    start = 2, delta = 3;    
    
    for(sg_index = 0; sg_index < (MAX_FILL_SG_ELEMENTS - 1); sg_index++)
    {
        sg_list[sg_index].count = (start + sg_index * delta)*FBE_BE_BYTES_PER_BLOCK;        
        sec_index = (sg_index == 0) ? 0 : total_blocks;
        sg_list[sg_index].address = (fbe_u8_t*) &utest_sector_pool[sec_index];      
        total_blocks += start + sg_index * delta;
    }
    
    sg_list[sg_index].count = 0; 
    sg_list[sg_index].address = NULL;

    for(blocks = 0; blocks <= total_blocks; blocks++)
    {
        fbe_u32_t temp_blocks, sg_block_count;
        fbe_u32_t index, inited_blocks, non_inited_blocks;
        fbe_u8_t *byte_ptr; 
        fbe_sg_element_t *cur_sg_ptr = sg_list;
        fbe_sector_invalidate_info_t *pInv;
        

        /* clean the XOR pool which we will be using 
         * in test cases.
         */
        for(xor_index=0; xor_index<total_blocks; xor_index++)
        {
           fbe_zero_memory(utest_sector_pool+xor_index, sizeof(SECTOR));
        }

        /* we have ensure that our sector pool does not contain
         * any pattern and all bytes of sector is set to ZERO.
         */
        fbe_raid_fill_invalid_sectors(sg_list,
                                  0, /* we will use 0 as seed value */
                                  blocks);
        
        /* validate that block is initialized as per the invalid 
         * pattern.
         */       
        inited_blocks = blocks;        
        while(inited_blocks > 0)
        {            
            sg_block_count = cur_sg_ptr->count / FBE_BE_BYTES_PER_BLOCK;
            temp_blocks = FBE_MIN(inited_blocks, sg_block_count);
            for (index = 0, byte_ptr = (fbe_u8_t *) cur_sg_ptr->address; 
                 index < temp_blocks; 
                 index++, byte_ptr += FBE_BE_BYTES_PER_BLOCK)
            {
                /* we will do basic sanity testing
                 */
                pInv = (fbe_sector_invalidate_info_t *) byte_ptr;
                MUT_ASSERT_INT_EQUAL(pInv->magic_number, FBE_INVALID_SECTOR_MAGIC_NUMBER);
                MUT_ASSERT_INT_EQUAL(pInv->who, FBE_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC);
                MUT_ASSERT_INT_EQUAL(pInv->why, 0);
                MUT_ASSERT_INT_EQUAL(pInv->odd_controller, AM_I_ODD_CONTROLLER);
            }            
         
            inited_blocks -= temp_blocks;
            cur_sg_ptr++;
        }

        /* validate that the other remaining blocks are not initialized with 
         * invalid pattern. We will just walkthrough the sector_pool and
         * will validate the sanity.
         */
        non_inited_blocks = total_blocks - blocks; 
        while(non_inited_blocks>0)
        {
            pInv = (fbe_sector_invalidate_info_t *) &utest_sector_pool[blocks];
            MUT_ASSERT_INT_NOT_EQUAL(pInv->magic_number, FBE_INVALID_SECTOR_MAGIC_NUMBER);
            MUT_ASSERT_INT_NOT_EQUAL(pInv->who, FBE_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC);
            
            non_inited_blocks -= 1;
        }
    } /* end for(blocks = 0; blocks <= total_blocks; blocks++) */
}
/******************************************
 * end utest_raid_fill_invalid_sectors()
 ******************************************/
#endif
/****************************************************************
 * fbe_raid_library_test_add_sg_util_tests()
 ****************************************************************
 * @brief
 *   This function acts as a test suite for unit test cases 
 *   used to validate the implementation of functions in 
 *   'rg_sg_util.c'.
 *
 *  argc [I] - number of user supplied arguments.
 *  argv [I] - user supplied arguments in the format of strings.
 *
 * @return
 *  None.
 *
 * @author
 *  6/10/2008 - Created. JR
 *
 ****************************************************************/
void fbe_raid_library_test_add_sg_util_tests(mut_testsuite_t * const suite_p)
{
    mut_testsuite_t *sg_suite_p;
    sg_suite_p = MUT_CREATE_TESTSUITE("fbe_raid_group_sg_tests");
//    MUT_ADD_TEST(suite_p, utest_raid_fill_invalid_sectors, NULL, NULL);
//    MUT_ADD_TEST(suite_p, fbe_raid_group_test_clip_buffer_list, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_clip_sg_list_empty_source, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_clip_sg_list_src_and_des_different, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_clip_sg_list_src_and_des_same, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_count_sg_blocks, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_sg_get_sg_total, NULL, NULL); 
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_determine_sg_uniform, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_sg_count_uniform_blocks, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_sg_count_nonuniform_blocks, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_get_sg_ptr_offset, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_copy_sg_list, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_adjust_sg_desc, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_scatter_cache_to_bed, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_scatter_fed_to_bed, NULL, NULL);
    MUT_ADD_TEST(suite_p, fbe_raid_group_test_scatter_sg_to_bed, NULL, NULL);

    MUT_RUN_TESTSUITE(sg_suite_p);
}
/******************************************
 * end fbe_raid_library_test_add_sg_util_tests()
 ******************************************/


/**********************************
 * end file fbe_raid_test_sg_util.c
 *********************************/
