/****************************************************************
 *  Copyright (C)  EMC Corporation 2006-2007
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_reconstruct_1.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the reconstruct 1 state of the
 *  EVENODD algorithm which is used for RAID 6 functionality.  The reconstruct
 *  one state is used to rebuild one data disk or one data disk and one parity
 *  disk.  If only a data drive is dead, the data will be reconstructed 
 *  independently from each parity drive.  Both sets of reconstructed data will
 *  be compared and considered before deciding on which one, if any, to use to
 *  complete the reconstruction.  If one parity drive is dead then the 
 *  remaining disk will be used to reconstruct the data drive and the scratch
 *  state will be set to reconstruct the parity.
 *
 * @notes
 *
 * @author
 *
 * 04/06/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_epu_tracing.h"

/* Bitmask for data comparison.
 */
#define FBE_XOR_RAID6_COMPARE_RPARITY_DEAD 0x20 /* row parity disk is invalid. */
#define FBE_XOR_RAID6_COMPARE_DPARITY_DEAD 0x10 /* diagonal parity disk is invalid. */
#define FBE_XOR_RAID6_COMPARE_DATA_MATCHES 0x8  /* reconstructed datas match. */
#define FBE_XOR_RAID6_COMPARE_RCSUM_MATCHES 0x4 /* reconstructed row checksum matches 
                                         * reconstructed data checksum.
                                         */
#define FBE_XOR_RAID6_COMPARE_DCSUM_MATCHES 0x2 /* reconstructed diagonal checksum 
                                         * matches reconstructed data checksum.
                                         */
#define FBE_XOR_RAID6_COMPARE_CSUMS_MATCHES 0x1 /* reconstructed row checksum matches
                                         * reconstructed diagonal checksum.
                                         */

/***************************************************************************
 *  fbe_xor_cleanup_successful_rebuild_1()
 ***************************************************************************
 * @brief
 *  This macro is used to clean up the tracking structures on a successful
 *  rebuild.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 *
 *  @return
 *    None.
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_cleanup_successful_rebuild_1(fbe_xor_scratch_r6_t * m_scratch_p,
                                                 fbe_xor_error_t * m_eboard_p,
                                                 fbe_u16_t m_bitkey,
                                                 fbe_bool_t * m_return_value)
{
    fbe_status_t status;
    fbe_xor_correct_all_one_pos(m_eboard_p, m_bitkey);
    status = fbe_xor_scratch_r6_remove_error(m_scratch_p, m_bitkey);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    } 
    m_eboard_p->m_bitmap |= m_bitkey;
    *m_return_value = FBE_TRUE;
    return FBE_STATUS_OK;
} /* fbe_xor_cleanup_successful_rebuild_1() */

/***************************************************************************
 *  fbe_xor_handle_case_15()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Everything matches.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 *
 * @return fbe_status_t
 *
 * @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_15(fbe_xor_scratch_r6_t * m_scratch_p,
                                   fbe_xor_error_t * m_eboard_p,
                                   fbe_u16_t m_bitkey,
                                   fbe_bool_t * m_return_value,
                                   fbe_u32_t m_checksum)
{
    fbe_status_t status;

    /* Copy reconstructed row checksum to sector->crc.
     */
    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word),
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word),
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Return successful rebuild and tell eval_parity_unit_r6 that this
     * rebuild is done.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p, m_eboard_p, 
                                                  m_bitkey, m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;       
    return FBE_STATUS_OK;
} /* fbe_xor_handle_case_15() */

/***************************************************************************
 *  fbe_xor_handle_case_12()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors match.
 *  Reconstructed row data matches reconstructed row checksum.
 *  Reconstructed diagonal data does not match reconstructed diagonal checksum.
 *  Reconstructed checksums do not match. 
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 * @return fbe_status_t
 *
 * @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_12(fbe_xor_scratch_r6_t * m_scratch_p,
                                   fbe_xor_error_t * m_eboard_p,
                                   fbe_u16_t m_bitkey,
                                   fbe_bool_t * m_return_value,
                                   fbe_u32_t m_checksum,
                                   fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed row checksum to sector->crc.
    */

    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word),
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word),
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild diagonal parity.
    */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* There must be a problem with the parity of checksums on the diagonal
     * parity disk.  Set parity of checksum coherency error bit in eboard.
     */
    m_eboard_p->u_poc_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_poc_coh_bitmap, 
                                       FBE_XOR_ERR_TYPE_POC_COH, 0, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_12() */

/***************************************************************************
 *  fbe_xor_handle_case_10()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors match.
 *  Reconstructed row data does not match reconstructed row checksum.
 *  Reconstructed diagonal data matches reconstructed diagonal checksum.
 *  Reconstructed checksums do not match.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_10(fbe_xor_scratch_r6_t * m_scratch_p,
                                   fbe_xor_error_t * m_eboard_p,
                                   fbe_u16_t m_bitkey,
                                   fbe_bool_t * m_return_value,
                                   fbe_u32_t m_checksum,
                                   fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed diagonal data to sector because the row
     * parity disk is bad.
     */
    memcpy(m_scratch_p->fatal_blk[0], 
           m_scratch_p->diag_syndrome, 
           (FBE_XOR_R6_SYMBOLS_PER_SECTOR * sizeof(fbe_xor_r6_symbol_size_t)));

    /* Copy reconstructed diagonal checksum to sector->crc.
     */
    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed diagnoal checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild row parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* There must be a problem with the parity of checksums on the row
     * parity disk.  Set parity of checksum coherency error bit in eboard.
     */
    m_eboard_p->u_poc_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_poc_coh_bitmap, 
                                       FBE_XOR_ERR_TYPE_POC_COH, 1, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_10() */

/***************************************************************************
 *  fbe_xor_handle_case_05()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors do not match.
 *  Reconstructed row data matches reconstructed row checksum.
 *  Reconstructed diagonal data does not match reconstructed diagonal checksum.
 *  Reconstructed checksums match.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *   
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_05(fbe_xor_scratch_r6_t * m_scratch_p,
                                   fbe_xor_error_t * m_eboard_p,
                                   fbe_u16_t m_bitkey,
                                   fbe_bool_t * m_return_value,
                                   fbe_u32_t m_checksum,
                                   fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed row checksum to sector->crc.
     */
    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed row checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild diagonal parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* Set coherency error in eboard for diagonal parity.
     */
    m_eboard_p->u_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_coh_bitmap, 
                                       FBE_XOR_ERR_TYPE_COH, 11, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_05() */

/***************************************************************************
 *  fbe_xor_handle_case_04()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors do not match.
 *  Reconstructed row data matches reconstructed row checksum.
 *  Reconstructed diagonal data does not match reconstructed diagonal checksum.
 *  Reconstructed checksums do not match.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 * 
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_04(fbe_xor_scratch_r6_t * m_scratch_p,
                                           fbe_xor_error_t * m_eboard_p,
                                           fbe_u16_t m_bitkey,
                                           fbe_bool_t * m_return_value,
                                           fbe_u32_t m_checksum,
                                           fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed row checksum to sector->crc.
     */
    if (XOR_COND(m_checksum !=xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed row checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                              (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild diagonal parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* Set multiple parity of checksum coherency error in eboard because
     * there is an error in the parity of checksums but we are not sure
     * which one.
     */
    m_eboard_p->u_n_poc_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_n_poc_coh_bitmap, 
                                     FBE_XOR_ERR_TYPE_N_POC_COH, 0, FBE_TRUE);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    } 
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_n_poc_coh_bitmap, 
                                       FBE_XOR_ERR_TYPE_POC_COH, 2, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_04() */

/***************************************************************************
 *  fbe_xor_handle_case_03()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors do not match.
 *  Reconstructed row data does not match reconstructed row checksum.
 *  Reconstructed diagonal data matches reconstructed diagonal checksum.
 *  Reconstructed checksums match.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *    None.
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_03(fbe_xor_scratch_r6_t * m_scratch_p,
                                   fbe_xor_error_t * m_eboard_p,
                                   fbe_u16_t m_bitkey,
                                   fbe_bool_t * m_return_value,
                                   fbe_u32_t m_checksum,
                                   fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed diagonal data to sector because the row
     * parity disk is bad.
     */
    memcpy(m_scratch_p->fatal_blk[0], 
           m_scratch_p->diag_syndrome, 
           (FBE_XOR_R6_SYMBOLS_PER_SECTOR * sizeof(fbe_xor_r6_symbol_size_t)));

    /* Copy reconstructed diagonal checksum to sector->crc.
     */
    if (XOR_COND(m_checksum != xorlib_cook_csum(
                                     xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                     (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed diagonal checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild row parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* Set coherency error in eboard for row parity.
     */
    m_eboard_p->u_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_coh_bitmap, 
                          FBE_XOR_ERR_TYPE_COH, 1, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_03() */

/***************************************************************************
 *  fbe_xor_handle_case_02()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Reconstructed data sectors do not match.
 *  Reconstructed row data does not match reconstructed row checksum.
 *  Reconstructed diagonal data matches reconstructed diagonal checksum.
 *  Reconstructed checksums do not match.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_02(fbe_xor_scratch_r6_t * m_scratch_p,
                                  fbe_xor_error_t * m_eboard_p,
                                  fbe_u16_t m_bitkey,
                                  fbe_bool_t * m_return_value,
                                  fbe_u32_t m_checksum,
                                  fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed diagonal data to sector because the row
     * parity disk is bad.
     */
    memcpy(m_scratch_p->fatal_blk[0], 
           m_scratch_p->diag_syndrome, 
           (FBE_XOR_R6_SYMBOLS_PER_SECTOR * sizeof(fbe_xor_r6_symbol_size_t)));

    /* Copy reconstructed diagonal checksum to sector->crc.
     */

    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                     (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed diagonal checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                                (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild row parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    fbe_xor_scratch_r6_add_error(m_scratch_p, m_parity_pos_bitkey);

    /* Set multiple parity of checksum coherency error in eboard because
     * there is an error in the parity of checksums but we are not sure
     * which one.
     */
    m_eboard_p->u_n_poc_coh_bitmap |= m_parity_pos_bitkey;
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_n_poc_coh_bitmap, 
                          FBE_XOR_ERR_TYPE_N_POC_COH, 1, FBE_TRUE);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_n_poc_coh_bitmap, 
                          FBE_XOR_ERR_TYPE_POC_COH, 3, FBE_TRUE);
    return status;
} /* fbe_xor_handle_case_02() */

/***************************************************************************
 *  fbe_xor_handle_case_row_bad_data_good()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Row parity disk is invalid.
 *  Reconstructed diagonal data matches reconstructed diagonal checksum.
 *  Reconstrcuted diagonal data is good, use it and rebuild row parity.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *  
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_row_bad_data_good(fbe_xor_scratch_r6_t * m_scratch_p,
                                                  fbe_xor_error_t * m_eboard_p,
                                                  fbe_u16_t m_bitkey,
                                                  fbe_bool_t * m_return_value,
                                                  fbe_u32_t m_checksum,
                                                  fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed diagonal data to sector because the row
     * parity disk is bad.
     */
    memcpy(m_scratch_p->fatal_blk[0], 
           m_scratch_p->diag_syndrome, 
           (FBE_XOR_R6_SYMBOLS_PER_SECTOR * sizeof(fbe_xor_r6_symbol_size_t)));

    /* Copy reconstructed diagonal checksum to sector->crc.
     */
    ;  

    if (XOR_COND(m_checksum !=  xorlib_cook_csum(
                            xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                            (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "reconstructed diagonal checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
                              m_checksum,
                              xorlib_cook_csum(
                                 xorlib_calc_csum(m_scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol), 
                                          (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild row parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;

    /* Leave the errors in eboard for row parity because it is still not fixed.
     */
    return FBE_STATUS_OK;
} /* fbe_xor_handle_case_row_bad_data_good() */

/***************************************************************************
 *  fbe_xor_handle_case_row_bad_data_bad()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Row parity disk is invalid.
 *  Reconstructed diagonal data does not match reconstructed diagonal checksum.
 *  Reconstructed diagonal data is bad or an invalidated block.
 *  Either way we will go through invalidate.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_row_bad_data_bad(fbe_xor_scratch_r6_t * m_scratch_p,
                                                 fbe_xor_error_t * m_eboard_p,
                                                 fbe_u16_t m_bitkey,
                                                 fbe_bool_t * m_return_value)
{
    fbe_status_t status;

    /* Determine the types of checksum error.
     */
    status = fbe_xor_determine_csum_error(m_eboard_p, 
                                          (fbe_sector_t *)m_scratch_p->diag_syndrome, 
                                          m_bitkey, 
                                          m_scratch_p->seed,
                                          0,
                                          0,
                                          FBE_TRUE);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* Check if the sector has been invalidated on purpose.  If it is report
     * it as a crc error, if not then report it as a coherency error.
     */
    if (((m_eboard_p->crc_invalid_bitmap & m_bitkey) != 0) ||
        ((m_eboard_p->crc_raid_bitmap & m_bitkey) != 0)    ||
        ((m_eboard_p->corrupt_crc_bitmap & m_bitkey) != 0)    )
    {
        /* This error is due to an invalidated sector mark it as a crc error.
         * We pass 0 to the EPU tracing since we don't want to trace these errors.
         */
        fbe_xor_correct_all_non_crc_one_pos(m_eboard_p, m_bitkey);
        m_eboard_p->u_crc_bitmap |= m_bitkey;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, 0, 
                              FBE_XOR_ERR_TYPE_CRC, 10, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    else
    {
        /* Set uncorrectable coherency error in eboard.
         */
        m_eboard_p->u_coh_bitmap |= m_bitkey;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_coh_bitmap, 
                                           FBE_XOR_ERR_TYPE_COH, 2, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }

    /* Return false to invalidate the rebuild position.
     */
    *m_return_value = FBE_FALSE;
    return FBE_STATUS_OK;
} /* fbe_xor_handle_case_row_bad_data_bad() */

/***************************************************************************
 *  fbe_xor_handle_case_diag_bad_data_good()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Diagonal parity disk is invalid.
 *  Reconstructed row data matches reconstructed row checksum.
 *  Reconstructed row data is good, use it and rebuild diagonal parity.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *   
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_handle_case_diag_bad_data_good(fbe_xor_scratch_r6_t * m_scratch_p,
                                                   fbe_xor_error_t * m_eboard_p,
                                                   fbe_u16_t m_bitkey,
                                                   fbe_bool_t * m_return_value,
                                                   fbe_u32_t m_checksum,
                                                   fbe_u16_t m_parity_pos_bitkey)
{
    fbe_status_t status;

    /* Copy reconstructed row checksum to sector->crc.
     */
    if (XOR_COND(m_checksum != xorlib_cook_csum(xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                                 (fbe_u32_t)m_scratch_p->seed)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s reconstructed diagonal checksum 0x%x != cooked checksum 0x%x,"
                              "seed value = 0x%x\n",
			      __FUNCTION__,
                              m_checksum,
                              xorlib_cook_csum(
                                                   xorlib_calc_csum(m_scratch_p->fatal_blk[0]->data_word), 
                                                   (fbe_u32_t)m_scratch_p->seed),
                              (fbe_u32_t)m_scratch_p->seed);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    m_scratch_p->fatal_blk[0]->crc = m_checksum;

    /* Setup invalid bitmask and scratch state to rebuild row parity.
     */
    status = fbe_xor_cleanup_successful_rebuild_1(m_scratch_p,
                                                  m_eboard_p, 
                                                  m_bitkey,
                                                  m_return_value);
    if (status != FBE_STATUS_OK) 
    { 
        return status;
    }
    m_scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;

    /* Leave the errors in eboard for diagonal parity because it is still not 
     * fixed.
     */
    return FBE_STATUS_OK;
} /* fbe_xor_handle_case_diag_bad_data_good() */

/***************************************************************************
 *  fbe_xor_handle_case_diag_bad_data_bad()
 ***************************************************************************
 * @brief
 *  This macro is used to handle the reconstruct one case where:
 *  Diagonal parity disk is invalid.
 *  Reconstructed row data does not match reconstructed row checksum.
 *  Reconstructed row data is bad or an invalidated block.
 *  Either way we will go through invalidate.
 *
 * @param m_scratch_p - ptr to the scratch database area.
 * @param m_eboard_p - error board for marking different types of errors.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 *   m_return_value,[IO] - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions.
 *
 *  @return fbe_status_t
 *
 *  @author
 *    06/19/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t  fbe_xor_handle_case_diag_bad_data_bad(fbe_xor_scratch_r6_t * m_scratch_p,
                                                  fbe_xor_error_t * m_eboard_p,
                                                  fbe_u16_t m_bitkey,
                                                  fbe_bool_t * m_return_value)
{
    fbe_status_t status;

    /* Determine the types of checksum error.
     */
    status = fbe_xor_determine_csum_error(m_eboard_p, 
                                          m_scratch_p->fatal_blk[0], 
                                          m_bitkey, 
                                          m_scratch_p->seed,
                                          0,
                                          0,
                                          FBE_TRUE);
    if (status != FBE_STATUS_OK) { return status; }

    /* Check if the sector has been invalidated on purpose.  If it is report
     * it as a crc error, if not then report it as a coherency error.
     */
    if (((m_eboard_p->crc_invalid_bitmap & m_bitkey) != 0) ||
        ((m_eboard_p->crc_raid_bitmap & m_bitkey) != 0)    ||
        ((m_eboard_p->corrupt_crc_bitmap & m_bitkey) != 0)    )
    {
        /* This error is due to an invalidated sector mark it as a crc error.
         * We pass 0 to the EPU tracing since we don't want to trace these errors.
         */
        fbe_xor_correct_all_non_crc_one_pos(m_eboard_p, m_bitkey);
        m_eboard_p->u_crc_bitmap |= m_bitkey;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, 0, 
                                           FBE_XOR_ERR_TYPE_CRC, 11, FBE_TRUE);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else
    {
        /* Set uncorrectable coherency error in eboard.
         */
        m_eboard_p->u_coh_bitmap |= m_bitkey;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_coh_bitmap, 
                                           FBE_XOR_ERR_TYPE_COH, 3, FBE_TRUE);
        if (status != FBE_STATUS_OK) { return status; }
    }
 
    /* Return false to invalidate the rebuild position.
     */
    *m_return_value = FBE_FALSE;
    return FBE_STATUS_OK;
} /* fbe_xor_handle_case_diag_bad_data_bad() */

/***************************************************************************
 *  fbe_xor_check_for_invalidated_reconstruct()
 ***************************************************************************
 * @brief
 *  This macro is used to check if the reconstructed data was an invalidated
 *  block.  If it is then set a crc error, if not set a specified value.
 *
 * @param m_sector_p - ptr to the reconstructed sector.
 * @param m_eboard_p - error board for marking different types 
 *                               of errors.
 * @param m_eboard_error_p - error to be set.
 * @param m_bitkey - The bitmap of the disk being cleaned up.
 * @param m_return_value - The status of the function.
 * @param m_checksum - Reconstructed checksum.
 * @param m_parity_pos_bitkey - Bitmap of parity positions
 * @param m_reconstruct_p - reconstruction result which is TRUE or FALSE
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    09/18/2006 - Created. RCL
 ***************************************************************************/
static fbe_status_t fbe_xor_check_for_invalidated_reconstruct(fbe_sector_t * m_sector_p,
                                                              fbe_xor_error_t * m_eboard_p,
                                                              fbe_u16_t m_bitkey,
                                                              fbe_lba_t m_seed,
                                                              fbe_bool_t * m_reconstruct_p)
{
    fbe_status_t status;

    /* Determine the types of checksum error.       
     */
    status = fbe_xor_determine_csum_error(m_eboard_p, 
                                          m_sector_p, 
                                          m_bitkey, 
                                          m_seed,
                                          0,
                                          0,
                                          FBE_TRUE);
    if (status != FBE_STATUS_OK) { return status; }

    /* Check if the sector has been invalidated on purpose.  If it has been
     * invalidated on purpose then report it as a crc error.  If not then
     * there is something wrong with the data so report it as a coherency
     * error.
     */
    if (((m_eboard_p->crc_invalid_bitmap & m_bitkey) != 0) ||
        ((m_eboard_p->crc_raid_bitmap & m_bitkey) != 0)    ||
        ((m_eboard_p->corrupt_crc_bitmap & m_bitkey) != 0)    )
    {
        /* This error is due to an invalidated sector mark it as a crc error.
         */
        *m_reconstruct_p = FBE_TRUE;
         return FBE_STATUS_OK;
    }

    *m_reconstruct_p = FBE_FALSE;
    return FBE_STATUS_OK;
} /* fbe_xor_check_for_invalidated_reconstruct() */

/***************************************************************************
 *  fbe_xor_calc_csum_and_reconstruct_1()
 ***************************************************************************
 * @brief
 *   This routine calculates the partial rebuild of the missing disk.  If
 *   this is the first disk to be processed in the strip then some 
 *   initialization is required.  While the partial rebuild calculations are
 *   being performed the checksum will be calculated for the sector.  The
 *   partial rebuild is done two different ways (once from row parity and 
 *   once from diagonal parity) to two different scratch sectors.  This is 
 *   done so we can compare both attempts later to ensure we are reconstructing
 *   the correct data.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param data_blk_p - ptr to data sector.
 * @param column_index - the logical position of the drive within the strip.
 * @param rebuild_pos - the rebuild position.
 * @param uncooked_csum_p -  [O] uncooked checksum
 *
 *  ASSUMPTIONS:
 *    The scratch database area will be initialized with a call to 
 *    fbe_xor_init_scratch_r6.
 *
 *  @return
 *    Uncooked checksum.
 *
 *  @author
 *    04/04/2006 - Created. RCL
 ***************************************************************************/
 fbe_status_t fbe_xor_calc_csum_and_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                                              fbe_sector_t * const data_blk_p,
                                              const fbe_u32_t column_index,
                                              const fbe_u16_t rebuild_pos,
                                              fbe_u32_t *uncooked_csum_p)
{
    fbe_u32_t checksum = 0x0;
    fbe_u32_t * row_rebuild_scratch_ptr;
    fbe_u32_t * diag_rebuild_scratch_ptr;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * s_value_ptr;
    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;
    fbe_u16_t checksum_s_value = 0x0;    
    fbe_u32_t row_index;
    fbe_u32_t * src_ptr;
    fbe_u16_t times_dsymbol_skipped = 0;

    /* We should not get called to perform these calculations on the rebuild
     * disk.
     */

    if (XOR_COND(rebuild_pos == column_index))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s rebuild_pos 0x%x != column_index 0x%x",
                              __FUNCTION__,
                              rebuild_pos,
                              column_index);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Use the scratch sector to rebuild using the row parity.  We are using
     * the fatal block because it points to the acutal sector that will be
     * returned when reconstruction finished and if it contains the correct
     * data no data copies will be requred on completion.
     */
    row_rebuild_scratch_ptr = scratch_p->fatal_blk[0]->data_word;

    /* Get the top of the data sector.
     */
    src_ptr = data_blk_p->data_word;

    /* Holds the pointer to the top diagonal reconstruction block.
     */
    tgt_diagonal_ptr_holder = scratch_p->diag_syndrome->syndrome_symbol;

    /* Find the component of the S value in this particular column.  The S
     * value component is the (rebuild position - data position - 1) MOD 
     * FBE_XOR_EVENODD_M_VALUE.  This will never index the FBE_XOR_EVENODD_M_VALUE - 1
     * imaginary symbol because we will never get called for the rebuild 
     * position.
     */
    s_value = *(((fbe_xor_r6_symbol_size_t*)data_blk_p) + 
      ((rebuild_pos - column_index - 1 + FBE_XOR_EVENODD_M_VALUE) % FBE_XOR_EVENODD_M_VALUE));

    /* For each data symbol do three things.
     * 1) Update the checksum.
     * 2) Update the correct diagonal parity symbol.
     * 3) Update the correct row parity symbol.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /* This loop will process one symbol.  First find the correct position
         * in the diagonal parity block for this symbol and find the top of the
         * S value.  The correct position is
         * (row_index + column_index - rebuild position)
         * Use the diagonal syndrome scratch to rebuild using the diagonal 
         * parity.  We can use the syndrome because it will not be used for 
         * rebuilding one failed data disk.
         */
        diag_rebuild_scratch_ptr = (fbe_u32_t*) (tgt_diagonal_ptr_holder + 
                                        FBE_XOR_MOD_VALUE_M((row_index + 
                                                         column_index - 
                                                         rebuild_pos + 
                                                         FBE_XOR_EVENODD_M_VALUE), 
                                                       ((column_index - 
                                                         rebuild_pos - 
                                                         1 + 
                                                         FBE_XOR_EVENODD_M_VALUE) %
                                                         FBE_XOR_EVENODD_M_VALUE) ));

        s_value_ptr = (fbe_u32_t*) &s_value;

        /* If we are indexing the imaginary row of zeros we can't access that
         * memory location because it is not really there.  So check if we are
         * accessing that and instead of using imaginary zeros, just skip it.
         * Also, the S value is accumulated into each symbols so we don't have
         * to accumulate it in another buffer and add it to each symbol at the
         * end.
         */
        if(FBE_XOR_MOD_VALUE_M((row_index + 
                            column_index - 
                            rebuild_pos + 
                            FBE_XOR_EVENODD_M_VALUE), (FBE_XOR_EVENODD_M_VALUE + 1)) !=
                            (FBE_XOR_EVENODD_M_VALUE + 1))
        {
            /* If the scratch is not initialized then we need to set the calculations
             * instead of just an update.
             */
            if (scratch_p->initialized)
            {      
                XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *diag_rebuild_scratch_ptr++ ^= *src_ptr ^ *s_value_ptr++, 
                        *row_rebuild_scratch_ptr++ ^= *src_ptr++));
            }
            else
            {
                XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *diag_rebuild_scratch_ptr++ = *src_ptr ^ *s_value_ptr++, 
                        *row_rebuild_scratch_ptr++ = *src_ptr++));
            }
        }
        else
        {
            /* This diagonal symbol is not needed to rebuild the lost data.  
             * Each sector should satisfy this condition only once.  If the
             * scratch is not initialized then we need to set the calculations
             * instead of just an update.
             */
            if (scratch_p->initialized)
            {      
                XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *diag_rebuild_scratch_ptr++ ^= *s_value_ptr++, 
                        *row_rebuild_scratch_ptr++ ^= *src_ptr++));
            }
            else
            {
                XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *diag_rebuild_scratch_ptr++ = *s_value_ptr++, 
                        *row_rebuild_scratch_ptr++ = *src_ptr++));
            }

            if (XOR_COND(times_dsymbol_skipped != 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "times_dsymbol_skipped =x%x != 0\n",
                                      times_dsymbol_skipped);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            times_dsymbol_skipped++;
        }
    }  

    /* Next we need to partially calculate the checksum from the parity of
     * checksums.
     */

    /* If the S value is set in this column set the S value to all ones.
     * This will allow us to xor in one S value to the whole diagonal
     * parity column instead of 16 individual bits.  Since the bit positions
     * are labeled from MSB to LSB the calulation must be relative to 
     * (FBE_XOR_EVENODD_M_VALUE -1).
     */
    if (FBE_XOR_IS_BITSET(data_blk_p->crc,((FBE_XOR_EVENODD_M_VALUE - 2) - 
                                      ((rebuild_pos - 
                                      column_index - 
                                      1 + 
                                      FBE_XOR_EVENODD_M_VALUE) % 
                                      FBE_XOR_EVENODD_M_VALUE)) ))
    {
        checksum_s_value = 0xffff;
    }

    /* Update the row parity symbol with the data symbol.
     */
    if (scratch_p->initialized)
    {      
        scratch_p->checksum_row_syndrome ^= data_blk_p->crc;
    }
    else
    {
        scratch_p->checksum_row_syndrome = data_blk_p->crc;
    }

    /* Update the correct diagonal parity symbol with the current data symbol.
     */
    if (scratch_p->initialized)
    {      
        scratch_p->checksum_diag_syndrome 
            ^= (0xffff) &
               FBE_XOR_EVENODD_DIAGONAL_MANGLE_RECONSTRUCT(data_blk_p->crc,
                                                       column_index,
                                                       rebuild_pos) ^
            checksum_s_value;
    }
    else
    {
        scratch_p->checksum_diag_syndrome 
            = (0xffff) &
               FBE_XOR_EVENODD_DIAGONAL_MANGLE_RECONSTRUCT(data_blk_p->crc,
                                                       column_index,
                                                       rebuild_pos) ^
            checksum_s_value;
    }


    /* If the scratch was not initialized it is now.
     */
    if (!(scratch_p->initialized))
    {      
        scratch_p->initialized = FBE_TRUE;
    }

    *uncooked_csum_p = checksum;
    return FBE_STATUS_OK;

} /*fbe_xor_calc_csum_and_reconstruct_1()*/

/***************************************************************************
 *  fbe_xor_calc_csum_and_parity_reconstruct_1()
 ***************************************************************************
 * @brief
 *   This routine calculates the partial rebuild of the missing disk using
 *   the parity disk passed in.  While the partial rebuild calculations are
 *   being performed the checksum will be calculated for the sector.    The
 *   partial rebuild is done two different ways (once from row parity and 
 *   once from diagonal parity) to two different scratch sectors.  This is 
 *   done so we can compare both attempts later to ensure we are reconstructing
 *   the correct data.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param parity_blk_p - ptr to parity sector.
 * @param parity_index - the parity position passed in.
 * @param rebuild_pos - the rebuild position.
 *
 *  @return
 *    Uncooked checksum.
 *
 *  @author
 *    04/04/2006 - Created. RCL
 ***************************************************************************/
fbe_u32_t fbe_xor_calc_csum_and_parity_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                                                     fbe_sector_t * const parity_blk_p,
                                                     const fbe_xor_r6_parity_positions_t parity_index,
                                                     const fbe_u16_t rebuild_pos)
{
    fbe_u32_t checksum = 0x0;
    fbe_u32_t * rebuild_scratch_ptr;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * s_value_ptr;
    fbe_xor_r6_symbol_size_t * tgt_ptr_holder;
    fbe_u16_t checksum_s_value = 0x0;    
    fbe_u32_t row_index;
    fbe_u32_t * src_ptr;

    if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
    {
    /* If row parity disk is passed in use the scratch sector to rebuild using
     * the row parity.  We are using the fatal block because it points to the 
     * acutal sector that will be returned when reconstruction finished and if 
     * it contains the correctdata no data copies will be requred on completion.
     */
        rebuild_scratch_ptr = scratch_p->fatal_blk[0]->data_word;
    }
    else
    {
    /* If the diagonal parity disk is passed in use the diagonal syndrome 
     * scratch to rebuild using the diagonal parity.  We can use the syndrome
     * because it will not be used for rebuilding one failed data disk.
     */
        rebuild_scratch_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
        tgt_ptr_holder = (fbe_xor_r6_symbol_size_t*) rebuild_scratch_ptr;
    }

    /* Get the top of the parity sector.
     */
    src_ptr = parity_blk_p->data_word;

    /* Holds the pointer to the top diagonal reconstruction block.
     */
    s_value_ptr = (fbe_u32_t*) &s_value;

    /* Find the component of the S value that is in this disk
     */

    /* There is no component of the S value in the row parity disk.  There is 
     * no component of the S value in the diagonal parity disk when the dead
     * disk is in the first column.
     */
    if ((parity_index == FBE_XOR_R6_PARITY_POSITION_ROW) || (rebuild_pos == 0))
    {
        XORLIB_REPEAT8( (*s_value_ptr++ = 0x0) );        
    }
    /* Find the component of the S value in this particular column.  The S
     * value component from the diagonal parity disk is the 
     * (rebuild position - 1) MOD FBE_XOR_EVENODD_M_VALUE position.  This will 
     * never index the FBE_XOR_EVENODD_M_VALUE - 1 imaginary symbol because the
     * rebuild position = 0 case was covered in the if block.
     */
    else
    {
        s_value = *( ( (fbe_xor_r6_symbol_size_t*)parity_blk_p ) + 
            ( (rebuild_pos - 1) % FBE_XOR_EVENODD_M_VALUE) );
    }

    /* For each data symbol do two of three things.
     * 1) Update the checksum.
     * 2) Update the correct diagonal parity symbol.
     * or
     * 2) Update the correct row parity symbol.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /* This loop will process one symbol.  
         */
        /* First find the correct position
         * in the diagonal parity block for this symbol and find the top of the
         * S value if the parity disk passed in is the diagonal one.  The 
         * correct position in the diagonal parity block to use is
         * (row_index - rebuild position)
         */
        if (parity_index == FBE_XOR_R6_PARITY_POSITION_DIAG)
        {
            rebuild_scratch_ptr = (fbe_u32_t*) (tgt_ptr_holder + 
                                        FBE_XOR_MOD_VALUE_M((row_index -
                                                         rebuild_pos + 
                                                         FBE_XOR_EVENODD_M_VALUE), 
                                                         (FBE_XOR_EVENODD_M_VALUE - 
                                                          rebuild_pos - 1)));
        }
        /* else the row parity block is being processed so just keep using the
         * pointer as is.
         */

        s_value_ptr = (fbe_u32_t*) &s_value;

        if(( (FBE_XOR_MOD_VALUE_M( (row_index - rebuild_pos + FBE_XOR_EVENODD_M_VALUE),  
                               (FBE_XOR_EVENODD_M_VALUE + 1))) != (FBE_XOR_EVENODD_M_VALUE + 1)) ||
           ( parity_index == FBE_XOR_R6_PARITY_POSITION_ROW))
        {
            /* This is either the row parity or a diagonal parity symbol that
             * is needed.
             */
            XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *rebuild_scratch_ptr++ ^= *src_ptr++ ^ *s_value_ptr++));
        }
        else 
        {
            /* This is a diagonal parity symbol that is not used in
             * reconstructing the data just do the checksum.
             */
            XORLIB_REPEAT8((checksum ^= *src_ptr++, 
                        *rebuild_scratch_ptr++ ^= *s_value_ptr++));
        }
    }  

    /* Next we need to partially calculate the checksum from the parity of
     * checksums.
     */

    /* If the S value in the diagonal parity column is set then set the s value.     
     */
    if ((parity_index == FBE_XOR_R6_PARITY_POSITION_DIAG) && 
        (rebuild_pos != 0) &&
        (FBE_XOR_IS_BITSET(parity_blk_p->lba_stamp, ((FBE_XOR_EVENODD_M_VALUE - 2) - 
                                           (rebuild_pos - 1) % 
                                           FBE_XOR_EVENODD_M_VALUE) )))
    {
        checksum_s_value = 0xffff;
    }

    if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
    {
        /* Update the row parity symbol with the data symbol.
        */
        scratch_p->checksum_row_syndrome ^= parity_blk_p->lba_stamp;
    }
    else
    {
        /* Update the correct diagonal parity reconstructed symbol with the 
         * current data symbol.  We only want 16 bits.
         */
        scratch_p->checksum_diag_syndrome 
                        ^= (0xffff) &
                           (FBE_XOR_EVENODD_DIAGONAL_RECONSTRUCT_PARITY(parity_blk_p->lba_stamp, rebuild_pos) ^
                            checksum_s_value);
    }

    return (checksum);

} /* fbe_xor_calc_csum_and_parity_reconstruct_1() */

/***************************************************************************
 *  fbe_xor_eval_reconstruct_1()
 ***************************************************************************
 * @brief
 *  This function compares the sector located at scratch_p->fatal_blk[0] with
 *  the one at scratch_p->diag_syndrome for differences.  Both should contain
 *  an attempt to reconstruct the same data so they should be identical.  If
 *  a difference is found then the reconstructed checksums will be checked
 *  against the reconstructed data to determine which data is correct.  If 
 *  that does not identify the valid data then something is wrong and there
 *  isn't anything else to do.  If valid data is identified then the pointer
 *  scratch_p->fatal_blk[0] will point to it when the function returns.
 *

 * @param scratch_p - ptr to the scratch database area.
 *  ppos[],        [I]  - parity positions.
 * @param eboard_p - error board for marking different types of errors.
 * @param bitkey - bitkey for stamps/error boards.
 *  rebuild_pos[2],       [I]  - indexes to be rebuilt, first position should always 
 *                         be valid.
 *
 * @notes
 *  There are four comparisons used to determine which, if either, set of
 *  reconstructed data to use.  The comparisons are:
 *
 *                   +-----------+                              +-----------+
 *                   | Recovered |                              | Recovered |
 *                   |   Row     |  RAID6_COMPARE_DATA_MATCHES  | Diagonal  |
 *                   |   Data    | <--------------------------> |   Data    |
 *                   |           |                              |           |
 *                   |           |                              |           |
 *                   |           |                              |           |
 *                   |           |                              |           |
 *                   +-----------+                              +-----------+
 *                   |   Row     |                              | Diagonal  |
 *                   |   Csum    |                              |   Csum    |
 *                   +-----------+                              +-----------+
 * RAID6_COMPARE_RCSUM_MATCHES ^                                         ^
 *                             |                                         |
 *                             v             RAID6_COMPARE_DCSUM_MATCHES v
 *                   +-----------+                              +-----------+
 *                   |   POC     |                              |   POC     | 
 *                   |   Row     |  RAID6_COMPARE_CSUMS_MATCHES | Diagonal  |
 *                   |   Csum    | <--------------------------> |   Csum    |
 *                   +-----------+                              +-----------+
 *
 *  The bits for RAID6_COMPARE_RPARITY_DEAD and RAID6_COMPARE_DPARITY_DEAD are
 *  set when the corresponding parity disk is dead.  Based on these six bits a
 *  course of action is taken.  The table below describes the various cases:
 *
 * Result        | Row Dead | Diag Dead | Data | RCsum | DCsum | Csums
 * -------------------------------------------------------------------
 * Row           |    0     |     0     |   1  |   1   |   1   |   1
 * NA            |    0     |     0     |   1  |   1   |   1   |   0
 * NA            |    0     |     0     |   1  |   1   |   0   |   1
 * poc, RB Diag  |    0     |     0     |   1  |   1   |   0   |   0
 * NA            |    0     |     0     |   1  |   0   |   1   |   1
 * poc, RB Row   |    0     |     0     |   1  |   0   |   1   |   0
 * npoc/crc, inv |    0     |     0     |   1  |   0   |   0   |   1
 * npoc, inv     |    0     |     0     |   1  |   0   |   0   |   0
 * coh, inv      |    0     |     0     |   0  |   1   |   1   |   1
 * coh, inv      |    0     |     0     |   0  |   1   |   1   |   0
 * coh, RB Diag  |    0     |     0     |   0  |   1   |   0   |   1
 * npoc, RB Diag |    0     |     0     |   0  |   1   |   0   |   0
 * coh, RB Row   |    0     |     0     |   0  |   0   |   1   |   1
 * npoc, RB Row  |    0     |     0     |   0  |   0   |   1   |   0
 * coh, inv      |    0     |     0     |   0  |   0   |   0   |   1
 * npoc, inv     |    0     |     0     |   0  |   0   |   0   |   0
 * Diag, RB Row  |    1     |     0     |  0/1 |  0/1  |   1   |  0/1
 * inv           |    1     |     0     |  0/1 |  0/1  |   0   |  0/1
 * Row, RB Diag  |    0     |     1     |  0/1 |   1   |  0/1  |  0/1
 * inv           |    0     |     1     |  0/1 |   0   |  0/1  |  0/1
 *
 *  Actions Key:
 *  Row     - Use the reconstructed data in the row parity scratch.
 *  Diag    - Use the reconstructed data in the diagonal parity scratch.
 *  NA      - Should not happen.
 *  poc     - Set parity of checksum coherency error.  This error usually
 *            indicates that there is an error in one of the parity of 
 *            checksums.  This is usually the erorr that causes only one 
 *            of the reconstructed checksums to not match the data.
 *  RB Diag - Rebuild the diagonal parity sector.
 *  RB Row  - Rebuild the row parity sector.
 *  npoc    - Set multiple parity of checksum coherency error.  This error 
 *            usually indicates that there is an error in both of the parity 
 *            of checksums.  This is usually the erorr that causes both 
 *            of the reconstructed checksums do not match the data.
 *  crc     - Set uncorrectable crc error.
 *  inv     - Invalidate data disk.
 *  coh     - Set uncorrectable coherency error.  This error usually
 *            indicates that there is an error in one of the data areas of 
 *            a sector.  This is usually the erorr that causes only one 
 *            of the reconstructed data areas to mismatch with all other
 *            reconstructed pieces.
 *  reconstruct_p - [O] - FBE_TRUE if the reconstruct is successful.  The reconstruct 
 *                        is successful if the data sector and matching checksum can be 
 *                        recreated and there is no other data sector that needs to be 
 *                        invalidated.
 *                        FBE_FALSE if the reconstruct fails.  The reconstruct fails if 
 *                        a data sector is reconstructed with a crc error.  The crc error
 *                        could be the result of reconstructing an invalidated block or 
 *                        just a plain failure to get the correct data back. In either case 
 *                        the sector needs to go through the invalidate sector code path next.
 *
 * @return fbe_status_t
 
 *
 * @author
 *  04/04/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_eval_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                            const fbe_u16_t ppos[],
                            fbe_xor_error_t * const eboard_p,
                            const fbe_u16_t bitkey[],
                            fbe_u16_t rebuild_pos[],
                            fbe_bool_t *reconstruct_p)
{
    fbe_u32_t * row_attempt_ptr; /* Attempt to rebuild data from row parity. */
    fbe_u32_t * diag_attempt_ptr;/* Attempt to rebuild data from diagonal parity. */

    /* Checksum of the data reconstructed from row parity. 
     */
    fbe_u32_t row_checksum = 0x0; 

    /* Checksum of the data reconstructed from diagonal parity. 
     */
    fbe_u32_t diag_checksum = 0x0;
    fbe_u16_t row_index;

    /* Integer that accumulates the result of the comparisons between the
     * reconstructed data.
     */
    fbe_u16_t reconstruct_results;  
    fbe_bool_t return_value = FBE_FALSE;

    fbe_bool_t check_invalid_data;

    fbe_status_t status;

    /* Make sure the pointers we are using are valid.  If they are not then
     * there is no where to go from here.
     */

    if (XOR_COND(scratch_p->fatal_blk[0] == NULL))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "scratch_p->fatal_blk[0] == NULL\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (XOR_COND(scratch_p->diag_syndrome == NULL))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "scratch_p->diag_syndrome == NULL\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the reconstructed data.
     */
    row_attempt_ptr = scratch_p->fatal_blk[0]->data_word;
    diag_attempt_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;

    /* Initialize reconstruct_results to assume data matches until we find a 
     * problem.  It is more efficient to change the value when a miscompare 
     * occures since it should be less often.
     */
    reconstruct_results = FBE_XOR_RAID6_COMPARE_DATA_MATCHES;

    /* For each data symbol do two things.
     * 1) Update each checksum.
     * 2) Compare data values.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        XORLIB_REPEAT8( (row_checksum ^= *row_attempt_ptr, 
                      diag_checksum ^= *diag_attempt_ptr,
                      /* If we find a problem unset the data matches bit, since
                       * no other bits have been manipulated yet just zero out 
                       * the whole value.
                       */
                     (*row_attempt_ptr++ != *diag_attempt_ptr++) 
                        ? reconstruct_results = 0x0 
                        : 0 ) );
    }

    /* Compare checksum of reconstructed data against checksum that was
     * reconstructed from the parity of checksums.  If they are the same
     * then set the bit in the results bitmap.
     */

    if (scratch_p->checksum_row_syndrome == 
            xorlib_cook_csum(row_checksum, (fbe_u32_t)scratch_p->seed))
    {
        reconstruct_results |= FBE_XOR_RAID6_COMPARE_RCSUM_MATCHES;
        row_checksum = xorlib_cook_csum(row_checksum, (fbe_u32_t)scratch_p->seed);
    }

    if (scratch_p->checksum_diag_syndrome == 
            xorlib_cook_csum(diag_checksum, (fbe_u32_t)scratch_p->seed))
    {
        reconstruct_results |= FBE_XOR_RAID6_COMPARE_DCSUM_MATCHES;
        diag_checksum = xorlib_cook_csum(diag_checksum, (fbe_u32_t)scratch_p->seed);
    }

    scratch_p->fatal_blk[0]->crc = scratch_p->checksum_row_syndrome;
    ((fbe_sector_t *)scratch_p->diag_syndrome)->crc = scratch_p->checksum_diag_syndrome;

    /* Compare the two checksums reconstructed from parity of checksums
     * against each other.
     */
    if (scratch_p->checksum_row_syndrome == scratch_p->checksum_diag_syndrome)
    {
        reconstruct_results |= FBE_XOR_RAID6_COMPARE_CSUMS_MATCHES;
    }

    /* Check if either parity disk is dead.
     */
    if (FBE_XOR_IS_BITSET(scratch_p->fatal_key, ppos[FBE_XOR_R6_PARITY_POSITION_ROW]))
    {
        reconstruct_results |= FBE_XOR_RAID6_COMPARE_RPARITY_DEAD;
    }

    if (FBE_XOR_IS_BITSET(scratch_p->fatal_key, ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
    {
        reconstruct_results |= FBE_XOR_RAID6_COMPARE_DPARITY_DEAD;
    }


    /* Based on bits set in reconstruct_results we have constructed an integer
     * value.  Use this value to determine the next course of action.
     */
    switch (reconstruct_results)
    {
    case 15: 
    /* Everything matches, copy reconstructed row checksum to sector->crc.
     */
        status = fbe_xor_handle_case_15(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                           &return_value, row_checksum);
        if (status != FBE_STATUS_OK) { return status; }
        break;


    case 12: 
    /* Reconstructed data sectors match.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */
        status = fbe_xor_handle_case_12(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                           &return_value, row_checksum, 
                           bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 10: 
    /* Reconstructed data sectors match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */ 
        status = fbe_xor_handle_case_10(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                           &return_value, diag_checksum,
                           bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 9: 
    /* Reconstructed data sectors match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums match.
     */ 
        /* Since there is a match between the checksums only one reconstruction
         * could be an invalidated block.  First see if the row reconstruction 
         * is an invalidated block.
         */
        status = fbe_xor_check_for_invalidated_reconstruct(scratch_p->fatal_blk[0],
                                                           eboard_p, bitkey[rebuild_pos[0]], 
                                                           scratch_p->seed,
                                                           &check_invalid_data);
        if (status != FBE_STATUS_OK) { return status; }
        if (check_invalid_data)
        {
            fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
            eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap,
                                  FBE_XOR_ERR_TYPE_CRC, 20, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }
        }
        else /* The rebuild did not reconstruct any invalidated blocks. */
        {
            /* The unknown single or multi bit crc error that was added in 
             * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
             * we now know it is an u_n_poc_coh_bitmap error.
             */
            eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
            eboard_p->crc_multi_bitmap &= ~bitkey[rebuild_pos[0]];

            eboard_p->u_n_poc_coh_bitmap |= bitkey[rebuild_pos[0]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_n_poc_coh_bitmap,
                                  FBE_XOR_ERR_TYPE_N_POC_COH, 2, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }
        }

        return_value = FBE_FALSE;
        break;

    case 8: 
    /* Reconstructed data sectors match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data do not match reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */

        /* First see if the row reconstruction is an invalidated block.
         */
        status = fbe_xor_check_for_invalidated_reconstruct(scratch_p->fatal_blk[0],
                                                           eboard_p, bitkey[rebuild_pos[0]],
                                                           scratch_p->seed,
                                                           &check_invalid_data);
        if (status != FBE_STATUS_OK) { return status; }
        if (check_invalid_data)
        {
            fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
            eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap,
                                  FBE_XOR_ERR_TYPE_CRC, 21, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }

            /* Mark the diagonal parity with a POC coherency error.
             * We will re-invalidate data and reconstruct parity later on
             * because we have an uncorrectable.
             */
            eboard_p->c_poc_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_poc_coh_bitmap,
                                  FBE_XOR_ERR_TYPE_POC_COH, 13, FBE_FALSE);
            if (status != FBE_STATUS_OK) { return status; }
        }
        /* Next, see if the diagonal reconstruction is an invalidated block.
         */
        else 
        {
            status = fbe_xor_check_for_invalidated_reconstruct((fbe_sector_t *)scratch_p->diag_syndrome,
                                                               eboard_p, bitkey[rebuild_pos[0]], 
                                                               scratch_p->seed,
                                                               &check_invalid_data);
            if (status != FBE_STATUS_OK) { return status; }

            if (check_invalid_data)
            {
                /* The unknown single or multi bit crc error that was added in 
                * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                * we now know it is an u_crc_bitmap and an c_poc_coh_bitmap error.
                */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];
    
                fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
                eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap,
                                          FBE_XOR_ERR_TYPE_CRC, 22, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }

                /* Mark the row parity with a POC coherency error.
                 * We will re-invalidate data and reconstruct parity later on
                 * because we have an uncorrectable.
                 */
                eboard_p->c_poc_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_poc_coh_bitmap, 
                                  FBE_XOR_ERR_TYPE_POC_COH, 14, FBE_FALSE);
                if (status != FBE_STATUS_OK) { return status; }
             }
            else /* The rebuild did not reconstruct any invalidated blocks. */
            {
                /* The unknown single or multi bit crc error that was added in 
                * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                * we now know it is an u_n_poc_coh_bitmap error.
                */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];

                eboard_p->u_n_poc_coh_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_n_poc_coh_bitmap,
                                      FBE_XOR_ERR_TYPE_N_POC_COH, 5, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }
            }
        }

        return_value = FBE_FALSE;
        break;

    case 0: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */

        /* First see if the row reconstruction is an invalidated block.
         */

        status = fbe_xor_check_for_invalidated_reconstruct(scratch_p->fatal_blk[0],
                                                  eboard_p, bitkey[rebuild_pos[0]], 
                                                  scratch_p->seed,
                                                  &check_invalid_data);

        if (status != FBE_STATUS_OK) { return status; }

        if (check_invalid_data)
        {
            fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
            eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap, 
                                               FBE_XOR_ERR_TYPE_CRC, 23, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }

            /* Mark the diagonal parity with a coherency error.
             * We will re-invalidate data and reconstruct parity later on
             * because we have an uncorrectable.
             */
            eboard_p->c_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap, 
                                               FBE_XOR_ERR_TYPE_COH, 11, FBE_FALSE);
            if (status != FBE_STATUS_OK) { return status; }
        }
        /* Next, see if the diagonal reconstruction is an invalidated block.
         */
        else 
        {
            status = fbe_xor_check_for_invalidated_reconstruct((fbe_sector_t *)scratch_p->diag_syndrome,
                                                               eboard_p, bitkey[rebuild_pos[0]], 
                                                               scratch_p->seed,
                                                               &check_invalid_data);

            if (status != FBE_STATUS_OK) { return status; }

            if (check_invalid_data)
            {
                /* The unknown single or multi bit crc error that was added in 
                 * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                 * we now know it is an u_crc_bitmap and an c_coh_bitmap error.
                 */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];

                fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
                eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap,
                                                   FBE_XOR_ERR_TYPE_CRC, 24, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }

                /* Mark the row parity with a coherency error.
                 * We will re-invalidate data and reconstruct parity later on
                 * because we have an uncorrectable.
                    */
                eboard_p->c_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap,
                                                   FBE_XOR_ERR_TYPE_COH, 12, FBE_FALSE);
                if (status != FBE_STATUS_OK) { return status; }
            }       
            else /* The rebuild did not reconstruct any invalidated blocks. */
            {
                /* The unknown single or multi bit crc error that was added in 
                * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                * we now know it is an u_coh_bitmap error.
                */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];

                eboard_p->u_coh_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap,
                                                   FBE_XOR_ERR_TYPE_COH, 13, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }
            }
        }

        return_value = FBE_FALSE;
        break;

    case 7: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstructed checksums match.
     */

    case 6: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */
        /* Set uncorrectable coherency error in eboard.
         */
        eboard_p->u_coh_bitmap |= bitkey[rebuild_pos[0]];
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap,
                                           FBE_XOR_ERR_TYPE_COH, 4, FBE_TRUE);
        if (status != FBE_STATUS_OK) { return status; }

        /* Setup invalid bitmask and scratch state to invalidate rebuild position.
         */
        return_value = FBE_FALSE;
        break;

    case 1: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums match.
     */
        /* Since there is a match between the checksums only one reconstruction
         * could be an invalidated block.  First see if the row reconstruction 
         * is an invalidated block.
         */
        status = fbe_xor_check_for_invalidated_reconstruct(scratch_p->fatal_blk[0],
                                                  eboard_p, bitkey[rebuild_pos[0]],
                                                  scratch_p->seed,
                                                  &check_invalid_data);
        if (status != FBE_STATUS_OK) { return status; }

        if (check_invalid_data)
        {
            fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
            eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap, 
                                               FBE_XOR_ERR_TYPE_CRC, 25, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }

            eboard_p->c_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap,
                                               FBE_XOR_ERR_TYPE_COH, 14, FBE_FALSE);
            if (status != FBE_STATUS_OK) { return status; }
        }
        /* Next, see if the diagonal reconstruction is an invalidated block.
         */
        else 
        {
            status = fbe_xor_check_for_invalidated_reconstruct((fbe_sector_t *)scratch_p->diag_syndrome,
                                                       eboard_p, bitkey[rebuild_pos[0]], 
                                                       scratch_p->seed,
                                                       &check_invalid_data);

            if (status != FBE_STATUS_OK) { return status; }
            if (check_invalid_data)
            {
                /* The unknown single or multi bit crc error that was added in 
                * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                * we now know it is an u_crc_bitmap and an c_coh_bitmap error.
                */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];

                fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[0]]);
                eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap,
                                                   FBE_XOR_ERR_TYPE_CRC, 26, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }

                eboard_p->c_coh_bitmap |= bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap,
                                                   FBE_XOR_ERR_TYPE_COH, 15, FBE_FALSE);
                if (status != FBE_STATUS_OK) { return status; }
            }
            else /* The rebuild did not reconstruct any invalidated blocks. */
            {
                /* The unknown single or multi bit crc error that was added in 
                * fbe_xor_check_for_invalidated_reconstruct needs to be removed since
                * we now know it is an u_coh_bitmap error.
                */
                eboard_p->crc_single_bitmap &= ~bitkey[rebuild_pos[0]];
                eboard_p->crc_multi_bitmap  &= ~bitkey[rebuild_pos[0]];

                eboard_p->u_coh_bitmap |= bitkey[rebuild_pos[0]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap, 
                                                   FBE_XOR_ERR_TYPE_COH, 16, FBE_TRUE);
                if (status != FBE_STATUS_OK) { return status; }
            }
        }
        return_value = FBE_FALSE;
        break;

    case 5: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums match.
     */
        status = fbe_xor_handle_case_05(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                           &return_value, row_checksum, 
                           bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 4: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */
        status = fbe_xor_handle_case_04(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                           &return_value, row_checksum, 
                           bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;
        
    case 3: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstructed checksums match.
     */
        status = fbe_xor_handle_case_03(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                                        &return_value, diag_checksum, 
                                        bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 2: 
    /* Reconstructed data sectors do not match.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstructed checksums do not match.
     */
        status = fbe_xor_handle_case_02(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                                        &return_value, diag_checksum, 
                                        bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    /* Start cases where row parity disk is invalid.
     */
    case 34:
    case 35:
    case 38:
    case 39:
    case 42:
    case 43:
    case 46:
    case 47:
    /* Row parity disk is invalid.
     * Reconstructed diagonal data matches reconstructed diagonal checksum.
     * Reconstrcuted diagonal data is good, use it and rebuild row parity.
     */
        status = fbe_xor_handle_case_row_bad_data_good(scratch_p, eboard_p, bitkey[rebuild_pos[0]], 
                                          &return_value, diag_checksum, 
                                          bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_ROW]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 32:  
    case 33:  
    case 36:
    case 37:
    case 40:
    case 41:
    case 44:
    case 45:
    /* Row parity disk is invalid.
     * Reconstructed diagonal data does not match reconstructed diagonal checksum.
     * Reconstructed diagonal data is bad, we are stuck.
     */
        status = fbe_xor_handle_case_row_bad_data_bad(scratch_p, eboard_p, 
                                        bitkey[rebuild_pos[0]], &return_value);
        if (status != FBE_STATUS_OK) { return status; }
        break;
    /* End cases where row parity disk is invalid and start cases where
     * diagonal parity disk is invalid.
     */

    case 20:  
    case 21:
    case 22:
    case 23:
    case 28:
    case 29:
    case 30:
    case 31:
    /* Diagonal parity disk is invalid.
     * Reconstructed row data matches reconstructed row checksum.
     * Reconstrcuted row data is good, use it and rebuild diagonal parity.
     */
        status = fbe_xor_handle_case_diag_bad_data_good(scratch_p,
                                                        eboard_p, 
                                                        bitkey[rebuild_pos[0]],
                                                        &return_value,
                                                        row_checksum,
                                                        bitkey[ppos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
        if (status != FBE_STATUS_OK) { return status; }
        break;

    case 16:  
    case 17:
    case 18:
    case 19:
    case 24:
    case 25:
    case 26:
    case 27:
    /* Diagonal parity disk is invalid.
     * Reconstructed row data does not match reconstructed row checksum.
     * Reconstrcuted row data is bad, we are stuck.
     */
        status = fbe_xor_handle_case_diag_bad_data_bad(scratch_p, eboard_p, 
                                          bitkey[rebuild_pos[0]], &return_value);
        if (status != FBE_STATUS_OK) { return status; }
        break;
    /* End cases where diagonal parity disk is invalid.
     */
    default:
        /* Something went wrong!
         * Set uncorrectable unknown coherency error in eboard.
         */
        eboard_p->u_coh_unk_bitmap |= bitkey[rebuild_pos[0]];

        /* Setup invalid bitmask and scratch state to invalidate the rebuild 
         * position.
         */
        return_value = FBE_FALSE;
        break;
    } /* END switch (reconstruct_results) */


    *reconstruct_p = return_value;
    return FBE_STATUS_OK;
} /* fbe_xor_eval_reconstruct_1() */

/***************************************************************************
 *  fbe_xor_raid6_correct_all()
 ***************************************************************************
 *  @brief
 *    This routine is used to mark corrected all which was marked uncorrected.
 *

 * @param eboard_p - error board for marking different types of errors.
 *
 *  @return
 *    NONE
 *
 *  @notes
 *
 *  @author
 *    04/06/06 - Created. RCL
 ***************************************************************************/

void fbe_xor_raid6_correct_all(fbe_xor_error_t * eboard_p)
{
    
    fbe_xor_correct_all(eboard_p);

    eboard_p->c_n_poc_coh_bitmap |= eboard_p->u_n_poc_coh_bitmap,
        eboard_p->u_n_poc_coh_bitmap = 0;
    eboard_p->c_poc_coh_bitmap |= eboard_p->u_poc_coh_bitmap,
        eboard_p->u_poc_coh_bitmap = 0;

    return;
}
/* fbe_xor_raid6_correct_all() */

/* end xor_raid6_reconstruct_1.c */
