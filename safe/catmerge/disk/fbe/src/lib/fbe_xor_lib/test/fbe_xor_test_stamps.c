/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_test_stamps.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions to evaluate stamps consistency for parity rg
 *
 * @version
 *  12/04/2013 - Created. Geng Han
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_xor_api.h"
#include "fbe_xor_private.h"
#include "mut.h"
#include "fbe/fbe_random.h"
#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"

/*************************
 *   TYPE DEFINITIONS
 *************************/
// for efficiency reason, test width 8 is sufficient
#define RG_START_WIDTH 8 
#define RG_END_WIDTH 8

#define FBE_ORIG_MR3_TSTAMP 0x5555u
#define FBE_NEW_MR3_TSTAMP  0x6666u

typedef enum step_ret_status_e
{
    TO_BREAK,
    TO_COMPLETED,
    TO_CONTINUE
} step_ret_status;

typedef enum orignal_stripe_states_e
{
    ORIG_INITIAL_STATE,
    ORIG_MR3_STATE,
    ORIG_468_STATE
} orignal_stripe_states;

typedef enum write_algorithm_e
{
    WRITE_MR3,
    WRITE_468
} write_algorithm;

// the expected ts_ws_mismatch_errs results
typedef enum expected_ts_ws_mismatch_errs_result_e
{
    NONZERO, // ts_ws_mismatch_errs is nonzero
    AS_SAME_AS_IW_POS, // as same as the single incomplete write pos or the pos which was the single complete write
    MORE_THAN_ONE, // more than one pos masked with error
    ZERO // there should be no stamps mismatch
} expected_ts_ws_mismatch_errs_result;

typedef enum step_states_e
{
    STEP_STATE_INVALID,
    STEP_STATE_COMPLETED,
    STEP_STATE_INITIALIZE,
    STEP_STATE_RUNNING
} step_states;

typedef step_ret_status (*step_function_ptr) (void *scratch_p, fbe_u32_t step_index);
typedef struct iterate_step_s
{
    step_function_ptr step_func;
    step_states       step_state;
} iterate_step;

typedef struct iterate_scratch_s
{
    iterate_step steps[20];
    fbe_u16_t parity_bitmap;
    fbe_u16_t iw_bitmap; // incomplete write bitmap
    write_algorithm write_algorithm;
    fbe_u32_t max_468_data_drive_cnt;
    fbe_u32_t phy_468_drive_bitmap; 
    expected_ts_ws_mismatch_errs_result expected_result;
    fbe_bool_t expected_needs_inv;
    fbe_bool_t needs_inv;
    fbe_u16_t width;
    fbe_u16_t fatal_key;
    fbe_u16_t fatal_cnt;
    fbe_xor_scratch_r6_state_t scratch_state;
    fbe_sector_t orig_sectors[16];
    fbe_sector_t new_sectors[16];
}
iterate_scratch_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
//--------------------------steps--------------------------------
void fbe_xor_test_r6_iw_stamps_verify(void);
// select one meaningful RG width for the test, 4-16
step_ret_status set_width(void *scratch_handle, fbe_u32_t step_index);
// set one or two meaningful fatal disk, there must be at least one fatal on data disks
step_ret_status set_fatal_disks(void *scratch_handle, fbe_u32_t step_index);
// set the initial stripe state, which might be left by MR3, 468 write or the initial state
step_ret_status set_initial_stripe_state(void *scratch_handle, fbe_u32_t step_index);
// set the incomplete write on the stripe, which might be left by MR3, 468 write.
step_ret_status set_incomplete_write(void *scratch_handle, fbe_u32_t step_index);
// set the expected output of fbe_xor_rbld_data_stamps_r6
step_ret_status set_expected_result(void *scratch_handle, fbe_u32_t step_index);
// call fbe_xor_rbld_data_stamps_r6 to get stamp error bitmap ts_ws_mismatch_errs and the conclusion of whether invalidate the sector 
void get_stamp_errors(iterate_scratch_t *scratch_p);
// check the output from fbe_xor_rbld_data_stamps_r6 against the expected result set by set_expected_result
void evaluate_stamp_errors(iterate_scratch_t *scratch_p);

//--------------------------internal--------------------------------
fbe_bool_t data_disk_orig_468_interation_finished(iterate_scratch_t *scratch_p, fbe_u32_t data_disk_num);
void data_disk_orig_init_468_ts_ws(iterate_scratch_t *scratch_p, fbe_u32_t data_disk_num);
void data_disk_orig_next_468_ts_ws(iterate_scratch_t *scratch_p, fbe_u32_t disk_index);
void generate_468_phy_iw_bitmap(iterate_scratch_t *scratch_p, fbe_u16_t log_iw_bitmap);
void generate_new_stamps(iterate_scratch_t *scratch_p);
void generate_468_phy_drive_bitmap(iterate_scratch_t *scratch_p, fbe_u16_t data_drive_bitmap);
fbe_u16_t next_468_data_drive_bitmap(fbe_u16_t width, fbe_u16_t current_data_drives_bitmap, fbe_u16_t expected_data_drives_cnt);
fbe_u32_t ones_count(fbe_u16_t bitmap);
fbe_bool_t get_next_2_fatal_key(iterate_scratch_t *scratch_p);


/*************************
 *   GLOBAL VARIABLE
 *************************/
iterate_scratch_t iterate_scratch = 
{
    // steps
    { 
        {set_width, STEP_STATE_INITIALIZE},
        {set_initial_stripe_state, STEP_STATE_INITIALIZE},
        {set_incomplete_write, STEP_STATE_INITIALIZE},
        {set_fatal_disks, STEP_STATE_INITIALIZE},
        {set_expected_result, STEP_STATE_INITIALIZE},
        {NULL, STEP_STATE_INVALID},
    }
};


/*!**************************************************************
 * set_expected_result()
 ****************************************************************
 * @brief
 *  set the expected results of fbe_xor_rbld_data_stamps_r6
 *
 * @param scratch_handle -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
step_ret_status set_expected_result(void *scratch_handle, fbe_u32_t step_index)
{
    iterate_scratch_t *scratch_p = (iterate_scratch_t *)scratch_handle;
    fbe_u16_t iw_bitmap; // incomplete write
    fbe_u16_t cw_bitmap; // completed write
    fbe_u16_t iw_bits_except_fatal_bits;
    fbe_u16_t cw_bits_except_fatal_bits;
    fbe_u16_t iw_bits_cnt_except_fatal_bits;
    fbe_u16_t cw_bits_cnt_except_fatal_bits; 
    fbe_u16_t data_disk_bitmask;
    fbe_u16_t touched_data_drive_bitmap;

    iw_bitmap = scratch_p->iw_bitmap;
    if (scratch_p->write_algorithm == WRITE_MR3)
    {
        cw_bitmap = ((1 << scratch_p->width) - 1) ^ iw_bitmap;
    }
    else
    {
        cw_bitmap = scratch_p->phy_468_drive_bitmap ^ iw_bitmap;
    }
    
    if (scratch_p->fatal_cnt == 2)
    {
        // if except the fatal_key, there are still incomplete write
        // ts_ws_mismatch_errs is nonzero is sufficient in that we could not recover any data
        if ((cw_bitmap & (~scratch_p->fatal_key)) == 0 || (iw_bitmap & (~scratch_p->fatal_key)) == 0)
        {
            scratch_p->expected_result = ZERO;
            scratch_p->expected_needs_inv = FBE_FALSE;
        }
        else 
        {
            scratch_p->expected_result = NONZERO;
            scratch_p->expected_needs_inv = FBE_TRUE;
        }
    }
    else if (scratch_p->fatal_cnt == 1)
    {
        data_disk_bitmask = ~(0x3 << (scratch_p->width - 2));
        if (scratch_p->write_algorithm == WRITE_MR3)
        {
            touched_data_drive_bitmap = data_disk_bitmask;
        }
        else
        {
            touched_data_drive_bitmap = (scratch_p->phy_468_drive_bitmap & data_disk_bitmask);
        }

        // if except the fatal_key, there are still incomplete write
        // we should take into account 3 kinds of cases:
        // 0_iw_pos, 1_iw_pos, more_than_2_iw_pos
        iw_bits_except_fatal_bits = (iw_bitmap & (~scratch_p->fatal_key));
        cw_bits_except_fatal_bits = (cw_bitmap & (~scratch_p->fatal_key));
        iw_bits_cnt_except_fatal_bits = ones_count(iw_bits_except_fatal_bits);
        cw_bits_cnt_except_fatal_bits = ones_count(cw_bits_except_fatal_bits);
        if ((iw_bits_cnt_except_fatal_bits == 0) || (cw_bits_cnt_except_fatal_bits == 0))
        {
            scratch_p->expected_result = ZERO;
            scratch_p->expected_needs_inv = FBE_FALSE;
        }
        else if ((((iw_bitmap | scratch_p->fatal_key) & data_disk_bitmask) == touched_data_drive_bitmap)
                 && (((cw_bitmap | scratch_p->fatal_key) & data_disk_bitmask) == touched_data_drive_bitmap))
        {
            // this case both row parity or diagnal parity could not find out any stamp errors,
            // except the mismatch against each other. row_parity and diag_parity would recover two internal consistent copys for the fatal blk.
            // But the data and CRC part of each copy could not match each other, and the sector would be invalidated eventually.
            // It should be the following case in fbe_xor_eval_reconstruct_1
            // Result        | Row Dead | Diag Dead | Data | RCsum | DCsum | Csums
            // -------------------------------------------------------------------
            // coh, inv      |    0     |     0     |   0  |   1   |   1   |   0
            // -----------------------------------------------------------

            /*
             width = 6,fatal_key = 0000000000001000,fatal_cnt = 1
             write alg: 468, phy_468_drive_bitmap = 0000000000111000, iw_bitmap = 0000000000101000
             expected result: MORE_THAN_ONE 
             ts_ws_mismatch_errs: 110000
             ------------orignal-------------
             row :time_stamp: 0x5555, write_stamp = 0x000f
             diag:time_stamp: 0x5555, write_stamp = 0x000f
             data:time_stamp: 0x7fff, write_stamp = 0x0008
             data:time_stamp: 0x7fff, write_stamp = 0x0004
             data:time_stamp: 0x7fff, write_stamp = 0x0002
             data:time_stamp: 0x7fff, write_stamp = 0x0001
             ------------new-------------
             row :time_stamp: 0x5555, write_stamp = 0x000f  
             diag:time_stamp: 0x5555, write_stamp = 0x0007  
             data:time_stamp: 0x7fff, write_stamp = 0x0008  
             data:time_stamp: 0x7fff, write_stamp = 0x0004
             data:time_stamp: 0x7fff, write_stamp = 0x0002
             data:time_stamp: 0x7fff, write_stamp = 0x0001
             */
            
            scratch_p->expected_result = MORE_THAN_ONE;
            scratch_p->expected_needs_inv = FBE_TRUE;
        }
        else if (iw_bits_cnt_except_fatal_bits == 1 || cw_bits_cnt_except_fatal_bits == 1)
        {
            // we should exactly know the the single incomplte write pos or single complete write pos
            scratch_p->expected_result = AS_SAME_AS_IW_POS;
            scratch_p->expected_needs_inv = FBE_FALSE;
        }
        else 
        {
            scratch_p->expected_result = MORE_THAN_ONE;
            scratch_p->expected_needs_inv = FBE_TRUE;
        }
    }

    scratch_p->steps[step_index].step_state = STEP_STATE_COMPLETED;
    return TO_CONTINUE;
}

/*!**************************************************************
 * get_next_2_fatal_key()
 ****************************************************************
 * @brief
 *  get next meaningful fatal_key with two disks dead
 *
 * @param scratch_p -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
fbe_bool_t get_next_2_fatal_key(iterate_scratch_t *scratch_p)
{
    // end with 2 parity disks dead, since both reconstruct_1 and reconstruct_2 asks at least one data drive dead
    while (++scratch_p->fatal_key < (0x3 << (scratch_p->width - 2)))
    {
        if (ones_count(scratch_p->fatal_key) == 2)
        {
            return 1;
        }
    }

    return 0;
}


/*!**************************************************************
 * set_fatal_disks()
 ****************************************************************
 * @brief
 * set one or two meaningful fatal disk, there must be at least one fatal on data disks
 *--------------------------------------
 * |row  | diag | Dn | Dn-1 | ... | D0 |
 *--------------------------------------
 *
 * @param scratch_handle -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
step_ret_status set_fatal_disks(void *scratch_handle, fbe_u32_t step_index)
{
    iterate_scratch_t *scratch_p = (iterate_scratch_t *)scratch_handle;
    fbe_u16_t width = scratch_p->width;
    fbe_u32_t next_step_index = step_index + 1;
    if (scratch_p->steps[step_index].step_state == STEP_STATE_INITIALIZE)
    {
        // start with fatal_cnt == 1 and fatal_key == 0x1
        scratch_p->fatal_key = 1;
        scratch_p->fatal_cnt = 1;
        scratch_p->steps[step_index].step_state = STEP_STATE_RUNNING;
        // tell next step to initialize
        scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
        return TO_CONTINUE;
    }
    else if (scratch_p->steps[step_index].step_state == STEP_STATE_RUNNING)
    {
        // check the next steps state
        if (scratch_p->steps[next_step_index].step_state == STEP_STATE_COMPLETED)
        {
            // change the fatal_key
            if (scratch_p->fatal_cnt == 1)
            {
                // single fatal disk must be on data disks, since fbe_xor_rbld_data_stamps_r6 would only be called by
                // reconstruct_1 and reconstruct_2, which commands that there must be one fatal data disk
                if (scratch_p->fatal_key & (1 << (width - 3)))
                {
                    // have already finished fatal_1 iteration
                    // start with fatal_2, D0 & D1
                    scratch_p->fatal_key = 0x3;
                    scratch_p->fatal_cnt = 2;
                }
                else
                {
                    scratch_p->fatal_key = scratch_p->fatal_key << 1;
                }
            }
            else // fatal_cnt == 2
            {
                if (!get_next_2_fatal_key(scratch_p))
                {
                    // have finished iterating all the fatal 2 keys
                    scratch_p->steps[step_index].step_state = STEP_STATE_COMPLETED;
                    return TO_COMPLETED;
                }
            }

            // initialize b_reconst_1, it depends on the fatal disk num on data disks
            if (ones_count(scratch_p->fatal_key & ((1 << (width - 2)) - 1)) == 1)
            {
                scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1;
            }
            else
            {
                scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2;
            }


            // tell next step to initialize
            scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
            return TO_CONTINUE;
        }
		else
		{
			// just continue to let the following steps to finish
			return TO_CONTINUE;
		}
    }
    else // STEP_STATE_COMPLETED
    {
        // should never come to here
        MUT_FAIL_MSG("should never come to here");
        return TO_BREAK;
    }
}

/*!**************************************************************
 * set_width()
 ****************************************************************
 * @brief
 *  select one meaningful RG width for the coming test, 4-16
 *
 * @param scratch_handle -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
step_ret_status set_width(void *scratch_handle, fbe_u32_t step_index)
{
    iterate_scratch_t *scratch_p = (iterate_scratch_t *)scratch_handle;
    fbe_u32_t next_step_index = step_index + 1;
    if (scratch_p->steps[step_index].step_state == STEP_STATE_INITIALIZE)
    {
        scratch_p->width = RG_START_WIDTH;
        scratch_p->parity_bitmap = ((1 << (scratch_p->width -1)) | (1 << (scratch_p->width -2)));
        scratch_p->steps[step_index].step_state = STEP_STATE_RUNNING;
        // tell next step to initialize
        scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
        return TO_CONTINUE;
    }
    else if (scratch_p->steps[step_index].step_state == STEP_STATE_RUNNING)
    {
        // check the next steps state
        if (scratch_p->steps[next_step_index].step_state == STEP_STATE_COMPLETED)
        {
            if (scratch_p->width >= RG_END_WIDTH)
            {
                scratch_p->steps[step_index].step_state = STEP_STATE_COMPLETED;
                return TO_BREAK;
            }
            else
            {
                // test next width
                scratch_p->width++;
                scratch_p->parity_bitmap = (1 << (scratch_p->width -1)) | (1 << (scratch_p->width -2));
                // tell next step to initialize
                scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
                return TO_CONTINUE;
            }
        }
		else
		{
			// just continue to let the following steps to finish
			return TO_CONTINUE;
		}
    }
    else // STEP_STATE_COMPLETED
    {
        // should never come to here
        MUT_FAIL_MSG("should never come to here");
        return TO_BREAK;
    }
}


/*!**************************************************************
 * show_combination()
 ****************************************************************
 * @brief
 *  show the result information of one combination
 *
 * @param scratch_p -- the iterate scratch mng structure               
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void show_combination(iterate_scratch_t *scratch_p)
{
    static fbe_u32_t index = 0;
	char bin_string[100];
	char bin_string_2[100];
    fbe_u32_t width = scratch_p->width;
    int i;
    mut_printf(MUT_LOG_TEST_STATUS, "---------%d-------------", index++);
	itoa(scratch_p->fatal_key, bin_string, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "width = %d, fatal_key = %s, fatal_cnt = %d", scratch_p->width, bin_string, scratch_p->fatal_cnt);
    if (scratch_p->write_algorithm == WRITE_MR3)
    {
        itoa(scratch_p->iw_bitmap, bin_string, 2);
        mut_printf(MUT_LOG_TEST_STATUS, "write alg: MR3, iw_bitmap = %s", bin_string);
    }
    else
    {
		itoa(scratch_p->iw_bitmap, bin_string, 2);
        itoa(scratch_p->phy_468_drive_bitmap, bin_string_2, 2);
        mut_printf(MUT_LOG_TEST_STATUS, "write alg: 468, phy_468_drive_bitmap = %s, iw_bitmap = %s", bin_string_2, bin_string);
    }
    if (scratch_p->expected_result == NONZERO)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "expected result: NONZERO");
    }
    else if (scratch_p->expected_result == ZERO)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "expected result: ZERO");
    }
    else if (scratch_p->expected_result == MORE_THAN_ONE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "expected result: MORE_THAN_ONE");
    }

    mut_printf(MUT_LOG_TEST_STATUS, "expected_needs_inv = %d, needs_inv = %d", scratch_p->expected_needs_inv, scratch_p->needs_inv);

    mut_printf(MUT_LOG_TEST_STATUS, "------------orignal-------------");
    mut_printf(MUT_LOG_TEST_STATUS, "row :time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->orig_sectors[width-1].time_stamp, scratch_p->orig_sectors[width-1].write_stamp);
    mut_printf(MUT_LOG_TEST_STATUS, "diag:time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->orig_sectors[width-2].time_stamp, scratch_p->orig_sectors[width-2].write_stamp);
    for (i = width - 3; i >= 0; i--)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "data:time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->orig_sectors[i].time_stamp, scratch_p->orig_sectors[i].write_stamp);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "------------new-------------");
    mut_printf(MUT_LOG_TEST_STATUS, "row :time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->new_sectors[width-1].time_stamp, scratch_p->new_sectors[width-1].write_stamp);
    mut_printf(MUT_LOG_TEST_STATUS, "diag:time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->new_sectors[width-2].time_stamp, scratch_p->new_sectors[width-2].write_stamp);
    for (i = width - 3; i >= 0; i--)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "data:time_stamp: 0x%04x, write_stamp = 0x%04x", scratch_p->new_sectors[i].time_stamp, scratch_p->new_sectors[i].write_stamp);
    }

}

/*!**************************************************************
 * data_disk_orig_468_interation_finished()
 ****************************************************************
 * @brief
 *  check whether all these data_disks has finished the orignal 468 ts & ws iteration
 *
 * @param scratch_p -- the iterate scratch mng structure               
 * @param data_disk_num -- the number of data disks
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
fbe_bool_t data_disk_orig_468_interation_finished(iterate_scratch_t *scratch_p, fbe_u32_t data_disk_num)
{
    fbe_u32_t i;
    for (i = 0; i < data_disk_num; i++)
    {
        if (scratch_p->orig_sectors[i].write_stamp == 0)
        {
            return 0;
        }
    }

    return 1;
}

/*!**************************************************************
 * data_disk_orig_init_468_ts_ws()
 ****************************************************************
 * @brief
 *  initialize the ts and ws on these data disks for orig 468, start from ws_0_ts_invalid
 *
 * @param scratch_p -- the iterate scratch mng structure               
 * @param data_disk_num -- the number of data disks
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void data_disk_orig_init_468_ts_ws(iterate_scratch_t *scratch_p, fbe_u32_t data_disk_num)
{
    fbe_u32_t i;

    for (i = 0; i < data_disk_num; i++)
    {
        scratch_p->orig_sectors[i].time_stamp = FBE_SECTOR_INVALID_TSTAMP; // start from ws_0_ts_invalid
        scratch_p->orig_sectors[i].write_stamp = 0;
    }
}

/*!**************************************************************
 * data_disk_orig_next_468_ts_ws()
 ****************************************************************
 * @brief
 *  select the next ts_ws combination on data disk for orig 468
 *  in the order: ws_0_ts_invalid->[ws_0_ts_valid]->ws_1_ts_invalid
 * @param scratch_p -- the iterate scratch mng structure               
 * @param disk_index -- the index of the index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void data_disk_orig_next_468_ts_ws(iterate_scratch_t *scratch_p, fbe_u32_t disk_index)
{
    fbe_u32_t width = scratch_p->width;
    if (scratch_p->orig_sectors[disk_index].time_stamp == FBE_SECTOR_INVALID_TSTAMP && scratch_p->orig_sectors[disk_index].write_stamp == 0)
    {
        // the next meaningful combination depends on the parity stamp
        if (scratch_p->orig_sectors[width - 1].time_stamp == FBE_ORIG_MR3_TSTAMP)
        {
            scratch_p->orig_sectors[disk_index].time_stamp = FBE_ORIG_MR3_TSTAMP;
        }
        else
        {
            scratch_p->orig_sectors[disk_index].write_stamp = 1 << disk_index;
        }
    }
    else if (scratch_p->orig_sectors[disk_index].time_stamp == FBE_ORIG_MR3_TSTAMP && scratch_p->orig_sectors[disk_index].write_stamp == 0) // ws_0_ts_valid
    {
        scratch_p->orig_sectors[disk_index].time_stamp = FBE_SECTOR_INVALID_TSTAMP;
        scratch_p->orig_sectors[disk_index].write_stamp = 1 << disk_index;
    }
    else
    {
        // should never come to here
        MUT_FAIL_MSG("should never come to here");
    }
}

/*!**************************************************************
 * generate_468_phy_iw_bitmap()
 ****************************************************************
 * @brief
 *  generate the 468 phy iw_bitmap
 *  
 * @param scratch_p -- the iterate scratch mng structure               
 * @param log_iw_bitmap -- the logical incomplete write bitmap
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void generate_468_phy_iw_bitmap(iterate_scratch_t *scratch_p, fbe_u16_t log_iw_bitmap)
{
    int i;
    fbe_u16_t phy_iw_bitmap = 0;
    for (i = 0; i < scratch_p->width; i++)
    {
        if (scratch_p->phy_468_drive_bitmap & (1 << i))
        {
            if (log_iw_bitmap & 0x1)
            {
                phy_iw_bitmap |= (1 << i); 
            }

            log_iw_bitmap >>= 1;
        }
    }

    scratch_p->iw_bitmap = phy_iw_bitmap;
}

/*!**************************************************************
 * generate_new_stamps()
 ****************************************************************
 * @brief
 *  generate the timestamps and writestamps for the new write
 *  
 * @param scratch_p -- the iterate scratch mng structure               
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void generate_new_stamps(iterate_scratch_t *scratch_p)
{
    int i;

    if (scratch_p->write_algorithm == WRITE_MR3)
    {
        for (i = 0; i < scratch_p->width; i++)
        {
            if ((scratch_p->iw_bitmap & (1 << i)) == 0)
            {
                // no incomplete write on this disk
                if (scratch_p->parity_bitmap & (1 << i))
                {
                    // this is a parity disk
                    scratch_p->new_sectors[i].time_stamp = (FBE_SECTOR_ALL_TSTAMPS | FBE_NEW_MR3_TSTAMP);
                }
                else
                {
                    scratch_p->new_sectors[i].time_stamp = FBE_NEW_MR3_TSTAMP;
                }
                scratch_p->new_sectors[i].write_stamp = 0;
            }
            else
            {
                // remains as the orig_sectors
                scratch_p->new_sectors[i].time_stamp = scratch_p->orig_sectors[i].time_stamp;
                scratch_p->new_sectors[i].write_stamp = scratch_p->orig_sectors[i].write_stamp;
            }
        }
    }
    else // 468 write
    {
        fbe_u16_t parity_ws;
        fbe_u16_t new_data_drive_ws;
        fbe_u16_t new_data_drive_ts;
        parity_ws = 0;
        // set ws_ts on data drives
        for (i = 0; i < (scratch_p->width - 2); i++)
        {
            scratch_p->new_sectors[i].time_stamp = scratch_p->orig_sectors[i].time_stamp;
            scratch_p->new_sectors[i].write_stamp = scratch_p->orig_sectors[i].write_stamp;

            if ((scratch_p->phy_468_drive_bitmap & (1 << i)) != 0)
            {
                // rotate the ws
                new_data_drive_ws = scratch_p->orig_sectors[i].write_stamp ^ (1 << i);
                new_data_drive_ts = FBE_SECTOR_INVALID_TSTAMP;
                if ((scratch_p->iw_bitmap & (1 << i)) == 0)
                {
                    scratch_p->new_sectors[i].time_stamp = new_data_drive_ts;
                    scratch_p->new_sectors[i].write_stamp = new_data_drive_ws;
                }
                parity_ws |= new_data_drive_ws;
            }
            else
            {
                parity_ws |= scratch_p->new_sectors[i].write_stamp;
            }
        }        

        // set ws_ts on parity drives
        for (i = (scratch_p->width - 1); i >= (scratch_p->width - 2); i--)
        {
            scratch_p->new_sectors[i].time_stamp = scratch_p->orig_sectors[i].time_stamp;
            scratch_p->new_sectors[i].write_stamp = scratch_p->orig_sectors[i].write_stamp;
            
            if ((scratch_p->iw_bitmap & (1 << i)) == 0)
            {
                scratch_p->new_sectors[i].time_stamp = (scratch_p->orig_sectors[i].time_stamp & (~FBE_SECTOR_ALL_TSTAMPS));
                scratch_p->new_sectors[i].write_stamp = parity_ws;
            }
        }
    }
}

/*!**************************************************************
 * generate_468_phy_drive_bitmap()
 ****************************************************************
 * @brief
 *  generate 468 physical drive bitmaps
 *  
 * @param scratch_p -- the iterate scratch mng structure               
 * @param data_drive_bitmap -- bitmap of data drives touched by 468 
 * 
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void generate_468_phy_drive_bitmap(iterate_scratch_t *scratch_p, fbe_u16_t data_drive_bitmap)
{
    fbe_u16_t width;
    width  = scratch_p->width;
    scratch_p->phy_468_drive_bitmap = ((0x3 << (width - 2)) | data_drive_bitmap);
}

/*!**************************************************************
 * next_468_data_drive_bitmap()
 ****************************************************************
 * @brief
 *  get the next meaningful 468 data drive bitmap
 *  
 * @param width -- the rg width
 * @param current_data_drives_bitmap -- current bitmap of data drives
 * @param expected_data_drives_cnt -- the expected data drive number
 * 
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
fbe_u16_t next_468_data_drive_bitmap(fbe_u16_t width, fbe_u16_t current_data_drives_bitmap, fbe_u16_t expected_data_drives_cnt)
{
    fbe_u32_t data_drives_cnt = width - 2;
 	fbe_u32_t i = 0;
    while (++current_data_drives_bitmap < (1 << data_drives_cnt))
    {
        if (ones_count(current_data_drives_bitmap) == expected_data_drives_cnt)
        {
            // imagine this bitmap as a ring, there should be no "0" between "1"
            // since all the data sectors should be continous
			fbe_u32_t current_data_drives_bitmap_tmp = current_data_drives_bitmap;
            for (i = 0; i < data_drives_cnt; i++)
            {
				if (current_data_drives_bitmap_tmp == ((1 << expected_data_drives_cnt) - 1))
                {
                    return current_data_drives_bitmap;
                }
				current_data_drives_bitmap_tmp = (((current_data_drives_bitmap_tmp & 1) << (data_drives_cnt - 1)) | (current_data_drives_bitmap_tmp >> 1));
			}
        }
    }

    return 0;
}

/*!**************************************************************
 * ones_count()
 ****************************************************************
 * @brief
 *  get ones count
 *  
 * @param bitmap -- the number to be evaluated
 * 
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
fbe_u32_t ones_count(fbe_u16_t bitmap)
{
    fbe_u32_t cnt = 0;
    while (bitmap)
    {
        if (bitmap & 0x1)
        {
            cnt++;
        }

        bitmap >>= 1;
    }

    return cnt;
}

/*!**************************************************************
 * set_initial_stripe_state()
 ****************************************************************
 * @brief
 *  set the initial stripe state, which might be left by MR3, 468 write or the initial state
 *  in the order ORIG_INITIAL_STATE->ORIG_MR3_STATE->ORIG_468_STATE
 *
 * @param scratch_handle -- the iterate scratch mng structure               
 * @param step_index -- the step index
 * 
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
step_ret_status set_initial_stripe_state(void *scratch_handle, fbe_u32_t step_index)
{    
    iterate_scratch_t *scratch_p = (iterate_scratch_t *)scratch_handle;
    fbe_u16_t width = scratch_p->width;
    fbe_u32_t next_step_index = step_index + 1;
    int i;
    static orignal_stripe_states orig_stripe_state;

    if (scratch_p->steps[step_index].step_state == STEP_STATE_INITIALIZE)
    {
        // start with ORIG_INITIAL_STATE 
        orig_stripe_state = ORIG_INITIAL_STATE;

        // set the the whole stripe to initial state
        for (i = width - 1; i >= 0; i--)
        {
            if ((i != width -1) && (i != width - 2))
            {
                scratch_p->orig_sectors[i].time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            }
            else
            {
                scratch_p->orig_sectors[i].time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
            }
            scratch_p->orig_sectors[i].write_stamp = 0;
        }
        scratch_p->steps[step_index].step_state = STEP_STATE_RUNNING;
        // tell next step to initialize
        scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
        return TO_CONTINUE;
    }
    else if (scratch_p->steps[step_index].step_state == STEP_STATE_RUNNING)
    {
        // check the state of the next state
        if (scratch_p->steps[next_step_index].step_state == STEP_STATE_COMPLETED)
        {
            if (orig_stripe_state == ORIG_INITIAL_STATE)
            {
                orig_stripe_state = ORIG_MR3_STATE;
                // set the the whole stripe to meaningful MR3 state
                for (i = width - 1; i >= 0; i--)
                {
                    if (scratch_p->parity_bitmap & (1 << i))
                    {
                        scratch_p->orig_sectors[i].time_stamp = (FBE_SECTOR_ALL_TSTAMPS | FBE_ORIG_MR3_TSTAMP);
                    }
                    else
                    {
                        scratch_p->orig_sectors[i].time_stamp = FBE_ORIG_MR3_TSTAMP;
                    }
                    scratch_p->orig_sectors[i].write_stamp = 0;
                }
            }
            else if (orig_stripe_state == ORIG_MR3_STATE)
            {
                orig_stripe_state = ORIG_468_STATE;
                data_disk_orig_init_468_ts_ws(scratch_p, width);
            }
            else // ORIG_468_STATE
            {
                // for the parity time_stamp, loop over FBE_SECTOR_INVALID_TSTAMP->FBE_ORIG_MR3_TSTAMP
                // check whether has completed the parity time_stamp iteration
                if (data_disk_orig_468_interation_finished(scratch_p, width - 2))
                {
                    // has finished this time_stamp, select next
                    if (scratch_p->orig_sectors[width-1].time_stamp == FBE_SECTOR_INVALID_TSTAMP)
                    {
                        scratch_p->orig_sectors[width-1].time_stamp = FBE_ORIG_MR3_TSTAMP;
                        scratch_p->orig_sectors[width-2].time_stamp = FBE_ORIG_MR3_TSTAMP;
                        // initialize the ts & ws on data disks
                        data_disk_orig_init_468_ts_ws(scratch_p, width - 2);
                    }
                    else
                    {
                        // we have finished the iteration
                        scratch_p->steps[step_index].step_state = STEP_STATE_COMPLETED;
                        return TO_COMPLETED;
                    }
                }
                else
                {
                    // check whether there is a data disk has been completed iteration
                    // for data disks ws_0_ts_invalid->[ws_0_ts_valid]->ws_1_ts_invalid
                    for (i = width - 3; i >= 1; i--)
                    {
                        if (data_disk_orig_468_interation_finished(scratch_p, i))
                        {
                            // select next ts_ws combination with disk index "i"
                            data_disk_orig_next_468_ts_ws(scratch_p, i);
                            // initialize the ts & ws on data disks
                            data_disk_orig_init_468_ts_ws(scratch_p, i);
                            break;
                        }
                    }

                    if (i == 0)
                    {
                        // select next next
                        data_disk_orig_next_468_ts_ws(scratch_p, 0);
                    }
                }
            }

            // generate the correct parity write_stamp
            scratch_p->orig_sectors[width-1].write_stamp = 0;
            for (i = width - 3; i >= 0; i--)
            {
                scratch_p->orig_sectors[width-1].write_stamp |= scratch_p->orig_sectors[i].write_stamp;
            }
            scratch_p->orig_sectors[width-2].write_stamp = scratch_p->orig_sectors[width-1].write_stamp;
            // tell next step to initialize
            scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
            return TO_CONTINUE;
        }
		else
		{
			// just continue to let the following steps to finish
			return TO_CONTINUE;
		}
    }
    else // STEP_STATE_COMPLETED
    {
        // should never come to here
        MUT_FAIL_MSG("should never come to here");
        return TO_BREAK;
    }
}

/*!**************************************************************
 * set_incomplete_write()
 ****************************************************************
 * @brief
 * set the incomplete write on the stripe, which might be left by MR3, 468 write.
 * for MR3 write, all the alive drives would be touched. so possible incomplete drive would be <1 - (2^width - 2)>
 * for 468 write, we should consider both how many drives would be touched by the write and how many drives would be written incompletely
 * order: MR3->468
 *
 * @param scratch_handle -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
step_ret_status set_incomplete_write(void *scratch_handle, fbe_u32_t step_index)
{
    iterate_scratch_t *scratch_p = (iterate_scratch_t *)scratch_handle;
    fbe_u16_t width = scratch_p->width;
    fbe_u32_t next_step_index = step_index + 1;
    static fbe_u32_t current_468_data_drive_cnt;
    static fbe_u32_t current_468_data_drive_bitmap;
    static fbe_u32_t current_468_log_iw_bitmap;
    static fbe_u32_t current_468_touched_drive_cnt;

    if (scratch_p->steps[step_index].step_state == STEP_STATE_INITIALIZE)
    {
        // start with MR3 write 
        scratch_p->write_algorithm = WRITE_MR3;

        // start from 1 to (1 << width - 2)
        scratch_p->iw_bitmap = 1;

        // generate the new ts_ws
        generate_new_stamps(scratch_p);

        // set proper step state
        scratch_p->steps[step_index].step_state = STEP_STATE_RUNNING;
        // tell next step to initialize
        scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
        return TO_CONTINUE;
    }
    else if (scratch_p->steps[step_index].step_state == STEP_STATE_RUNNING)
    {
        // check the state of the next state
        if (scratch_p->steps[next_step_index].step_state == STEP_STATE_COMPLETED)
        {
            if (scratch_p->write_algorithm == WRITE_MR3)
            {
                // whether we have completed all the MR3 incompleted iteration
                if (scratch_p->iw_bitmap != ((1 << width) - 2)) // sth like 11110, all 1...1 stands for no write on the stripe
                {
                    // has NOT
                    scratch_p->iw_bitmap++;
                    // generate the new ts_ws
                    generate_new_stamps(scratch_p);

                    // tell next step to initialize
                    scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
                    return TO_CONTINUE;
                }
                else
                {
                    // has finished, change to WRITE_468
                    scratch_p->write_algorithm = WRITE_468;

                    // how many drives touched would apply 468 write
                    scratch_p->max_468_data_drive_cnt = 2 * (width - 3) / 3;

                    // start with 1 data drive
                    current_468_data_drive_cnt = 1;

                    // get the intial data drive bitmap, start with 0000001
                    current_468_data_drive_bitmap = 1;

                    // get the 468 touched physical drives bitmap
                    generate_468_phy_drive_bitmap(scratch_p, current_468_data_drive_bitmap);

                    current_468_touched_drive_cnt = current_468_data_drive_cnt + 2/* parity disks */;

                    // start with logical incomplete write bitmap with 1
                    current_468_log_iw_bitmap = 1;

                    // generate new stamps
                    goto GEN_NEW_STAMPS;
                }
            }
            // for 468 write, select next one
            // in the order:
            // loop1: current_468_data_drive_cnt -- [1, max_468_data_drive_cnt]
            //    loop2: current_468_data_drive_bitmap  -- in the collection [1, 2^(width - 2) - 1] all the bitmaps whose ones_count is current_468_data_drive_cnt
            //       loop3: current_468_log_iw_bitmap  --  [1, 2^current_468_touched_drive_cnt - 2]
            // whether we have finished all the iteration for current_468_data_drive_cnt 
            if (current_468_log_iw_bitmap == ((1 << current_468_touched_drive_cnt) - 2))
            {
                // finished loop3.
                // start next iteration of loop2
                current_468_data_drive_bitmap = next_468_data_drive_bitmap(width, current_468_data_drive_bitmap, current_468_data_drive_cnt);
                if (current_468_data_drive_bitmap == 0)
                {
                    // finished loop 2
                    if (current_468_data_drive_cnt == scratch_p->max_468_data_drive_cnt)
                    {
                        // finished loop 1
                        scratch_p->steps[step_index].step_state = STEP_STATE_COMPLETED;
                        return TO_COMPLETED;
                    }
                    else
                    {
                        // start next iteration of loop1
                        current_468_data_drive_cnt++;
                        // start from the first iteration of loop2
                        current_468_data_drive_bitmap = next_468_data_drive_bitmap(width, 1, current_468_data_drive_cnt);
                        // start from the first iteration of loop3
                        current_468_log_iw_bitmap = 1;
                    }
                }
                else
                {
                    // start from the first iteration of loop3
                    current_468_log_iw_bitmap = 1;
                }
            }
            else
            {
                // start the next iteration of loop3
                current_468_log_iw_bitmap++;
            }

            // get the 468 touched logical and physical drives bitmap
            generate_468_phy_drive_bitmap(scratch_p, current_468_data_drive_bitmap);

            current_468_touched_drive_cnt = current_468_data_drive_cnt + 2/* parity disks */;

GEN_NEW_STAMPS:
            {
                if (scratch_p->expected_result == ZERO
                    && scratch_p->fatal_key == 0x11
                    && scratch_p->write_algorithm == WRITE_468
                    && scratch_p->phy_468_drive_bitmap == 0x31
                    && scratch_p->iw_bitmap == 0x10
                    && scratch_p->orig_sectors[5].time_stamp == 0xd555
                    && scratch_p->orig_sectors[4].time_stamp == 0xd555)
                {
                    int j;
                    j = 0;
                }

                // generate phy 468 iw_bitmap
                generate_468_phy_iw_bitmap(scratch_p, current_468_log_iw_bitmap);

                // generate the new ts_ws
                generate_new_stamps(scratch_p);
            }

            // tell next step to initialize
            scratch_p->steps[next_step_index].step_state = STEP_STATE_INITIALIZE;
            return TO_CONTINUE;
        }
		else
		{
			// just continue to let the following steps to finish
			return TO_CONTINUE;
		}
    }
    else // STEP_STATE_COMPLETED
    {
        // should never come to here
        MUT_FAIL_MSG("should never come to here");
        return TO_BREAK;
    }
}

/*!**************************************************************
 * get_stamp_errors()
 ****************************************************************
 * @brief
 *  call fbe_xor_rbld_data_stamps_r6 to find out the stamp errors ts_ws_mismatch_errs
 *
 * @param scratch_p -- the iterate scratch mng structure               
 * @param step_index -- the step index
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void get_stamp_errors(iterate_scratch_t *scratch_p)
{
    int i;
    fbe_xor_error_t eboard;
    fbe_u16_t bitkey[16];
    fbe_u16_t parity_bitmap;
    fbe_u16_t rebuild_pos[2];
    fbe_u16_t parity_pos[2];
    fbe_bool_t error_result;
    fbe_sector_t *sector_p[16];
    fbe_xor_scratch_r6_t r6_scratch;
    fbe_sector_t fatal_blk[2];



    for (i = 0; i < 16; i++)
    {
        bitkey[i] = (1 << i);
        sector_p[i] = &scratch_p->new_sectors[i];
    }
    parity_bitmap = (0x3 << (scratch_p->width - 2));

    rebuild_pos[0] = 0;
    rebuild_pos[1] = 1;

    parity_pos[0] = scratch_p->width - 1; // row
    parity_pos[1] = scratch_p->width - 2; // diag

    r6_scratch.fatal_blk[0] = &fatal_blk[0];
    r6_scratch.fatal_blk[1] = &fatal_blk[1];
    r6_scratch.fatal_key = scratch_p->fatal_key;
    r6_scratch.fatal_cnt = scratch_p->fatal_cnt;
    r6_scratch.scratch_state = scratch_p->scratch_state;
    
    fbe_xor_rbld_data_stamps_r6(&r6_scratch, 
                                &eboard,
                                sector_p,
                                bitkey,
                                parity_bitmap,
                                scratch_p->width,
                                rebuild_pos,
                                parity_pos,
                                (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1),
                                &error_result,
                                &scratch_p->needs_inv);

    // regenerate the new_sectors, since fbe_xor_rbld_data_stamps_r6 change some data that would have impact on following combinations
    generate_new_stamps(scratch_p);
}

/*!**************************************************************
 * evaluate_stamp_errors()
 ****************************************************************
 * @brief
 *  check the output from fbe_xor_rbld_data_stamps_r6 against the expected result set by set_expected_result
 *
 * @param scratch_p -- the iterate scratch mng structure               
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void evaluate_stamp_errors(iterate_scratch_t *scratch_p)
{

    // first check the flag returned by fbe_xor_rbld_data_stamps_r6
    if (scratch_p->expected_needs_inv != scratch_p->needs_inv)
    {
        show_combination(scratch_p);
        MUT_FAIL_MSG("unexpected output of fbe_xor_rbld_data_stamps_r6");
    }
    else
    {
        // accord with the expectation
        // show_combination(scratch_p);
    }
}

//////////////////////////////////////
/*!**************************************************************
 * fbe_xor_test_r6_iw_stamps_verify()
 ****************************************************************
 * @brief
 *  Loop over all meaningful r6 iw test cases
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void fbe_xor_test_r6_iw_stamps_verify(void)
{
    step_ret_status step_ret;
    fbe_u32_t step_index = 0;

    iterate_scratch.fatal_key = 0;
    iterate_scratch.fatal_cnt = 0;
    
    // select one meaningful combination
    while (1)
    {
        if (iterate_scratch.steps[step_index].step_func)
        {
            step_ret = iterate_scratch.steps[step_index].step_func(&iterate_scratch, step_index);
            switch (step_ret)
            {
            case TO_BREAK:
                return;
            case TO_COMPLETED:
				step_index = 0;
                continue;
            default:
                step_index++;
            }
        }
        else
        {
            // has succesfully selected one combination
            // call fbe_xor_rbld_data_stamps_r6 to find out the stamp errors ts_ws_mismatch_errs
            get_stamp_errors(&iterate_scratch);

            // evaluate ts_ws_mismatch_errs 
            evaluate_stamp_errors(&iterate_scratch);

            // select next
            step_index = 0;
        }
    }
}
/* end fbe_xor_test_r6_iw_stamps_verify() */


/*!***************************************************************
 * fbe_xor_test_stamps_add_tests()
 ****************************************************************
 * @brief
 *  This function adds the xor stamps inconsistent verify unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  12/04/2013 - Created. Geng Han
 *
 ****************************************************************/
void fbe_xor_test_stamps_add_tests(mut_testsuite_t * const suite_p)
{
    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_r6_iw_stamps_verify,
                 NULL,
                 NULL);

    return;
}
/* end fbe_xor_test_stamps_add_tests() */
/*************************
 * end file fbe_xor_test_stamps.c
 *************************/

