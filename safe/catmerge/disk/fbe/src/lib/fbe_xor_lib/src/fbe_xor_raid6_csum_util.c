/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_csum_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to support and implement optimizations for 
 *  RAID 6.
 *
 * @notes
 *
 * @author
 *
 * 05/24/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe/fbe_library_interface.h"

/* Globals for debug functions.
 */
fbe_sector_t row_parity_holder;
fbe_sector_t diag_parity_holder;
fbe_sector_t row_temp_parity_holder;
fbe_sector_t diag_temp_parity_holder;

/* Disabled MMX routines in 64 bit code.
 */
fbe_u32_t R6UseMMX = MMX_UNKNOWN;

extern fbe_u32_t fbe_mmx_cpuid(void);

/* MMX Routines are disabled in 64 bit code.
 */
#ifndef _AMD64_ // SSE2 assembler in xor_csum_a64.asm
/***************************************************************************
 *  fbe_xor_calc_csum_and_init_r6_parity_mmx()
 ***************************************************************************
 * @brief
 *  This function calculates a "raw" checksum (unseeded, unfolded) for
 *  data within a sector using the "classic" checksum algorithm; this is
 *  the first pass over the data so the parity blocks are initialized
 *  based on the data.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal 
 *                         parity
 * @param column_index - The logical index of the disk passed in.
 *      
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *
 * @author
 *  05/24/06 - Created. RCL
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_init_r6_parity_mmx(const fbe_u32_t * src_ptr,
                                         fbe_u32_t * tgt_row_ptr,
                                         fbe_u32_t * tgt_diagonal_ptr,
                                         fbe_u32_t column_index)
{
    fbe_u32_t checksum = 0x0;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * bottom_of_diagonal_parity_sector;
    fbe_u32_t * special_diagonal_parity_symbol;

    /* Holds the pointer to the top diagonal parity block.
     */
    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;

    /* Find the component of the S value in this particular column.  There is
     * no component of the S value contained in the first column. 
     */
    if (column_index == 0)
    {
        /* Since this is the first column there is no s value component, so we
         * will just initialize the s value to zero for each of the words that
         * make up this symbol.
         */
        fbe_zero_memory(&s_value, sizeof(fbe_xor_r6_symbol_size_t));
    }
    else
    {
        /* Since this is not the first column find the S value component which
         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
         */
        s_value = *(((fbe_xor_r6_symbol_size_t*)src_ptr) + 
                    ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) );
    }

    /* Find the address of the symbol we need to start at in the diagonal
     * parity sector.
     */
    tgt_diagonal_ptr = (fbe_u32_t *)(tgt_diagonal_ptr_holder + column_index);

    /* Find the address of the bottom of the diagonal parity sector so we
     * don't go past it.
     */
    bottom_of_diagonal_parity_sector = (fbe_u32_t *)(tgt_diagonal_ptr_holder + 
                                                    FBE_XOR_R6_SYMBOLS_PER_SECTOR);

    /* Find the address of the diagonal parity symbol that will not get the
     * source data XORed into it.  There is one of these symbols for each 
     * column except for column 0.
     */
    special_diagonal_parity_symbol = tgt_diagonal_ptr - FBE_XOR_WORDS_PER_SYMBOL;

    /* For each data symbol do three things.
     * 1) Update the checksum.                      (checksum ^= *src_ptr)
     * 2) Set the correct row parity symbol.        (*tgt_row_ptr++ = *src_ptr)
     * 3) Set the correct diagonal parity symbol.   (*tgt_diagonal_ptr++ = *src_ptr++ ^ s_value)
     */ 

    __asm {
        /* Start the prefetching and save the old state.
         */
        prefetchnta src_ptr
        push ebx
        push ecx
        push edx

        /* Store the S value in mmx registers 1, 2, 3, and 4.  This is 32 bytes
         * we are going to use for every symbol so lets keep it handy.
         */
        lea ebx, s_value
        movq mm1, [ebx]
        movq mm2, [ebx+8]
        movq mm3, [ebx+16]
        movq mm4, [ebx+24]

        /* Get the addresses of the source data, the row parity sector, and the
         * diagonal parity sector.
         */
        mov ebx, src_ptr
        mov ecx, tgt_row_ptr
        mov edx, tgt_diagonal_ptr

        /* Lets do some more prefetching.  We prefetch ahead 128 bytes because
         * the prefetches are done in 128 bytes chunks and we started the first
         * 128 bytes already.
         */
        prefetchnta [ebx+128]

        /* Zero out the register we will store the checksum in and init the 
         * loop counter.
         */
        pxor mm0, mm0
        mov eax, 16

    do_loop:
        /* We have prefetched 2 ahead. Therefore we will fetch off of the end.  
         * This is intentional, this size sets us up for the next sector.
         */
        prefetchnta [ebx+256]

        /* For each 64 bits of source data we need to: 
         *  1) update the checksum;
         *  2) copy the source into the row parity sector;
         *  3) copy the source XORed with the S value into the diagonal parity 
         *       sector.
         */
        movq mm5, [ebx]     ; load src
        movq mm6, [ebx+8]   ; load second 64 bits now so it will be ready when needed.
        movq mm7, [ebx+16]  ; load third 64 bits now so it will be ready when needed.

        movq [ecx], mm5     ; copy into row target
        pxor mm0, mm5       ; running checksum
        pxor mm5, mm1       ; xor in s value
        movq [edx], mm5     ; copy into diagonal target
        movq mm5, [ebx+24]  ; load fourth 64 bits now so it will be ready when needed.

        movq [ecx+8], mm6
        pxor mm0, mm6
        pxor mm6, mm2
        movq [edx+8], mm6

        movq [ecx+16], mm7
        pxor mm0, mm7
        pxor mm7, mm3
        movq [edx+16], mm7

        movq [ecx+24], mm5
        pxor mm0, mm5
        pxor mm5, mm4
        movq [edx+24], mm5

        /* We have processed 32 bytes of source data so lets move on to the next
         * 32 bytes.
         */
        add ebx, 32 
        add ecx, 32
        add edx, 32 

        /* Decrement the loop counter.
         */
        dec eax

        /* If the loop counter is zero we are done with the loop, if not go back
         * to the top of the loop.
         */
        jz loop_done

        /* If we hit the bottom of the diagonal parity sector we are going to
         * handle the next 32 bytes differently, fall into that section of code.
         */
        cmp edx, bottom_of_diagonal_parity_sector
        jnz do_loop

        /* This block of code handles the one diagonal parity symbol that does
         * not contain any of the source data, just the S value.  This block
         * should be executed at most once for any one data sector.
         */

        /* Start by going to the address of the special diagonal parity symbol.
         */
        mov edx, special_diagonal_parity_symbol

        /* For each 64 bits of source data we need to: 
         *  1) update the checksum;
         *  2) copy the source into the row parity sector;
         *  3) copy the S value into the diagonal parity sector.
         */
        movq mm5, [ebx]        ; load src
        movq mm6, [ebx+8]   ; load second 64 bits now so it will be ready when needed.
        movq mm7, [ebx+16]  ; load third 64 bits now so it will be ready when needed.

        pxor mm0, mm5        ; running checksum
        movq [ecx], mm5        ; copy into row target
        movq [edx], mm1     ; copy into diagonal target
        movq mm5, [ebx+24]  ; load fourth 64 bits now so it will be ready when needed.

        pxor mm0, mm6
        movq [ecx+8], mm6
        movq [edx+8], mm2

        pxor mm0, mm7
        movq [ecx+16], mm7
        movq [edx+16], mm3

        pxor mm0, mm5
        movq [ecx+24], mm5
        movq [edx+24], mm4

        /* We have processed 32 bytes of source data so lets move on to the next
         * 32 bytes.  Move the diagonal parity pointer to the top of that sector.
         */
        add ebx, 32
        add ecx, 32
        mov edx, tgt_diagonal_ptr_holder

        /* Decrement the loop counter.  This is still part of the loop, just a 
         * special case.
         */
        dec eax

        /* If the loop counter is zero we are done with the loop, if not go back
         * to the top of the loop.
         */
        jnz do_loop

    loop_done:
        /* We are done processing all 512 bytes of the source sector so clean
         * up the checksum and put it in the return variable.
         */
        movq mm6, mm0       ; 64bit checksum
        psrlq mm0, 32       ; high 32bits
        pxor mm0, mm6       ; xor hi/lo
        movd checksum, mm0  ; ret 32bit checksum 
        emms                ; MMX end

        /* Return the registers to their initial state.
         */
        pop edx
        pop ecx
        pop ebx
    }

    return (checksum);
} /* end fbe_xor_calc_csum_and_init_r6_parity_mmx */

/***************************************************************************
 * fbe_xor_calc_csum_and_update_r6_parity_mmx()
 ***************************************************************************
 * @brief
 *  This function calculates a "raw" checksum (unseeded, unfolded) for
 *      data within a sector using the "classic" checksum algorithm; the
 *  parity values are updated based on the data.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal
 *                         parity
 * @param column_index - The logical index of the disk passed in.
 *
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *
 * @author
 *  05/24/06 - Created. RCL
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_update_r6_parity_mmx(const fbe_u32_t * src_ptr,
                                           fbe_u32_t * tgt_row_ptr,
                                           fbe_u32_t * tgt_diagonal_ptr,
                                           fbe_u32_t column_index)
{
    fbe_u32_t checksum = 0x0;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * bottom_of_diagonal_parity_sector;
    fbe_u32_t * special_diagonal_parity_symbol;

    /* Holds the pointer to the top diagonal parity block.
     */
    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;

    /* Find the component of the S value in this particular column.  There is
     * no component of the S value contained in the first column. 
     */
    if (column_index == 0)
    {
        /* Since this is the first column there is no s value component, so we
         * will just initialize the s value to zero for each of the words that
         * make up this symbol.
         */
        fbe_zero_memory(&s_value, sizeof(fbe_xor_r6_symbol_size_t));
    }
    else
    {
        /* Since this is not the first column find the S value component which
         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
         */
        s_value = *(((fbe_xor_r6_symbol_size_t*)src_ptr) + 
                    ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) );
    }

    /* Find the address of the symbol we need to start at in the diagonal
     * parity sector.
     */
    tgt_diagonal_ptr = (fbe_u32_t *)(tgt_diagonal_ptr_holder + column_index);

    /* Find the address of the bottom of the diagonal parity sector so we
     * don't go past it.
     */
    bottom_of_diagonal_parity_sector = (fbe_u32_t *)(tgt_diagonal_ptr_holder + 
                                                    FBE_XOR_R6_SYMBOLS_PER_SECTOR);

    /* Find the address of the diagonal parity symbol that will not get the
     * source data XORed into it.  There is one of these symbols for each 
     * column except for column 0.
     */
    special_diagonal_parity_symbol = tgt_diagonal_ptr - FBE_XOR_WORDS_PER_SYMBOL;

    /* For each data symbol do three things.
     * 1) Update the checksum.                      (checksum ^= *src_ptr)
     * 2) Set the correct row parity symbol.        (*tgt_row_ptr++ ^= *src_ptr)
     * 3) Set the correct diagonal parity symbol.   (*tgt_diagonal_ptr++ ^= *src_ptr++ ^ s_value)
     */

    __asm {
        /* Start the prefetching and save the old state.
         */
        prefetchnta src_ptr
        prefetchnta tgt_row_ptr
        prefetchnta tgt_diagonal_ptr
        push ebx
        push ecx
        push edx

        /* Store the S value in mmx registers 1, 2, 3, and 4.  This is 32 bytes
         * we are going to use for every symbol so lets keep it handy.
         */
        lea ebx, s_value
        movq mm1, [ebx]
        movq mm2, [ebx+8]
        movq mm3, [ebx+16]
        movq mm4, [ebx+24]

        /* Get the addresses of the source data, the row parity sector, and the
         * diagonal parity sector.
         */
        mov ebx, src_ptr
        mov ecx, tgt_row_ptr
        mov edx, tgt_diagonal_ptr

        /* Lets do some more prefetching.  We prefetch ahead 128 bytes because
         * the prefetches are done in 128 bytes chunks and we started the first
         * 128 bytes already.
         */
        prefetchnta [ebx+128]
        prefetchnta [ecx+128]
        prefetchnta [edx+128]

        /* Zero out the register we will store the checksum in and init the 
         * loop counter.
         */
        pxor mm0, mm0
        mov eax, 16

    do_loop:
        /* We have prefetched 2 ahead. Therefore we will fetch off of the end.  
         * This is intentional, this size sets us up for the next sector.
         */
        prefetchnta [ebx+256]
        prefetchnta [ecx+256]
        prefetchnta [edx+256]

        /* For each 64 bits of source data we need to: 
         *  1) make a copy of the source;
         *  2) update the checksum;
         *  3) XOR the row parity into the source;
         *  4) copy the source into the row parity sector;
         *  5) XOR the S value into the source;
         *  6) XOR the diagonal parity into the source;
         *  7) copy the source into the diagonal parity sector.
         */
        movq mm5, [ebx]     ; load src
        movq mm7, [ebx+8]   ; load second 64 bits now so it will be ready when needed.


        movq mm6, mm5       ; make a copy of the source
        pxor mm0, mm5        ; running checksum
        pxor mm5, [ecx]     ; xor the row parity into the source
        movq [ecx], mm5        ; copy into row target
        movq mm5, [ebx+16]  ; load third 64 bits now so it will be ready when needed.
        pxor mm6, mm1       ; xor in s value
        pxor mm6, [edx]     ; xor the diagonal parity into the source
        movq [edx], mm6     ; copy into diagonal target

        movq mm6, mm7
        pxor mm0, mm7
        pxor mm7, [ecx+8]
        movq [ecx+8], mm7
        movq mm7, [ebx+24]  ; load fourth 64 bits now so it will be ready when needed.
        pxor mm6, mm2
        pxor mm6, [edx+8]
        movq [edx+8], mm6

        movq mm6, mm5
        pxor mm0, mm5
        pxor mm5, [ecx+16]
        movq [ecx+16], mm5
        pxor mm6, mm3
        pxor mm6, [edx+16]
        movq [edx+16], mm6

        movq mm6, mm7
        pxor mm0, mm7
        pxor mm7, [ecx+24]
        movq [ecx+24], mm7
        pxor mm6, mm4
        pxor mm6, [edx+24]
        movq [edx+24], mm6

        /* We have processed 32 bytes of source data so lets move on to the next
         * 32 bytes.
         */
        add ebx, 32
        add ecx, 32
        add edx, 32 

        /* Decrement the loop counter.
         */
        dec eax

        /* If the loop counter is zero we are done with the loop, if not go back
         * to the top of the loop.
         */
        jz loop_done

        /* If we hit the bottom of the diagonal parity sector we are going to
         * handle the next 32 bytes differently, fall into that section of code.
         */
        cmp edx, bottom_of_diagonal_parity_sector
        jnz do_loop

        /* This block of code handles the one diagonal parity symbol that does
         * not contain any of the source data, just the S value.  This block
         * should be executed at most once for any one data sector.
         */

        /* Start by going to the address of the special diagonal parity symbol.
         */
        mov edx, special_diagonal_parity_symbol

        /* For each 64 bits of source data we need to: 
         *  1) update the checksum;
         *  2) XOR the row parity into the source;
         *  3) copy the source into the row parity sector;
         *  4) XOR the S value into the diagonal parity;
         *  5) copy into the diagonal parity sector.
         */
        movq mm5, [ebx]        ; load src
        movq mm7, [ebx+8]   ; load second 64 bits now so it will be ready when needed.

        movq mm6, [edx]     ; get diagonal parity
        pxor mm0, mm5        ; running checksum
        pxor mm5, [ecx]     ; xor the row parity into the source
        movq [ecx], mm5        ; copy into row target
        pxor mm6, mm1       ; xor in s value to diagonal parity
        movq [edx], mm6     ; copy into diagonal target
        movq mm5, [ebx+16]  ; load third 64 bits now so it will be ready when needed.

        movq mm6, [edx+8]
        pxor mm0, mm7
        pxor mm7, [ecx+8]
        movq [ecx+8], mm7
        pxor mm6, mm2
        movq [edx+8], mm6
        movq mm7, [ebx+24]  ; load fourth 64 bits now so it will be ready when needed.

        movq mm6, [edx+16]
        pxor mm0, mm5
        pxor mm5, [ecx+16]
        movq [ecx+16], mm5
        pxor mm6, mm3
        movq [edx+16], mm6

        movq mm6, [edx+24]
        pxor mm0, mm7
        pxor mm7, [ecx+24]
        movq [ecx+24], mm7
        pxor mm6, mm4
        movq [edx+24], mm6

        /* We have processed 32 bytes of source data so lets move on to the next
         * 32 bytes.  Move the diagonal parity pointer to the top of that sector.
         */
        add ebx, 32
        add ecx, 32
        mov edx, tgt_diagonal_ptr_holder

        /* Decrement the loop counter.  This is still part of the loop, just a 
         * special case.
         */
        dec eax

        /* If the loop counter is zero we are done with the loop, if not go back
         * to the top of the loop.
         */
        jnz do_loop

    loop_done:
        /* We are done processing all 512 bytes of the source sector so clean
         * up the checksum and put it in the return variable.
         */
        movq mm7, mm0       ; 64bit checksum
        psrlq mm0, 32       ; high 32bits
        pxor mm0, mm7       ; xor hi/lo
        movd checksum, mm0  ; ret 32bit checksum 
        emms                ; MMX end

        /* Return the registers to their initial state.
         */
        pop edx
        pop ecx
        pop ebx
    }


    return (checksum);
} /* end fbe_xor_calc_csum_and_update_r6_parity_mmx */
#else
extern fbe_u32_t fbe_xor_calc_csum_and_init_r6_parity_mmx(const fbe_u32_t * src_ptr, 
                                                    fbe_u32_t * tgt_row_ptr,  
                                                    fbe_u32_t * tgt_diagonal_ptr,
                                                    fbe_u32_t column_index);

extern fbe_u32_t fbe_xor_calc_csum_and_update_r6_parity_mmx(const fbe_u32_t * src_ptr,
                                                      fbe_u32_t * tgt_row_ptr,
                                                      fbe_u32_t * tgt_diagonal_ptr,
                                                      fbe_u32_t column_index);

extern UINT_32 xor_calc_csum_and_init_r6_parity_to_temp_mmx (const UINT_32E * src_ptr, 
                                                         UINT_32E * tgt_row_ptr,  
                                                         UINT_32E * tgt_diagonal_ptr,
                                                         const UINT_32 column_index);

extern UINT_32 xor_calc_csum_and_update_r6_parity_to_temp_mmx (const UINT_32E * src_ptr,
                                           UINT_32E * tgt_temp_row_ptr,
                                           UINT_32E * tgt_temp_diagonal_ptr,
                                           const UINT_32 column_index);

extern UINT_32 xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_mmx (const UINT_32E * src_ptr,
                                           const UINT_32E * tgt_temp_row_ptr,
                                           const UINT_32E * tgt_temp_diagonal_ptr,
                                           const UINT_32 column_index,
                                           UINT_32E * tgt_row_ptr,
                                           UINT_32E * tgt_diagonal_ptr);
#endif

#if DBG
/***************************************************************************
 *  fbe_xor_calc_csum_and_init_r6_parity_debug()
 ***************************************************************************
 * @brief
 *  This function calls both fbe_xor_calc_csum_and_init_r6_parity functions and 
 *  compares their returns to check for correctness.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal 
 *                         parity
 * @param column_index - The index of the disk passed in.
 *      
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *   This function is for debugging only.  It will never be called once the 
 *   functionality of the mmx optimizations is proven.
 *
 * @author
 *  05/24/06 - Created. RCL
 ***************************************************************************/
fbe_u32_t fbe_xor_calc_csum_and_init_r6_parity_debug(const fbe_u32_t * src_ptr,
                                         fbe_u32_t * tgt_row_ptr,
                                         fbe_u32_t * tgt_diagonal_ptr,
                                         fbe_u32_t column_index)
{
    fbe_u32_t checksum1;
    fbe_u32_t checksum2;
    fbe_bool_t is_parity_same = FBE_TRUE;
    fbe_u32_t * row_parity_holder_ptr;
    fbe_u32_t * diag_parity_holder_ptr;
    fbe_u16_t index;

    /* Make a "scratch" copy for the computations.
     */
    row_parity_holder_ptr = (fbe_u32_t *) &row_parity_holder;
    diag_parity_holder_ptr = (fbe_u32_t *) &diag_parity_holder;

    checksum1 = fbe_xor_calc_csum_and_init_r6_parity_to_temp_mmx(src_ptr,
                                                     tgt_row_ptr,
                                                     tgt_diagonal_ptr,
                                                     column_index);

    checksum2 = fbe_xor_calc_csum_and_init_r6_parity_non_mmx(src_ptr,
                                                         row_parity_holder_ptr,
                                                         diag_parity_holder_ptr,
                                                         column_index);

    /* If there is a miscompare, fbe_panic so we can catch it while we have
     * both results.
     */
    if (checksum1 != checksum2)
    {
        /* Since this is a debug function and will not be included in
         * a release. We will print critical error. 
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "RAID6: Checksum mismatch for sector mapped in at 0x%p", src_ptr);
    }

    /* Compare the results of the two methods and if they don't match
     * set the flag to FBE_FALSE.
     */
    for (index = 0; index < FBE_WORDS_PER_BLOCK; index++)
    {
        if ((*row_parity_holder_ptr != *tgt_row_ptr) ||
            (*diag_parity_holder_ptr != *tgt_diagonal_ptr))
        {
            is_parity_same = FBE_FALSE;

            /* Since this is a debug function and will not be included in
             * a release. We will report it as a critical error.
             */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                                  "RAID6: Parity mismatch for sector mapped in at 0x%p",
                                  src_ptr);
        }
        row_parity_holder_ptr++;
        tgt_row_ptr++;
        diag_parity_holder_ptr++;
        tgt_diagonal_ptr++;
    }

    return (checksum1);
} /* end fbe_xor_calc_csum_and_init_r6_parity_cmp_op */

/***************************************************************************
 * fbe_xor_calc_csum_and_update_r6_parity_debug()
 ***************************************************************************
 * @brief
 *  This function calculates a "raw" checksum (unseeded, unfolded) for
 *      data within a sector using the "classic" checksum algorithm; the
 *  parity values are updated based on the data.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal
 *                         parity
 * @param column_index - The index of the disk passed in.
 *
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *   This function is for debugging only.  It will never be called once the 
 *   functionality of the mmx optimizations is proven.
 *
 * @author
 *  05/24/06 - Created. RCL
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_update_r6_parity_debug(const fbe_u32_t * src_ptr,
                                           fbe_u32_t * tgt_row_ptr,
                                           fbe_u32_t * tgt_diagonal_ptr,
                                           fbe_u32_t column_index)
{
    fbe_u32_t checksum1;
    fbe_u32_t checksum2;
    fbe_bool_t is_parity_same = FBE_TRUE;
    fbe_u32_t * row_parity_holder_ptr;
    fbe_u32_t * diag_parity_holder_ptr;
    fbe_u16_t index;

    /* Make a "scratch" copy for the computations.
     */
    row_parity_holder_ptr = (fbe_u32_t *) &row_parity_holder;
    diag_parity_holder_ptr = (fbe_u32_t *) &diag_parity_holder;

    memcpy(row_parity_holder_ptr, tgt_row_ptr, sizeof(fbe_sector_t));
    memcpy(diag_parity_holder_ptr, tgt_diagonal_ptr, sizeof(fbe_sector_t));

    checksum1 = fbe_xor_calc_csum_and_update_r6_parity_to_temp_mmx(src_ptr,
                                                       tgt_row_ptr,
                                                       tgt_diagonal_ptr,
                                                       column_index);

    checksum2 = fbe_xor_calc_csum_and_update_r6_parity_non_mmx(src_ptr,
                                                       row_parity_holder_ptr,
                                                       diag_parity_holder_ptr,
                                                       column_index);

    /* If there is a miscompare, fbe_panic so we can catch it while we have
     * both results.
     */
    if ((checksum1 != checksum2) ||
        (!is_parity_same))
    {
        /* Since this is a debug function and will not be included in
         * a release it but we will report it as a critical error
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "RAID6: Checksum mismatch for sector mapped in at 0x%p",
                              src_ptr);
    }

    /* Compare the results of the two methods and if they don't match
     * set the flag to FBE_FALSE.
     */
    for (index = 0; index < FBE_WORDS_PER_BLOCK; index++)
    {
        if ((*row_parity_holder_ptr != *tgt_row_ptr) ||
            (*diag_parity_holder_ptr != *tgt_diagonal_ptr))
        {
            is_parity_same = FBE_FALSE;

            /* Since this is a debug function and will not be included in
             * a release but we will report it as a critical error.
             */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                                  "RAID6: Parity mismatch for sector mapped in at0x %p",
                                  src_ptr);
        }
        row_parity_holder_ptr++;
        tgt_row_ptr++;
        diag_parity_holder_ptr++;
        tgt_diagonal_ptr++;
    }
   
    return (checksum1);
} /* end fbe_xor_calc_csum_and_update_r6_parity_debug */

/***************************************************************************
 * fbe_xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_debug()
 ***************************************************************************
 * @brief
 *  
 *
 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_temp_row_ptr - ptr to first word of temp sector of row parity
 * @param tgt_temp_diagonal_ptr - ptr to first word of temp sector of diagonal
 *                         parity
 * @param column_index - The index of the disk passed in.
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal
 *                         parity
 *
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *   This function is for debugging only.  It will never be called once the 
 *   functionality of the mmx optimizations is proven.
 *
 * @author
 *  10/07/13 - Created. Vamsi V.
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_debug(const fbe_u32_t * src_ptr,
                                           fbe_u32_t * tgt_temp_row_ptr,
                                           fbe_u32_t * tgt_temp_diagonal_ptr,
                                           fbe_u32_t column_index,
                                           fbe_u32_t * tgt_row_ptr,
                                           fbe_u32_t * tgt_diagonal_ptr)
{
    fbe_u32_t checksum1;
    fbe_u32_t checksum2;
    fbe_bool_t is_parity_same = FBE_TRUE;
    fbe_u32_t * row_parity_holder_ptr;
    fbe_u32_t * diag_parity_holder_ptr;
    fbe_u16_t index;

    /* Make a "scratch" copy for the computations.
     */
    row_parity_holder_ptr = (fbe_u32_t *) &row_parity_holder;
    diag_parity_holder_ptr = (fbe_u32_t *) &diag_parity_holder;
    row_temp_parity_holder_ptr = (fbe_u32_t *) &row_temp_parity_holder;
    diag_temp_parity_holder_ptr = (fbe_u32_t *) &diag_temp_parity_holder;

    memcpy(row_parity_holder_ptr, tgt_row_ptr, sizeof(fbe_sector_t));
    memcpy(diag_parity_holder_ptr, tgt_diagonal_ptr, sizeof(fbe_sector_t));
    memcpy(row_temp_parity_holder_ptr, tgt_temp_row_ptr, sizeof(fbe_sector_t));
    memcpy(diag_temp_parity_holder_ptr, tgt_temp_diagonal_ptr, sizeof(fbe_sector_t));

    checksum1 = fbe_xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_mmx(src_ptr,
                                                       tgt_temp_row_ptr,
                                                       tgt_temp_diagonal_ptr,
                                                       column_index,
                                                       tgt_row_ptr,
                                                       tgt_diagonal_ptr,);

    checksum2 = fbe_xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_non_mmx(src_ptr,
                                                       row_parity_holder_ptr,
                                                       diag_parity_holder_ptr,
                                                       column_index,
                                                       row_temp_parity_holder_ptr,
                                                       diag_temp_parity_holder_ptr);

    /* If there is a miscompare, fbe_panic so we can catch it while we have
     * both results.
     */
    if ((checksum1 != checksum2) ||
        (!is_parity_same))
    {
        /* Since this is a debug function and will not be included in
         * a release it but we will report it as a critical error
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "RAID6: Checksum mismatch for sector mapped in at 0x%p",
                              src_ptr);
    }

    /* Compare the results of the two methods and if they don't match
     * set the flag to FBE_FALSE.
     */
    for (index = 0; index < FBE_WORDS_PER_BLOCK; index++)
    {
        if ((*row_parity_holder_ptr != *tgt_row_ptr) ||
            (*diag_parity_holder_ptr != *tgt_diagonal_ptr))
        {
            is_parity_same = FBE_FALSE;

            /* Since this is a debug function and will not be included in
             * a release but we will report it as a critical error.
             */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                                  "RAID6: Parity mismatch for sector mapped in at0x %p",
                                  src_ptr);
        }
        row_parity_holder_ptr++;
        tgt_row_ptr++;
        diag_parity_holder_ptr++;
        tgt_diagonal_ptr++;
    }
   
    return (checksum1);
} /* end fbe_xor_calc_csum_and_update_r6_parity_to_temp_and_cpy_debug */

#endif /* if DBG */

/***************************************************
 * end xor_raid6_csum_util.c 
 ***************************************************/
