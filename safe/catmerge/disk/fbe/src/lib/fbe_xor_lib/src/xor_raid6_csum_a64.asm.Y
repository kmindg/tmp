;; Copyright (C) EMC Corporation, 2006-2008
;; All rights reserved.
;; Licensed material -- property of EMC Corporation
;;
;; xor_raid6_csum_a64.asm - Raid6 SSE2 XOR functions for Win64.
;; Use 128-bit SSE2 instructions instead of 64-bit MMX instructions.
;; Note intentional prefetch ahead sets up for next sector.
;; Adapted from MMX inline assembler xor_raid6_csum_util.c
;;
;; Assemble with ML64 (empty with ML)
;; Created GSchaffer 07-14-06
;; Unaligned loads.  RCL 01-xx-08
;; Software prefetching pattern. GSchaffer 02-01-08

%ifdef _AMD64_

SECTION .text

;;
;; fbe_xor_calc_csum_and_init_r6_parity_mmx()
;;
;; DESCRIPTION:
;;  This function calculates a "raw" checksum (unseeded, unfolded) for
;;  data within a sector using the "classic" checksum algorithm; this is
;;  the first pass over the data so the parity blocks are initialized
;;  based on the data.
;;
;; PARAMETERS:
;;  rcx  src_ptr, [I] - ptr to first word of source sector data
;;  rdx  tgt_row_ptr, [IO] - ptr to first word of target sector of row parity
;;  r8   tgt_diagonal_ptr, [IO] - ptr to first word of target sector of diagonal parity
;;  r9   column_index, [I] - The index of the disk passed in.
;;      
;; RETURNS:
;;  The "raw" checksum for the data in the sector.
;;
;; NOTES:
;;
;; HISTORY:
;;  05/24/06 - Created. RCL
;;  07/12/06 - SSE2 for Win64. GSchaffer
;;
;; UINT_32 fbe_xor_calc_csum_and_init_r6_parity_mmx(
;;     const UINT_32E * src_ptr,
;;     UINT_32E * tgt_row_ptr,
;;     UINT_32E * tgt_diagonal_ptr,
;;     UINT_32 column_index)
;; Registers:
;;  rcx  src_ptr
;;  rdx  tgt_row_ptr
;;  r8   tgt_diagonal_ptr
;;  r9d  column index
;; Uses:
;;  xmm0 128-bit checksum
;;  xmm1, xmm2 = 32-byte s_value
;;  xmm3, xmm4 = loop processing
;;  xmm5 folding to 32-bit checksum
;; Returns:
;;  eax  32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_init_r6_parity_mmx
fbe_xor_calc_csum_and_init_r6_parity_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
    %define _locals_s$ 24 ; Stack space needed to save locals
%endif ;; __SAFE__ SAFEMESS
%ifndef __SAFE__
    ; save args on stack (debug)
    mov    [rsp+8],  rcx
    mov    [rsp+16], rdx
    mov    [rsp+24], r8
    mov    [rsp+32], r9d
    %define _locals_s$ 40 ; Stack space needed to save locals
%endif ;; __SAFE__ SAFEMESS  - no shadow space in non-MS calling convention

    sub    rsp, _locals_s$ ; Increment stack pointer to accomodate locals

    ; software prefetching src_ptr, defeat hardware prefetch
    prefetchnta [rcx+320]
    prefetchnta [rcx]
    prefetchnta [rcx+384]
    prefetchnta [rcx+64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+192]
    pause

;    UINT_32E * bottom_of_diagonal_parity_sector;
;    UINT_32E * special_diagonal_parity_symbol;
;    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

%define tgt_diagonal_ptr_holder$ 0
%define bottom_of_diagonal_parity_sector$ 8
%define special_diagonal_parity_symbol$ 16

;    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;

    mov    tgt_diagonal_ptr_holder$[rsp], r8

;    /* Find the address of the bottom of the diagonal parity sector so we
;     * don't go past it.
;     */
;    bottom_of_diagonal_parity_sector = (UINT_32E *)(tgt_diagonal_ptr_holder + 
;                            XOR_R6_SYMBOLS_PER_SECTOR);

    mov    rax, r8         ; r8 = tgt_diagonal_ptr_holder$[rsp]
    add    rax, 512
    mov    bottom_of_diagonal_parity_sector$[rsp], rax

;    /* Find the address of the symbol we need to start at in the diagonal
;     * parity sector.
;     */
;    tgt_diagonal_ptr = (UINT_32E *)(tgt_diagonal_ptr_holder + column_index);

    mov    eax, r9d        ; r9d = column_index$[rsp]
    imul   rax, 32
    add    r8, rax         ; adjust r8 = tgt_diagonal_ptr

;    /* Find the address of the diagonal parity symbol that will not get the
;     * source data XORed into it.  There is one of these symbols for each 
;     * column except for column 0.
;     */
;    special_diagonal_parity_symbol = tgt_diagonal_ptr - FBE_XOR_WORDS_PER_SYMBOL;

    mov    rax, r8         ; tgt_diagonal_ptr$[rsp]
    sub    rax, 32
    mov    special_diagonal_parity_symbol$[rsp], rax

;
;    /* Find the component of the S value in this particular column.  There is
;     * no component of the S value contained in the first column. 
;     */
;    if (column_index == 0)

    cmp  r9d, 0            ; r9d = column_index$[rsp]
    jne  SHORT $LN2a

;    {
;        fbe_zero_memory(&s_value, sizeof(fbe_xor_r6_symbol_size_t));

     pxor   xmm1, xmm1
     pxor   xmm2, xmm2

;    }
;    else

    jmp     SHORT $LN1a
$LN2a:

;    {
;        /* Since this is not the first column find the S value component which
;         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
;         */
;        fbe_xor_r6_symbol_size_t *s_value_ptr =
;            ((fbe_xor_r6_symbol_size_t*)src_ptr) + (FBE_XOR_EVENODD_M_VALUE - column_index - 1);

    mov    eax, 17         ; FBE_XOR_EVENODD_M_VALUE
    sub    eax, r9d        ; - column_index$[rsp]
    dec    eax             ; -1
    imul   rax, 32         ; * sizeof(fbe_xor_r6_symbol_size_t)
    lea    rax, [rcx+rax]
    movdqu xmm1, [rax]     ; load 32-bytes at s_value_ptr
    movdqu xmm2, [rax+16]
$LN1a:

;    }

    ; Zero out the 128 bit checksum and init loop counter.
    pxor   xmm0, xmm0
    mov    eax, 16

;    /* For each data symbol do three things.
;     * 1) Update the checksum.                      (checksum ^= *src_ptr)
;     * 2) Set the correct row parity symbol.        (*tgt_row_ptr++ = *src_ptr)
;     * 3) Set the correct diagonal parity symbol.   (*tgt_diagonal_ptr++ = *src_ptr++ ^ s_value)
;     */
  do_loop_a:
    ; For each 128 bits of source data we need to: 
    ;  1) update the checksum;
    ;  2) copy the source into the row parity sector;
    ;  3) copy the source XORed with the S value into diagonal parity sector

    movdqu xmm3, [rcx]     ; load src
    movdqu xmm4, [rcx+16]  ; load second 128-bits now

    movdqu [rdx], xmm3     ; copy into row target
    pxor   xmm0, xmm3      ; running checksum
    pxor   xmm3, xmm1      ; xor in s value
    movdqu [r8], xmm3      ; copy into diagonal target

    movdqu [rdx+16], xmm4
    pxor   xmm0, xmm4
    pxor   xmm4, xmm2
    movdqu [r8+16], xmm4

    ; We have processed 32 bytes of source data so lets move on to the next.
    add    rcx, 32
    add    rdx, 32
    add    r8, 32 

    ; Decrement the loop counter.
    dec  eax
    pause
    jz   loop_done_a

    ; If we hit the bottom of the diagonal parity sector we are going to
    ; handle the next 32 bytes differently, fall into that section of code.
    cmp  r8, bottom_of_diagonal_parity_sector$[rsp]
    jnz  do_loop_a

    ; This block of code handles the one diagonal parity symbol that does
    ; not contain any of the source data, just the S value.  This block
    ; should be executed at most once for any one data sector.
    ;
    ; Start by going to the address of the special diagonal parity symbol.
    mov    r8, special_diagonal_parity_symbol$[rsp]

    ; For each 128 bits of source data we need to: 
    ;  1) update the checksum;
    ;  2) copy the source into the row parity sector;
    ;  3) copy the S value into the diagonal parity sector.
    movdqu xmm3, [rcx]     ; load src
    movdqu xmm4, [rcx+16]  ; load second 128 bits now so it will be ready when needed.

    pxor   xmm0, xmm3      ; running checksum
    movdqu [rdx], xmm3     ; copy into row target
    movdqu [r8], xmm1      ; copy into diagonal target

    pxor   xmm0, xmm4
    movdqu [rdx+16], xmm4
    movdqu [r8+16], xmm2

    ; We have processed 32 bytes of source data so lets move on to the next
    ; 32 bytes.  Move the diagonal parity pointer to the top of that sector.
    add    rcx, 32
    add    rdx, 32
    mov    r8, tgt_diagonal_ptr_holder$[rsp]

    ; Decrement the loop counter.  This is still part of the loop, just a 
    ; special case.
    dec    eax
    pause
    jnz    do_loop_a

  loop_done_a:
    ; We are done processing all 512 bytes of the source sector so
    ; fold up the checksum to return
    movdqa xmm5, xmm0      ; 128-bit checksum
    psrldq xmm0, 8         ; high 64-bits (8 bytes)
    pxor   xmm0, xmm5      ; xor hi/lo
    movdqa xmm5, xmm0      ; 64-bit checksum
    psrldq xmm0, 4         ; high 32-bits (4 bytes)
    pxor   xmm0, xmm5      ; xor hi/lo
    movd   eax, xmm0       ; ret 32-bit checksum 

    add    rsp, _locals_s$
    ret    0


;;
;; fbe_xor_calc_csum_and_update_r6_parity_mmx()
;;
;; DESCRIPTION:
;;  This function calculates a "raw" checksum (unseeded, unfolded) for
;;  data within a sector using the "classic" checksum algorithm; the
;;  parity values are updated based on the data.
;;
;; PARAMETERS:
;;  rcx  src_ptr, [I] - ptr to first word of source sector data
;;  rdx  tgt_row_ptr, [IO] - ptr to first word of target sector of row parity
;;  r8   tgt_diagonal_ptr, [IO] - ptr to first word of target sector of diagonal parity
;;  r9   column_index, [I] - The index of the disk passed in.
;;      
;; RETURNS:
;;  The "raw" checksum for the data in the sector.
;;
;; NOTES:
;;
;; HISTORY:
;;  05/24/06 - Created. RCL
;;  07/12/06 - SSE2 for Win64. GSchaffer
;;
;; UINT_32 fbe_xor_calc_csum_and_update_r6_parity_mmx(
;;     const UINT_32E * src_ptr,
;;     UINT_32E * tgt_row_ptr,
;;     UINT_32E * tgt_diagonal_ptr,
;;     UINT_32 column_index)
;; Registers:
;;  rcx  src_ptr
;;  rdx  tgt_row_ptr
;;  r8   tgt_diagonal_ptr
;;  r9d  column index
;; Uses:
;;  xmm0 128-bit checksum
;;  xmm1, xmm2 = 32-byte s_value
;;  xmm3, xmm4, xmm5 = loop processing
;; Returns:
;;  eax  32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_update_r6_parity_mmx
fbe_xor_calc_csum_and_update_r6_parity_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
    %define _locals_s$ 24 ; Stack space needed to save locals
%endif ;; __SAFE__ SAFEMESS
%ifndef __SAFE__
    ; save args on stack (debug)
    mov    [rsp+8],  rcx
    mov    [rsp+16], rdx
    mov    [rsp+24], r8
    mov    [rsp+32], r9d
    %define _locals_s$ 40 ; Stack space needed to save locals
%endif ;; __SAFE__ SAFEMESS  - no shadow space in non-MS calling convention

    sub    rsp, _locals_s$ ; Increment stack pointer to accomodate locals

    ; software prefetching, defeat hardware prefetch
    prefetchnta [rcx+320]
    prefetchnta [rcx]      ; src_ptr
    prefetchnta [rdx+320]
    prefetchnta [rdx]      ; tgt_row_ptr
    prefetchnta [r8+320]
    prefetchnta [r8]       ; tgt_diagonal_ptr
    prefetchnta [rcx+384]
    prefetchnta [rcx+64]
    prefetchnta [rdx+384]
    prefetchnta [rdx+64]
    prefetchnta [r8+384]
    prefetchnta [r8+64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+128]
    prefetchnta [rdx+448]
    prefetchnta [rdx+128]
    prefetchnta [r8+448]
    prefetchnta [r8+128]
    pause

;    UINT_32E * bottom_of_diagonal_parity_sector;
;    UINT_32E * special_diagonal_parity_symbol;
;    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

%define tgt_diagonal_ptr_holder$ 0
%define bottom_of_diagonal_parity_sector$ 8
%define special_diagonal_parity_symbol$ 16

;    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;

    mov    tgt_diagonal_ptr_holder$[rsp], r8

;    /* Find the address of the bottom of the diagonal parity sector so we
;     * don't go past it.
;     */
;    bottom_of_diagonal_parity_sector = (UINT_32E *)(tgt_diagonal_ptr_holder + 
;                            XOR_R6_SYMBOLS_PER_SECTOR);

    mov    rax, r8         ; r8 = tgt_diagonal_ptr_holder$[rsp]
    add    rax, 512
    mov    bottom_of_diagonal_parity_sector$[rsp], rax

;    /* Find the address of the symbol we need to start at in the diagonal
;     * parity sector.
;     */
;    tgt_diagonal_ptr = (UINT_32E *)(tgt_diagonal_ptr_holder + column_index);

    mov    eax, r9d        ; r9d = column_index$[rsp]
    imul rax, 32
    add    r8, rax         ; adjust r8 = tgt_diagonal_ptr

;    /* Find the address of the diagonal parity symbol that will not get the
;     * source data XORed into it.  There is one of these symbols for each 
;     * column except for column 0.
;     */
;    special_diagonal_parity_symbol = tgt_diagonal_ptr - FBE_XOR_WORDS_PER_SYMBOL;

    mov    rax, r8         ; r8 = tgt_diagonal_ptr$[rsp]
    sub    rax, 32
    mov    special_diagonal_parity_symbol$[rsp], rax

;
;    /* Find the component of the S value in this particular column.  There is
;     * no component of the S value contained in the first column. 
;     */
;    if (column_index == 0)

    cmp  r9d, 0            ; r9d = DWORD PTR column_index$[rsp]
    jne  SHORT $LN2b

;    {
;        fbe_zero_memory(&s_value, sizeof(fbe_xor_r6_symbol_size_t));

     pxor   xmm1, xmm1
     pxor   xmm2, xmm2

;    }
;    else

    jmp     SHORT $LN1b
$LN2b:

;    {
;        /* Since this is not the first column find the S value component which
;         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
;         */
;        fbe_xor_r6_symbol_size_t *s_value_ptr =
;            ((fbe_xor_r6_symbol_size_t*)src_ptr) + (FBE_XOR_EVENODD_M_VALUE - column_index - 1);

    mov    eax, 17         ; FBE_XOR_EVENODD_M_VALUE
    sub    eax, r9d        ; - column_index$[rsp]
    dec  eax               ; -1
    imul   rax, 32         ; * sizeof(fbe_xor_r6_symbol_size_t)
    lea  rax, [rcx+rax]
    movdqu xmm1, [rax]     ; load 32-bytes at s_value_ptr
    movdqu  xmm2, [rax+16]
$LN1b:

;    }

    ; Zero out the 128 bit checksum and init loop counter.
    pxor   xmm0, xmm0
    mov    eax, 16

;    /* For each data symbol do three things.
;     * 1) Update the checksum.                      (checksum ^= *src_ptr)
;     * 2) Set the correct row parity symbol.        (*tgt_row_ptr++ ^= *src_ptr)
;     * 3) Set the correct diagonal parity symbol.   (*tgt_diagonal_ptr++ ^= *src_ptr++ ^ s_value)
;     */
  do_loop_b:
    ; For each 128 bits of source data we need to: 
    ;  1) make a copy of the source;
    ;  2) update the checksum;
    ;  3) XOR the row parity into the source;
    ;  4) copy the source into the row parity sector;
    ;  5) XOR the S value into the source;
    ;  6) XOR the diagonal parity into the source;
    ;  7) copy the source into the diagonal parity sector.
    movdqu xmm3, [rcx]     ; load src
    movdqu xmm4, [rcx+16]  ; load second 128 bits now


    movdqu xmm5, xmm3      ; make a copy of the source
    pxor   xmm0, xmm3      ; running checksum
    movdqu xmm6, [rdx]     ; move row parity just in case it is unaligned
    pxor   xmm3, xmm6      ; xor the row parity into the source
    movdqu [rdx], xmm3     ; copy into row target
    pxor   xmm5, xmm1      ; xor in s value
    movdqu xmm7, [r8]      ; move diagonal parity just in case it is unaligned
    pxor   xmm5, xmm7      ; xor the diagonal parity into the source
    movdqu [r8], xmm5      ; copy into diagonal target

    movdqu xmm5, xmm4
    pxor   xmm0, xmm4
    movdqu xmm6, [rdx+16]  ; move row parity just incase it is unaligned
    pxor   xmm4, xmm6
    movdqu [rdx+16], xmm4
    pxor   xmm5, xmm2
    movdqu xmm7, [r8+16]   ; move diagonal parity just in case it is unaligned
    pxor   xmm5, xmm7
    movdqu [r8+16], xmm5

    ; We have processed 32 bytes of source data so lets move on to the next.
    add    rcx, 32
    add    rdx, 32
    add    r8, 32 

    ; Decrement the loop counter.
    dec  eax
    pause
    jz   loop_done_b

    ; If we hit the bottom of the diagonal parity sector we are going to
    ; handle the next 32 bytes differently, fall into that section of code.
    cmp  r8, bottom_of_diagonal_parity_sector$[rsp]
    jnz  do_loop_b

    ; This block of code handles the one diagonal parity symbol that does
    ; not contain any of the source data, just the S value.  This block
    ; should be executed at most once for any one data sector.
    ;
    ; Start by going to the address of the special diagonal parity symbol.

    mov   r8, special_diagonal_parity_symbol$[rsp]

    ; For each 128 bits of source data we need to: 
    ;  1) update the checksum;
    ;  2) XOR the row parity into the source;
    ;  3) copy the source into the row parity sector;
    ;  4) XOR the S value into the diagonal parity;
    ;  5) copy into the diagonal parity sector.

    movdqu xmm3, [rcx]     ; load src
    movdqu xmm4, [rcx+16]  ; load second 128 bits now so it will be ready when needed.

    movdqu xmm5, [r8]      ; get diagonal parity
    pxor   xmm0, xmm3      ; running checksum
    movdqu xmm6, [rdx]     ; move row parity  just in case it in unaligned  
    pxor   xmm3, xmm6      ; xor the row parity into the source
    movdqu [rdx], xmm3     ; copy into row target
    pxor   xmm5, xmm1      ; xor in s value to diagonal parity
    movdqu [r8], xmm5      ; copy into diagonal target

    movdqu xmm5, [r8+16]
    pxor   xmm0, xmm4
    movdqu xmm6, [rdx+16]  ; move row parity  just in case it in unaligned  
    pxor   xmm4, xmm6
    movdqu [rdx+16], xmm4
    pxor   xmm5, xmm2
    movdqu [r8+16], xmm5

    ; We have processed 32 bytes of source data so lets move on to the next
    ; 32 bytes.  Move the diagonal parity pointer to the top of that sector.
    add    rcx, 32
    add    rdx, 32
    mov    r8, tgt_diagonal_ptr_holder$[rsp]

    ; Decrement the loop counter.  This is still part of the loop, just a 
    ; special case.
    dec    eax
    pause
    jnz    do_loop_b

  loop_done_b:
    ; We are done processing all 512 bytes of the source sector so 
    ; fold up the checksum to return
    movdqa xmm5, xmm0      ; 128-bit checksum
    psrldq xmm0, 8         ; high 64-bits (8 bytes)
    pxor   xmm0, xmm5      ; xor hi/lo
    movdqa xmm5, xmm0      ; 64-bit checksum
    psrldq xmm0, 4         ; high 32-bits (4 bytes)
    pxor   xmm0, xmm5      ; xor hi/lo
    movd   eax, xmm0       ; ret 32-bit checksum 

    add    rsp, _locals_s$
    ret    0

%endif
END
