;; Copyright (C) EMC Corporation, 2006-2008
;; All rights reserved.
;; Licensed material -- property of EMC Corporation
;;
;; fbe_xor_csum_a64.asm - SSE2 XOR functions for Win64.
;; Use 128-bit SSE2 instructions instead of 64-bit MMX instructions.
;; Note intentional prefetch ahead sets up for next sector.
;; Adapted from MMX inline assembler fbe_xor_csum_util.c
;; Buffers are 8-byte aligned.
;; If 16-byte aligned, then XOR can be performed more efficiently.
;;
;; Calling conventions:
;;  rcx  1st argument
;;  rdx  2nd argument
;;  r8   3rd argument
;;  r9   4th argument
;;  eax  return 4-byte checksum
;;
;; Uses:
;;  eax  loop counter, 32-bit return value.
;;  xmm0 16-byte (128-bit) checksum
;;  xmm1 .. xmm4 src loop unrolling
;;  xmm5 .. xmm8 target loop unrolling (unaligned)
;;  xmm5 checksum folding
;;  xmm9 comparison checksum
;;
;; Assemble with ML64 (empty with ML)
;; Created GSchaffer 07-12-06
;; Performance Optimizations. Vamsi Vankamamidi 03-01-08 
;;
;; Prefetch and access pattern designed to defeat hardware prefetch
;; and avoid extra data being fetched into the L2 cache.
;; Align target to 16-byte for non-temporal store: movntdq
;; GSchaffer 03-17-08
;;
;; Optimized "to_temp_mmx" where temp parity will be read soon (16-byte align).
;; "fbe_xor_with_temp_and_cpy" non-temporal write of final parity.
;; "468" to xor old, new, parity in single pass.
;; Split misaligned into separate functions for VTune, _COUNTERS_
;; Vamsi Vankamamidi 04-04-08
;;
;; Moved alignment related logic and load instructions to execute before prefetch 
;; instructions for "aligned mmx routines" as it is shown that performance will be 
;; improved. Also, removed prefetchnta [reg + 512] instruction as its not required.   
;; Vamsi Vankamamidi 06-02-08
;;
;; Changed all non-temporal writes to temporal writes. This change is showing significant
;; improvement in performance
;; Vamsi Vankamamidi 04-17-2009
;;


%ifdef _AMD64_

%ifdef _COUNTERS_
SECTION .data
misaligned_csum_cnt     DQ  0
misaligned_xor_cnt      DQ  0
misaligned_cpy_cnt      DQ  0
misaligned_cmp_cnt      DQ  0
misaligned_xor_cpy_cnt  DQ  0
misaligned_xor_fbe_xor_cnt  DQ  0
%endif

SECTION .text

;;
;; UINT_32 fbe_mmx_cpuid(void)
;; returns: eax = CPUID bit 26: SSE2
;; NOTE: All ASE processors support MMX.
;; Only Snapper does not support SSE2 (Not 64-bits).
ALIGN 16
GLOBAL fbe_mmx_cpuid
fbe_mmx_cpuid PROC 
    push    rbx
    push    rcx
    push    rdx
    mov     eax, 1         ; feature flags
    cpuid
    xor     eax, eax;      ; eax = 0;
    test    edx, 04000000h ; bit 26: SSE2
    setnz   al             ; set boolean
    pop     rdx
    pop     rcx
    pop     rbx
    ret     0

;; mmx optimize sector (checksum ^= *srcptr++)
;; This is optimized to NOT use the HW prefetcher to 
;; avoid extra data being fetched into the L2 cache.
;; This routine is invoked if srcptr is neither 16
;; or 8 byte aligned.
;; UINT_32 fbe_xor_calc_csum_mmx(const UINT_32E *srcptr)
;;  rcx  srcptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_misaligned_mmx
fbe_xor_calc_csum_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_calc_csum_misaligned_mmx_aok
fbe_xor_calc_csum_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc [misaligned_csum_cnt]
%endif
    prefetchnta [rcx+192]
    prefetchnta [rcx+64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]
    pause
    pxor    xmm0, xmm0        ; zero the register        
    movdqu  xmm1, [rcx+192]   ; perform unaligned load
    pxor    xmm0, xmm1        ; accumulate parity
    movdqu  xmm1, [rcx+208]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+224]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+240]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+64]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+80]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+96]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+112]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+448]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+464]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+480]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+496]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+320]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+336]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+352]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+368]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+128]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+144]
    pxor    xmm0, xmm1      
    movdqu  xmm1, [rcx+160]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+176]
    pxor    xmm0, xmm1      
    movdqu  xmm1, [rcx+256]
    pxor    xmm0, xmm1      
    movdqu  xmm1, [rcx+272]
    pxor    xmm0, xmm1      
    movdqu  xmm1, [rcx+288]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+304]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+384]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+400]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx+416]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+432]
    pxor    xmm0, xmm1        
    movdqu  xmm1, [rcx]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+16]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+32]
    pxor    xmm0, xmm1       
    movdqu  xmm1, [rcx+48]
    pxor    xmm0, xmm1                
fold_a:
    movdqa  xmm5, xmm0   ; 128-bit checksum
    psrldq  xmm0, 8      ; high 64-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movdqa  xmm5, xmm0   ; 64-bit checksum
    psrldq  xmm0, 4      ; high 32-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movd    eax,  xmm0   ; ret 32-bit checksum 
    ret     0

;; mmx optimize sector (checksum ^= *srcptr++)
;; This is optimized to NOT use the HW prefetcher to 
;; avoid extra data being fetched into the L2 cache. 
;; UINT_32 fbe_xor_calc_csum_mmx(const UINT_32E *srcptr)
;; rcx  srcptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_mmx
fbe_xor_calc_csum_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    test    rcx,  7         ; srcptr 16 or 8-byte aligned?
    jnz     fbe_xor_calc_csum_misaligned_mmx_aok   
    test    rcx,  15        ; Is src ptr 16-byte aligned?
    jz      aligned16            
    movq    xmm0, qword ptr [rcx]     ; load first 8-bytes 
    movq    xmm1, qword ptr [rcx+504] ; load last 8-bytes 
    add     rcx, 8          ; now aligned
    jmp     cont_b
  aligned16:
    movdqa    xmm0, [rcx+496] ; xor last 16-bytes
    pxor      xmm1, xmm1    ; (0-bytes) 
  cont_b:
    prefetchnta [rcx+192]
    prefetchnta [rcx+64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]
    pause
    pxor    xmm0, [rcx+192] ; start checksum into two regs
    pxor    xmm1, [rcx+208]
    pxor    xmm0, [rcx+224] ; continue to accumulate checksum into two regs
    pxor    xmm1, [rcx+240]
    pxor    xmm0, [rcx+64]
    pxor    xmm1, [rcx+80]
    pxor    xmm0, [rcx+96]
    pxor    xmm1, [rcx+112]
    pxor    xmm0, [rcx+448]
    pxor    xmm1, [rcx+464]
    pxor    xmm0, [rcx+480]
    ;;pxor  xmm0, [rcx+496]   ; This case handled before prefetches 
    pxor    xmm1, [rcx+320]
    pxor    xmm0, [rcx+336]
    pxor    xmm1, [rcx+352]
    pxor    xmm0, [rcx+368]
    pxor    xmm1, [rcx+128]
    pxor    xmm0, [rcx+144]
    pxor    xmm1, [rcx+160]
    pxor    xmm0, [rcx+176]
    pxor    xmm1, [rcx+256]
    pxor    xmm0, [rcx+272]
    pxor    xmm1, [rcx+288]
    pxor    xmm0, [rcx+304]
    pxor    xmm1, [rcx+384]
    pxor    xmm0, [rcx+400]
    pxor    xmm1, [rcx+416]
    pxor    xmm0, [rcx+432]
    pxor    xmm1, [rcx]
    pxor    xmm0, [rcx+16]
    pxor    xmm1, [rcx+32]
    pxor    xmm0, [rcx+48]

  reduce:
    pxor    xmm0, xmm1      ; reduce accumulated checksum one reg    
  fold_b:
    movdqa  xmm5, xmm0      ; 128-bit checksum
    psrldq  xmm0, 8         ; high 64-bits
    pxor    xmm0, xmm5      ; xor hi/lo
    movdqa  xmm5, xmm0      ; 64-bit checksum
    psrldq  xmm0, 4         ; high 32-bits
    pxor    xmm0, xmm5      ; xor hi/lo
    movd    eax,  xmm0      ; ret 32-bit checksum 
    ret     0          

;; mmx optimize sector (checksum ^= *srcptr, *tgtptr++ = *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_cpy_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;; rcx  srcptr
;; rdx  tgtptr
;; returns: eax = 32-bit checksum
;; This routine is invoked if srcptr and trgptr are neither 16
;; or 8 byte aligned.
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cpy_misaligned_mmx
fbe_xor_calc_csum_and_cpy_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_calc_csum_and_cpy_misaligned_mmx_aok
fbe_xor_calc_csum_and_cpy_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc [misaligned_cpy_cnt]
%endif
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]    ; srcptr
    pause  
    ;; Block is processed in reverse order from last 64bytes to first 64bytes
    add     rcx, 448     ; load start address of last 64 bytes of source
    add     rdx, 448     ; load start address of last 64 bytes of target
    pxor    xmm0, xmm0      ; xmm0 checksum = 0
    mov     eax,  8         ; 8 loops of 64 bytes
    ;; unaligned loop 
un_loop_c:
    movdqu  xmm1, [rcx]     ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  [rdx],xmm1     ; copy target
    pxor    xmm0, xmm1      ; running checksum
    movdqu  [rdx+16], xmm2
    pxor    xmm0, xmm2
    movdqu  [rdx+32], xmm3
    pxor    xmm0, xmm3
    movdqu  [rdx+48], xmm4
    pxor    xmm0, xmm4
    ; while (--loop != 0)
    sub     rcx, 64   
    sub     rdx, 64
    dec     eax
    jnz     un_loop_c
fold_c:
    movdqa  xmm5, xmm0   ; 128-bit checksum
    psrldq  xmm0, 8      ; high 64-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movdqa  xmm5, xmm0   ; 64-bit checksum
    psrldq  xmm0, 4      ; high 32-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movd    eax, xmm0    ; ret 32-bit checksum 
    ret     0

;;
;; mmx optimize sector (checksum ^= *srcptr, *tgtptr++ = *srcptr++)
;; This function will copy a block in order.  This allows the source and
;; the destination to overlap.
;; UINT_32 fbe_xor_calc_csum_and_cpy_sequential_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cpy_sequential_mmx
fbe_xor_calc_csum_and_cpy_sequential_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    prefetchnta [rcx]    ; srcptr
    prefetchnta [rcx+128]
    pxor    xmm0, xmm0   ; xmm0 checksum = 0
    mov     eax, 8       ; 8 loops of 64 bytes

    test    rcx, 15      ; srcptr 16-byte aligned?
    jnz     un_loop_d
    test    rdx, 15      ; tgtptr 16-byte aligned?
    jnz     un_loop_d

  do_loop:
    prefetchnta [rcx+256]
    movdqa  xmm1, [rcx]  ; load src
    movdqa  xmm2, [rcx+16]
    movdqa  xmm3, [rcx+32]
    movdqa  xmm4, [rcx+48]
    movdqa [rdx], xmm1  ; copy target
    pxor    xmm0, xmm1   ; running checksum
    movdqa [rdx+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+48], xmm4
    pxor    xmm0, xmm4

    ; while (--loop != 0)
    add     rcx, 64
    add     rdx, 64
    dec     eax
    jnz     do_loop

  fold_d:
    movdqa  xmm5, xmm0   ; 128-bit checksum
    psrldq  xmm0, 8      ; high 64-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movdqa  xmm5, xmm0   ; 64-bit checksum
    psrldq  xmm0, 4      ; high 32-bits
    pxor    xmm0, xmm5   ; xor hi/lo
    movd    eax, xmm0    ; ret 32-bit checksum 
    ret     0

  ;; unaligned loop 
  un_loop_d:
    prefetchnta [rcx+256]
    movdqu  xmm1, [rcx]  ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  [rdx], xmm1  ; copy target
    pxor    xmm0, xmm1   ; running checksum
    movdqu  [rdx+16], xmm2
    pxor    xmm0, xmm2
    movdqu  [rdx+32], xmm3
    pxor    xmm0, xmm3
    movdqu  [rdx+48], xmm4
    pxor    xmm0, xmm4

    ; while (--loop != 0)
    add     rcx, 64
    add     rdx, 64
    dec     eax
    jnz     un_loop_d
    jmp     fold_d

;;
;; mmx optimize sector (checksum ^= *srcptr, *tgtptr++ = *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_cpy_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cpy_mmx
fbe_xor_calc_csum_and_cpy_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    
    test    rcx, 7              ; srcptr 16 or 8-byte aligned ?
    jnz     fbe_xor_calc_csum_and_cpy_misaligned_mmx_aok ; jump to misaligned routine
    test    rdx, 7              ; trgptr 16 or 8-byte aligned ?
    jnz     fbe_xor_calc_csum_and_cpy_misaligned_mmx_aok ; jump to misaligned routine
    
    ;; if unaligned tgtptr, last load is 8-bytes instead of 16.
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    test    rdx, 15             ; tgtptr 16-byte aligned?
    jz      aligned_tgt_e      
    mov     rax, [rcx]          ; load starting 8-bytes
    movq    xmm4, qword ptr [rcx]         ; checksum = (0 ^ 8-bytes)
    movnti  [rdx], rax          ; store tgt 8-bytes
    pxor    xmm0, xmm4          ; xor checksum
    mov     rax,  [rcx+504]     ; get 8-bytes at end
    movq    xmm4, qword ptr [rcx+504]     ; get 8-bytes at end
    movnti  [rdx+504], rax      ; copy to tgtptr
    pxor    xmm0, xmm4          ; xor checksum
    add     rcx, 8              ; update srcptr, and
    add     rdx, 8              ; tgtptr now aligned
    jmp     cont_e
    
  aligned_tgt_e:  
    movdqu  xmm4, [rcx+448+48]  ; load last 16-bytes
    movdqa [rdx+448+48], xmm4  ; store
    pxor    xmm0, xmm4          ; accumulate checksum
  cont_e:
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause   
    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src_e;
    movdqa  xmm1, [rcx+192]     ; load src
    movdqa  xmm2, [rcx+192+16]
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqa  xmm5, [rcx+ 64]
    movdqa  xmm6, [rcx+ 64+16]
    movdqa  xmm7, [rcx+ 64+32]
    movdqa  xmm8, [rcx+ 64+48]

    movdqa [rdx+192], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+192+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+192+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+192+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+ 64], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+ 64+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+ 64+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+ 64+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqa  xmm5, [rcx+320]     ; load src
    movdqa  xmm6, [rcx+320+16]
    movdqa  xmm7, [rcx+320+32]
    movdqa  xmm8, [rcx+320+48]

    movdqa [rdx+448], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+448+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+448+32], xmm3
    pxor    xmm0, xmm3
    ;;movdqa [rdx+448+48], xmm4  ;; This case handled before prefetches
    ;;pxor    xmm0, xmm4          ;; This case handled before prefetches
    movdqa [rdx+320], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+320+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+320+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+320+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqa  xmm5, [rcx+256]
    movdqa  xmm6, [rcx+256+16]
    movdqa  xmm7, [rcx+256+32]
    movdqa  xmm8, [rcx+256+48]

    movdqa [rdx+128], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+128+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+128+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+128+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+256], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+256+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+256+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+256+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqa  xmm5, [rcx]
    movdqa  xmm6, [rcx+16]
    movdqa  xmm7, [rcx+32]
    movdqa  xmm8, [rcx+48]

    movdqa [rdx+384], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+384+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+384+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+384+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+48], xmm8
    pxor    xmm0, xmm8
 
  fold_e:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src_e:                ; tgt has been aligned, src unaligned
    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rcx+ 64]
    movdqu  xmm6, [rcx+ 64+16]
    movdqu  xmm7, [rcx+ 64+32]
    movdqu  xmm8, [rcx+ 64+48]

    movdqa [rdx+192], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+192+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+192+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+192+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+ 64], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+ 64+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+ 64+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+ 64+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqu  xmm5, [rcx+320]     ; load src
    movdqu  xmm6, [rcx+320+16]
    movdqu  xmm7, [rcx+320+32]
    movdqu  xmm8, [rcx+320+48]
    
    movdqa [rdx+448], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+448+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+448+32], xmm3
    pxor    xmm0, xmm3
    ;;movdqa [rdx+448+48], xmm4  ;; This case handled before prefetches
    ;;pxor    xmm0, xmm4          ;; This case handled before prefetches
    movdqa [rdx+320], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+320+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+320+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+320+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rcx+256]
    movdqu  xmm6, [rcx+256+16]
    movdqu  xmm7, [rcx+256+32]
    movdqu  xmm8, [rcx+256+48]

    movdqa [rdx+128], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+128+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+128+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+128+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+256], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+256+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+256+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+256+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rcx]
    movdqu  xmm6, [rcx+16]
    movdqu  xmm7, [rcx+32]
    movdqu  xmm8, [rcx+48]

    movdqa [rdx+384], xmm1     ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+384+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+384+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+384+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+48], xmm8
    pxor    xmm0, xmm8
    jmp     fold_e
    

;; mmx optimize with prefetch sector (checksum ^= *srcptr, *tgtptr++ ^= *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_misaligned_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;; returns: eax = 32-bit checksum
;; This routine is invoked if srcptr and trgptr are neither 16
;; or 8 byte aligned.
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_misaligned_mmx
fbe_xor_calc_csum_and_xor_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_calc_csum_and_xor_misaligned_mmx_aok
fbe_xor_calc_csum_and_xor_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc [misaligned_xor_cnt]
%endif
    prefetchnta [rcx+192]
    prefetchnta [rdx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rdx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rdx+448]
    prefetchnta [rcx+320]
    prefetchnta [rdx+320]
    prefetchnta [rcx+128]
    prefetchnta [rdx+128]
    prefetchnta [rcx+256]
    prefetchnta [rdx+256]
    prefetchnta [rcx+384]
    prefetchnta [rdx+384]
    prefetchnta [rcx]           ; srcptr
    prefetchnta [rdx]           ; tgtptr
    pause
    ;; Block is processed in reverse order from last 64bytes to first 64bytes
    add     rcx, 448            ; load start address of last 64 bytes of source
    add     rdx, 448            ; load start address of last 64 bytes of target
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    mov     eax,  8             ; 8 loops of 64 bytes
  ;; unaligned loop 
  un_loop_f:
    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  xmm5, [rdx]         ; load target
    movdqu  xmm6, [rdx+16]
    movdqu  xmm7, [rdx+32]
    movdqu  xmm8, [rdx+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqu  [rdx], xmm1         ; store xor target
    movdqu  [rdx+16], xmm2
    movdqu  [rdx+32], xmm3
    movdqu  [rdx+48], xmm4

    ; while (--loop != 0)
    sub     rcx, 64
    sub     rdx, 64
    dec     eax
    jnz     un_loop_f
fold_f:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

;; mmx optimize with prefetch sector (checksum ^= *srcptr, *tgtptr++ ^= *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_mmx
fbe_xor_calc_csum_and_xor_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    test    rcx, 7              ; srcptr is 16 or 8-byte aligned? 
    jnz     fbe_xor_calc_csum_and_xor_misaligned_mmx_aok ; jump to misaligned routine
    test    rdx, 7              ; trgptr is 16 or 8-byte aligned?
    jnz     fbe_xor_calc_csum_and_xor_misaligned_mmx_aok ; jump to misaligned routine     

    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    test    rdx, 15             ; tgtptr 16-byte aligned?
    jz      aligned_tgt_g
    mov     rax,  [rcx]         ; load starting 8-bytes of src
    movq    xmm4, qword ptr [rcx]         ; load starting 8-bytes of src
    pxor    xmm0, xmm4          ; running src checksum
    xor     rax,  [rdx]         ; xor starting 8-bytes of trg in rax
    movnti  [rdx], rax          ; store xor target
    mov     rax,  [rcx+504]     ; load last 8-bytes of src
    movq    xmm4, qword ptr [rcx+504]     ; load last 8-bytes of src
    pxor    xmm0, xmm4          ; running src checksum
    xor     rax,  [rdx+504]     ; xor last 8-bytes of trg in rax
    movnti  [rdx+504], rax      ; store xor target
    add     rcx, 8              ; update srcptr, and
    add     rdx, 8              ; tgtptr now aligned.
    jmp     cont_g
    
  aligned_tgt_g:
    movdqu  xmm4, [rcx+448+48]  ; load last 16-bytes of src
    pxor    xmm0, xmm4          ; xor source checksum
    pxor    xmm4, [rdx+448+48]  ; xor target
    movdqa [rdx+448+48], xmm4  ; store xor target
  cont_g:
    prefetchnta [rcx+192]
    prefetchnta [rdx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rdx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rdx+448]
    prefetchnta [rcx+320]
    prefetchnta [rdx+320]
    prefetchnta [rcx+128]
    prefetchnta [rdx+128]
    prefetchnta [rcx+256]
    prefetchnta [rdx+256]
    prefetchnta [rcx+384]
    prefetchnta [rdx+384]
    prefetchnta [rcx]           ; srcptr
    prefetchnta [rdx]           ; tgtptr
    pause
    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src_g

    movdqa  xmm1, [rcx+192]     ; load src
    movdqa  xmm2, [rcx+192+16]
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqa  xmm5, [rcx+ 64]
    movdqa  xmm6, [rcx+ 64+16]
    movdqa  xmm7, [rcx+ 64+32]
    movdqa  xmm8, [rcx+ 64+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    movdqa [rdx+192],    xmm1  ; store xor target
    movdqa [rdx+192+16], xmm2
    movdqa [rdx+192+32], xmm3
    movdqa [rdx+192+48], xmm4
    movdqa [rdx+ 64],    xmm5
    movdqa [rdx+ 64+16], xmm6
    movdqa [rdx+ 64+32], xmm7
    movdqa [rdx+ 64+48], xmm8

    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqa  xmm5, [rcx+320]     ; load src
    movdqa  xmm6, [rcx+320+16]
    movdqa  xmm7, [rcx+320+32]
    movdqa  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4          ;; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]  ;; This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    movdqa [rdx+448],    xmm1  ; store xor target
    movdqa [rdx+448+16], xmm2
    movdqa [rdx+448+32], xmm3
    ;; movdqa [rdx+448+48], xmm4 - done before prefetches
    movdqa [rdx+320],    xmm5
    movdqa [rdx+320+16], xmm6
    movdqa [rdx+320+32], xmm7
    movdqa [rdx+320+48], xmm8

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqa  xmm5, [rcx+256]
    movdqa  xmm6, [rcx+256+16]
    movdqa  xmm7, [rcx+256+32]
    movdqa  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    movdqa [rdx+128], xmm1     ; store xor target
    movdqa [rdx+128+16], xmm2
    movdqa [rdx+128+32], xmm3
    movdqa [rdx+128+48], xmm4
    movdqa [rdx+256],    xmm5
    movdqa [rdx+256+16], xmm6
    movdqa [rdx+256+32], xmm7
    movdqa [rdx+256+48], xmm8

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqa  xmm5, [rcx]
    movdqa  xmm6, [rcx+16]
    movdqa  xmm7, [rcx+32]
    movdqa  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    movdqa [rdx+384], xmm1     ; store xor target
    movdqa [rdx+384+16], xmm2
    movdqa [rdx+384+32], xmm3
    movdqa [rdx+384+48], xmm4
    movdqa [rdx],    xmm5
    movdqa [rdx+16], xmm6
    movdqa [rdx+32], xmm7
    movdqa [rdx+48], xmm8
  
  fold_g:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src_g:                ; tgtptr is aligned
    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rcx+ 64]
    movdqu  xmm6, [rcx+ 64+16]
    movdqu  xmm7, [rcx+ 64+32]
    movdqu  xmm8, [rcx+ 64+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    movdqa [rdx+192],    xmm1  ; store xor target
    movdqa [rdx+192+16], xmm2
    movdqa [rdx+192+32], xmm3
    movdqa [rdx+192+48], xmm4
    movdqa [rdx+ 64],    xmm5
    movdqa [rdx+ 64+16], xmm6
    movdqa [rdx+ 64+32], xmm7
    movdqa [rdx+ 64+48], xmm8

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqu  xmm5, [rcx+320]     ; load src
    movdqu  xmm6, [rcx+320+16]
    movdqu  xmm7, [rcx+320+32]
    movdqu  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4          ;; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]  ;; This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    movdqa [rdx+448],    xmm1  ; store xor target
    movdqa [rdx+448+16], xmm2
    movdqa [rdx+448+32], xmm3
    ;;movdqu  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqa [rdx+320],    xmm5
    movdqa [rdx+320+16], xmm6
    movdqa [rdx+320+32], xmm7
    movdqa [rdx+320+48], xmm8

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rcx+256]
    movdqu  xmm6, [rcx+256+16]
    movdqu  xmm7, [rcx+256+32]
    movdqu  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    movdqa [rdx+128], xmm1     ; store xor target
    movdqa [rdx+128+16], xmm2
    movdqa [rdx+128+32], xmm3
    movdqa [rdx+128+48], xmm4
    movdqa [rdx+256],    xmm5
    movdqa [rdx+256+16], xmm6
    movdqa [rdx+256+32], xmm7
    movdqa [rdx+256+48], xmm8

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rcx]
    movdqu  xmm6, [rcx+16]
    movdqu  xmm7, [rcx+32]
    movdqu  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    movdqa [rdx+384], xmm1     ; store xor target
    movdqa [rdx+384+16], xmm2
    movdqa [rdx+384+32], xmm3
    movdqa [rdx+384+48], xmm4
    movdqa [rdx],    xmm5
    movdqa [rdx+16], xmm6
    movdqa [rdx+32], xmm7
    movdqa [rdx+48], xmm8
    jmp     fold_g
    

;; mmx optimize sector (checksum ^= *srcptr, 
;;                      *tgtptr2++ = *srcptr, 
;;                      *tgtptr1++ = *srcptr++)
;;
;; UINT_32 fbe_xor_calc_csum_and_cpy_vault_mmx(const UINT_32E *srcptr, 
;;                                         UINT_32E *tgtptr1, 
;;                                         UINT_32E *tgtptr2)
;;  rcx  srcptr
;;  rdx  tgtptr1
;;  r8   tgtptr2
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cpy_vault_mmx
fbe_xor_calc_csum_and_cpy_vault_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    prefetchnta [rcx+192]
    prefetchnta [rcx+64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]
    pause

    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    mov     eax, 8              ; 8 loops of 64 bytes

  un_loop_h:
    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  [rdx], xmm1         ; copy target1
    movdqu  [r8], xmm1          ; copy target2
    pxor    xmm0, xmm1          ; running checksum
    movdqu  [rdx+16], xmm2
    movdqu  [r8 +16], xmm2
    pxor    xmm0, xmm2
    movdqu  [rdx+32], xmm3
    movdqu  [r8 +32], xmm3
    pxor    xmm0, xmm3
    movdqu  [rdx+48], xmm4
    movdqu  [r8 +48], xmm4
    pxor    xmm0, xmm4

    ; while (--loop != 0)
    add     rcx, 64
    add     rdx, 64
    add     r8,  64
    dec     eax
    jnz     un_loop_h

  fold_h:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0



;; mmx optimize with prefetch sector 
;;      (checksum ^= *srcptr, 
;;       *tgtptr1++ = *srcptr, 
;;       *tgtptr2++ ^= *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_vault_mmx(const UINT_32E *srcptr, 
;;                                         UINT_32E *tgtptr1, 
;;                                         UINT_32E *tgtptr2)
;;  rcx  srcptr
;;  rdx  tgtptr1
;;  r8   tgtptr2
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_vault_mmx
fbe_xor_calc_csum_and_xor_vault_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    prefetchnta [rcx+192]
    prefetchnta [r8 +192]
    prefetchnta [rcx+ 64]
    prefetchnta [r8 + 64]
    prefetchnta [rcx+448]
    prefetchnta [r8 +448]
    prefetchnta [rcx+320]
    prefetchnta [r8 +320]
    prefetchnta [rcx+128]
    prefetchnta [r8 +128]
    prefetchnta [rcx+256]
    prefetchnta [r8 +256]
    prefetchnta [rcx+384]
    prefetchnta [r8 +384]
    prefetchnta [rcx]           ; srcptr
    prefetchnta [r8]            ; tgtptr
    pause

    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    mov     eax, 8              ; 8 loops of 64 bytes

  un_loop_i:
    movdqu  xmm1, [rcx]         ; load srcptr
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  xmm5, [r8]          ; load tgtptr2
    movdqu  xmm6, [r8+16]
    movdqu  xmm7, [r8+32]
    movdqu  xmm8, [r8+48]

    pxor    xmm0, xmm1          ; running src checksum
    movdqu  [rdx], xmm1         ; copy *tgtptr1
    pxor    xmm1, xmm5          ; xor  *tgtptr2
    pxor    xmm0, xmm2
    movdqu  [rdx+16], xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    movdqu  [rdx+32], xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    movdqu  [rdx+48], xmm4
    pxor    xmm4, xmm8
    movdqu  [r8], xmm1          ; store xor target
    movdqu  [r8+16], xmm2
    movdqu  [r8+32], xmm3
    movdqu  [r8+48], xmm4

    ; while (--loop != 0)
    add     rcx, 64
    add     rdx, 64
    add     r8,  64
    dec     eax
    jnz     un_loop_i

  fold_i:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0


;; The below assembly corresponds to the following C code.
;; for all words in sector {
;;  checksum ^= *srcptr, comparison |= (*tgtptr++ ^ *srcptr++);
;; }
;; *cmpptr = ((comparison == 0) ? XOR_CSUM_SAME_DATA : XOR_CSUM_DIFF_DATA);
;;
;; UINT_32 fbe_xor_calc_csum_and_cmp_mmx(const UINT_32E * srcptr,
;;                        const UINT_32E * tgtptr,
;;                        XOR_CSUM_CMP * cmpptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;;  r8   cmpptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cmp_misaligned_mmx
fbe_xor_calc_csum_and_cmp_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_calc_csum_and_cmp_misaligned_mmx_aok
fbe_xor_calc_csum_and_cmp_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc     [misaligned_cmp_cnt]
%endif
    prefetchnta [rcx+192]
    prefetchnta [rdx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rdx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rdx+448]
    prefetchnta [rcx+320]
    prefetchnta [rdx+320]
    prefetchnta [rcx+128]
    prefetchnta [rdx+128]
    prefetchnta [rcx+256]
    prefetchnta [rdx+256]
    prefetchnta [rcx+384]
    prefetchnta [rdx+384]
    prefetchnta [rcx]           ; srcptr
    prefetchnta [rdx]           ; tgtptr
    pause
    ;; Block is processed in reverse order from last 64bytes to first 64bytes
    add     rcx, 448            ; load start address of last 64 bytes of source
    add     rdx, 448            ; load start address of last 64 bytes of target
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    pxor    xmm9, xmm9          ; xmm9 comparison = 0
    mov     eax, 8              ; 8 loops of 64 bytes

  ;; unaligned loop 
  un_loop_j:
    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  xmm5, [rdx]         ; load target
    movdqu  xmm6, [rdx+16]
    movdqu  xmm7, [rdx+32]
    movdqu  xmm8, [rdx+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor target (src ^ tgt)
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4

    ; while (--loop != 0)
    sub     rcx, 64
    sub     rdx, 64
    dec     eax
    jnz     un_loop_j

  compare_j:
    movdqa  xmm5, xmm9          ; fold 128-bit comparison
    psrldq  xmm9, 8             ; high 64-bits
    por     xmm9, xmm5          ; or hi/lo
    movdqa  xmm5, xmm9          ; 64-bit comparison
    psrldq  xmm9, 4             ; high 32-bits
    por     xmm9, xmm5          ; or hi/lo
    movd    eax, xmm9           ; 32-bit comparison
    mov     ecx, 1              ; 1 = XOR_CSUM_DIFF_DATA
    test    eax, eax            ; different?
    jnz     store_j               ;
    xor     ecx, ecx            ; 0 = XOR_CSUM_SAME_DATA
  store_j:
    mov     [r8], ecx           ; *cmpptr = SAME or DIFF

  fold_j:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0


;; The below assembly corresponds to the following C code.
;; for all words in sector {
;;  checksum ^= *srcptr, comparison |= (*tgtptr++ ^ *srcptr++);
;; }
;; *cmpptr = ((comparison == 0) ? XOR_CSUM_SAME_DATA : XOR_CSUM_DIFF_DATA);
;;
;; UINT_32 fbe_xor_calc_csum_and_cmp_mmx(const UINT_32E * srcptr,
;;                        const UINT_32E * tgtptr,
;;                        XOR_CSUM_CMP * cmpptr)
;;  rcx  srcptr
;;  rdx  tgtptr
;;  r8   cmpptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cmp_mmx
fbe_xor_calc_csum_and_cmp_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
   
    test    rcx, 7              ; srcptr is 16 or 8-byte aligned? 
    jnz     fbe_xor_calc_csum_and_cmp_misaligned_mmx_aok ; jump to misaligned routine
    test    rdx, 7              ; trgptr is 16 or 8-byte aligned?
    jnz     fbe_xor_calc_csum_and_cmp_misaligned_mmx_aok ; jump to misaligned routine
    
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    pxor    xmm9, xmm9          ; xmm9 comparison = 0
    test    rdx, 15             ; tgtptr 16-byte aligned?
    jz      aligned_tgt_k
    movq    xmm1, qword ptr [rcx]         ; load src 8-bytes
    pxor    xmm0, xmm1          ; running src checksum
    movq    xmm2, qword ptr [rdx]         ; load tgt 8-bytes
    pxor    xmm1, xmm2          ; xor target
    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    movq    xmm4, qword ptr [rcx+504]     ; get last 8-bytes at end
    pxor    xmm0, xmm4          ; xor source checksum
    movq    xmm10, qword ptr [rdx+504]     ; load target 8-bytes
    pxor    xmm4, xmm10         ; xor target
    por     xmm9, xmm4          ; cumulate xors comparison |= (src ^ tgt)
    add     rcx, 8              ; update srcptr, and
    add     rdx, 8              ; tgtptr now aligned.
    jmp     cont_k
  aligned_tgt_k:
    movdqu  xmm4, [rcx+448+48]  ; load last 16-bytes
    pxor    xmm0, xmm4          ; xor source checksum
    pxor    xmm4, [rdx+448+48]  ; xor target
    por     xmm9, xmm4          ; cumulate xors comparison |= (src ^ tgt)
  cont_k:
    prefetchnta [rcx+192]
    prefetchnta [rdx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rdx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rdx+448]
    prefetchnta [rcx+320]
    prefetchnta [rdx+320]
    prefetchnta [rcx+128]
    prefetchnta [rdx+128]
    prefetchnta [rcx+256]
    prefetchnta [rdx+256]
    prefetchnta [rcx+384]
    prefetchnta [rdx+384]
    prefetchnta [rcx]           ; srcptr
    prefetchnta [rdx]           ; tgtptr
    pause
    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src_k       
    
    movdqa  xmm1, [rcx+192]     ; load src
    movdqa  xmm2, [rcx+192+16]
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqa  xmm5, [rcx+ 64]
    movdqa  xmm6, [rcx+ 64+16]
    movdqa  xmm7, [rcx+ 64+32]
    movdqa  xmm8, [rcx+ 64+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor target (src ^ tgt)
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqa  xmm1, [rcx+448]  ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqa  xmm5, [rcx+320]  ; load src
    movdqa  xmm6, [rcx+320+16]
    movdqa  xmm7, [rcx+320+32]
    movdqa  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1       ; running src checksum
    pxor    xmm1, [rdx+448]  ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4          ;;This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]  ;;This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    ;;por     xmm9, xmm4 ;;This case handled before prefetches
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqa  xmm5, [rcx+256]
    movdqa  xmm6, [rcx+256+16]
    movdqa  xmm7, [rcx+256+32]
    movdqa  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqa  xmm5, [rcx]
    movdqa  xmm6, [rcx+16]
    movdqa  xmm7, [rcx+32]
    movdqa  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

  compare_k:
    movdqa  xmm5, xmm9          ; fold 128-bit comparison
    psrldq  xmm9, 8             ; high 64-bits
    por     xmm9, xmm5          ; or hi/lo
    movdqa  xmm5, xmm9          ; 64-bit comparison
    psrldq  xmm9, 4             ; high 32-bits
    por     xmm9, xmm5          ; or hi/lo
    movd    eax, xmm9           ; 32-bit comparison
    mov     ecx, 1              ; 1 = XOR_CSUM_DIFF_DATA
    test    eax, eax            ; different?
    jnz     store_k               ;
    xor     ecx, ecx            ; 0 = XOR_CSUM_SAME_DATA
  store_k:
    mov     [r8], ecx           ; *cmpptr = SAME or DIFF

  fold_k:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src_k:                ; tgtptr is aligned
    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rcx+ 64]
    movdqu  xmm6, [rcx+ 64+16]
    movdqu  xmm7, [rcx+ 64+32]
    movdqu  xmm8, [rcx+ 64+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor target (src ^ tgt)
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqu  xmm5, [rcx+320]     ; load src
    movdqu  xmm6, [rcx+320+16]
    movdqu  xmm7, [rcx+320+32]
    movdqu  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4          ;;This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]  ;;This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    ;;por     xmm9, xmm4 ;;This case handled before prefetches
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rcx+256]
    movdqu  xmm6, [rcx+256+16]
    movdqu  xmm7, [rcx+256+32]
    movdqu  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rcx]
    movdqu  xmm6, [rcx+16]
    movdqu  xmm7, [rcx+32]
    movdqu  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor target
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    por     xmm9, xmm1          ; cumulate xors comparison |= (src ^ tgt)
    por     xmm9, xmm2
    por     xmm9, xmm3
    por     xmm9, xmm4
    por     xmm9, xmm5
    por     xmm9, xmm6
    por     xmm9, xmm7
    por     xmm9, xmm8
    jmp     compare_k

;; Following routine is created to optimize MR3 write operation.
;; temp buffer is always 16-byte aligned and is maintained in L2 cache
;; mmx optimize sector (checksum ^= *srcptr, *tempptr++ = *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_cpy_to_temp_mmx(const UINT_32E *srcptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tempptr
;; returns: eax = 32-bit checksum
;;
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_cpy_to_temp_mmx
fbe_xor_calc_csum_and_cpy_to_temp_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    test    rcx, 7              ; srcptr 8 or 16-byte aligned?
    jnz     fbe_xor_calc_csum_and_cpy_misaligned_mmx_aok ;jump to misaligned routine
    ;; Below instructions are executed before prefetches to improve performance of prefetches
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    movdqu  xmm1, [rcx+448+48]  ; load src
    movdqa  [rdx+448+48], xmm1  ; copy target
    pxor    xmm0, xmm1          ; running checksum
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause

    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src_l;
    movdqa  xmm1, [rcx+192]  
    movdqa  xmm2, [rcx+192+16]  ; load src
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqa  xmm5, [rcx+ 64]
    movdqa  xmm6, [rcx+ 64+16]
    movdqa  xmm7, [rcx+ 64+32]
    movdqa  xmm8, [rcx+ 64+48]

    movdqa [rdx+192], xmm1  
    pxor    xmm0, xmm1   
    movdqa [rdx+192+16], xmm2   ; copy target
    pxor    xmm0, xmm2          ; running checksum
    movdqa [rdx+192+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+192+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+ 64], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+ 64+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+ 64+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+ 64+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48]  ; This case handled before prefetches
    movdqa  xmm5, [rcx+320]     ; load src
    movdqa  xmm6, [rcx+320+16]
    movdqa  xmm7, [rcx+320+32]
    movdqa  xmm8, [rcx+320+48]

    movdqa [rdx+448], xmm1      ; copy target
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+448+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+448+32], xmm3
    pxor    xmm0, xmm3
    ;;movdqa [rdx+448+48], xmm4 ; This case handled before prefetches
    ;;pxor    xmm0, xmm4        ; This case handled before prefetches
    movdqa [rdx+320], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+320+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+320+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+320+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqa  xmm5, [rcx+256]
    movdqa  xmm6, [rcx+256+16]
    movdqa  xmm7, [rcx+256+32]
    movdqa  xmm8, [rcx+256+48]

    movdqa [rdx+128], xmm1      ; copy to temp
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+128+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+128+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+128+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+256], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+256+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+256+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+256+48], xmm8
    pxor    xmm0, xmm8

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqa  xmm5, [rcx]
    movdqa  xmm6, [rcx+16]
    movdqa  xmm7, [rcx+32]
    movdqa  xmm8, [rcx+48]

    movdqa [rdx+384], xmm1      ; copy to temp
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+384+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+384+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+384+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+48], xmm8
    pxor    xmm0, xmm8

  fold_l:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src_l:                ; src is unaligned
    movdqu  xmm1, [rcx+192]
    movdqu  xmm2, [rcx+192+16]  ; load src
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rcx+ 64]
    movdqu  xmm6, [rcx+ 64+16]
    movdqu  xmm7, [rcx+ 64+32]
    movdqu  xmm8, [rcx+ 64+48]

    movdqa [rdx+192], xmm1  
    pxor    xmm0, xmm1
    movdqa [rdx+192+16], xmm2   ; copy to temp
    pxor    xmm0, xmm2          ; running checksum
    movdqa [rdx+192+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+192+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+ 64], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+ 64+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+ 64+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+ 64+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48]  ; This case handled before prefetches
    movdqu  xmm5, [rcx+320]     ; load src
    movdqu  xmm6, [rcx+320+16]
    movdqu  xmm7, [rcx+320+32]
    movdqu  xmm8, [rcx+320+48]

    movdqa [rdx+448], xmm1      ; copy to temp
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+448+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+448+32], xmm3
    pxor    xmm0, xmm3
    ;;movdqa [rdx+448+48], xmm4   ; This case handled before prefetches
    ;;pxor    xmm0, xmm4          ; This case handled before prefetches
    movdqa [rdx+320], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+320+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+320+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+320+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rcx+256]
    movdqu  xmm6, [rcx+256+16]
    movdqu  xmm7, [rcx+256+32]
    movdqu  xmm8, [rcx+256+48]

    movdqa [rdx+128], xmm1      ; copy to temp
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+128+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+128+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+128+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx+256], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+256+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+256+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+256+48], xmm8
    pxor    xmm0, xmm8

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rcx]
    movdqu  xmm6, [rcx+16]
    movdqu  xmm7, [rcx+32]
    movdqu  xmm8, [rcx+48]

    movdqa [rdx+384], xmm1      ; copy to temp
    pxor    xmm0, xmm1          ; running checksum
    movdqa [rdx+384+16], xmm2
    pxor    xmm0, xmm2
    movdqa [rdx+384+32], xmm3
    pxor    xmm0, xmm3
    movdqa [rdx+384+48], xmm4
    pxor    xmm0, xmm4
    movdqa [rdx], xmm5
    pxor    xmm0, xmm5
    movdqa [rdx+16], xmm6
    pxor    xmm0, xmm6
    movdqa [rdx+32], xmm7
    pxor    xmm0, xmm7
    movdqa [rdx+48], xmm8
    pxor    xmm0, xmm8

    jmp     fold_l

;; Following routine is created to optimize MR3 write operation.
;; temp buffer is always 16-byte aligned and is maintained in L2 cache
;; mmx optimize with prefetch sector (checksum ^= *srcptr, *tempptr++ ^= *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_to_temp_mmx(const UINT_32E *srcptr, UINT_32E *tempptr)
;;  rcx  srcptr
;;  rdx  tempptr
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_to_temp_mmx
fbe_xor_calc_csum_and_xor_to_temp_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    test    rcx, 7              ; srcptr 8 or 16-byte aligned?
    jnz     fbe_xor_calc_csum_and_xor_misaligned_mmx_aok ; jump to misaligned routine
    ;; Below instructions are executed before prefetches to improve performance of prefetches 
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    movdqu  xmm1, [rcx+448+48]  ; load src
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448+48]  ; xor temp buffer
    movdqa  [rdx+448+48], xmm1  ; store xor to temp
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause

    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src_m;

    movdqa  xmm1, [rcx+192]
    movdqa  xmm2, [rcx+192+16]  ; load src
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqa  xmm5, [rcx+ 64]
    movdqa  xmm6, [rcx+ 64+16]
    movdqa  xmm7, [rcx+ 64+32]
    movdqa  xmm8, [rcx+ 64+48]

    pxor    xmm0, xmm1               
    pxor    xmm1, [rdx+192]
    pxor    xmm0, xmm2          ; running src checksum
    pxor    xmm2, [rdx+192+16]  ; xor temp buffer 
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    movdqa [rdx+192], xmm1
    movdqa [rdx+192+16], xmm2   ; store xor to temp
    movdqa [rdx+192+32], xmm3
    movdqa [rdx+192+48], xmm4
    movdqa [rdx+ 64],    xmm5
    movdqa [rdx+ 64+16], xmm6
    movdqa [rdx+ 64+32], xmm7
    movdqa [rdx+ 64+48], xmm8

    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48]  ; This case handled before prefetches
    movdqa  xmm5, [rcx+320]     ; load src
    movdqa  xmm6, [rcx+320+16]
    movdqa  xmm7, [rcx+320+32]
    movdqa  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4         ; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48] ; This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    movdqa [rdx+448],    xmm1   ; store xor temp
    movdqa [rdx+448+16], xmm2
    movdqa [rdx+448+32], xmm3
    ;; movdqa [rdx+448+48], xmm4  ; This case handled before prefetches
    movdqa [rdx+320],    xmm5
    movdqa [rdx+320+16], xmm6
    movdqa [rdx+320+32], xmm7
    movdqa [rdx+320+48], xmm8

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqa  xmm5, [rcx+256]
    movdqa  xmm6, [rcx+256+16]
    movdqa  xmm7, [rcx+256+32]
    movdqa  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    movdqa [rdx+128], xmm1      ; store xor temp
    movdqa [rdx+128+16], xmm2
    movdqa [rdx+128+32], xmm3
    movdqa [rdx+128+48], xmm4
    movdqa [rdx+256],    xmm5
    movdqa [rdx+256+16], xmm6
    movdqa [rdx+256+32], xmm7
    movdqa [rdx+256+48], xmm8

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqa  xmm5, [rcx]
    movdqa  xmm6, [rcx+16]
    movdqa  xmm7, [rcx+32]
    movdqa  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    movdqa [rdx+384], xmm1      ; store xor temp
    movdqa [rdx+384+16], xmm2
    movdqa [rdx+384+32], xmm3
    movdqa [rdx+384+48], xmm4
    movdqa [rdx],    xmm5
    movdqa [rdx+16], xmm6
    movdqa [rdx+32], xmm7
    movdqa [rdx+48], xmm8

  fold_m:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src_m:                ; tgtptr is aligned
    movdqu  xmm1, [rcx+192]
    movdqu  xmm2, [rcx+192+16]  ; load src
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rcx+ 64]
    movdqu  xmm6, [rcx+ 64+16]
    movdqu  xmm7, [rcx+ 64+32]
    movdqu  xmm8, [rcx+ 64+48]

    pxor    xmm0, xmm1      
    pxor    xmm1, [rdx+192] 
    pxor    xmm0, xmm2          ; running src checksum 
    pxor    xmm2, [rdx+192+16]  ; xor temp
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+ 64]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+ 64+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+ 64+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+ 64+48]

    movdqa [rdx+192],    xmm1  
    movdqa [rdx+192+16], xmm2    ; store xor temp
    movdqa [rdx+192+32], xmm3
    movdqa [rdx+192+48], xmm4
    movdqa [rdx+ 64],    xmm5
    movdqa [rdx+ 64+16], xmm6
    movdqa [rdx+ 64+32], xmm7
    movdqa [rdx+ 64+48], xmm8

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48] ; This case handled before prefetches
    movdqu  xmm5, [rcx+320]     ; load src
    movdqu  xmm6, [rcx+320+16]
    movdqu  xmm7, [rcx+320+32]
    movdqu  xmm8, [rcx+320+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4         ; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48] ; This case handled before prefetches
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+320]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+320+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+320+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+320+48]

    movdqa [rdx+448],    xmm1   ; store xor temp
    movdqa [rdx+448+16], xmm2
    movdqa [rdx+448+32], xmm3
    ;;movdqa [rdx+448+48], xmm4 ; This case handled before prefetches
    movdqa [rdx+320],    xmm5
    movdqa [rdx+320+16], xmm6
    movdqa [rdx+320+32], xmm7
    movdqa [rdx+320+48], xmm8

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rcx+256]
    movdqu  xmm6, [rcx+256+16]
    movdqu  xmm7, [rcx+256+32]
    movdqu  xmm8, [rcx+256+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx+256]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+256+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+256+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+256+48]

    movdqa [rdx+128], xmm1      ; store xor temp
    movdqa [rdx+128+16], xmm2
    movdqa [rdx+128+32], xmm3
    movdqa [rdx+128+48], xmm4
    movdqa [rdx+256],    xmm5
    movdqa [rdx+256+16], xmm6
    movdqa [rdx+256+32], xmm7
    movdqa [rdx+256+48], xmm8

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rcx]
    movdqu  xmm6, [rcx+16]
    movdqu  xmm7, [rcx+32]
    movdqu  xmm8, [rcx+48]

    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor temp
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    pxor    xmm0, xmm5
    pxor    xmm5, [rdx]
    pxor    xmm0, xmm6
    pxor    xmm6, [rdx+16]
    pxor    xmm0, xmm7
    pxor    xmm7, [rdx+32]
    pxor    xmm0, xmm8
    pxor    xmm8, [rdx+48]

    movdqa [rdx+384], xmm1      ; store xor temp
    movdqa [rdx+384+16], xmm2
    movdqa [rdx+384+32], xmm3
    movdqa [rdx+384+48], xmm4
    movdqa [rdx],    xmm5
    movdqa [rdx+16], xmm6
    movdqa [rdx+32], xmm7
    movdqa [rdx+48], xmm8

    jmp     fold_m

;; Following routine is created to optimize MR3 write operation.
;; temp buffer is always 16-byte aligned and is maintained in L2 cache.
;; This routine is called if srcptr and trgptr are not 16 or 8-byte aligned
;; mmx optimize with prefetch sector (checksum ^= *srcptr, *tgtptr++ = tempptr++ ^ *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx
;;        (const UINT_32E *srcptr, UINT_32E *tempptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tempptr
;;  r8   tarptr 
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx
fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx_aok
fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc [misaligned_xor_cpy_cnt]
%endif
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause
    pxor    xmm0, xmm0          ; xmm0 checksum = 0

    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    movdqu [r8+192],    xmm1    ; store xor temp
    movdqu [r8+192+16], xmm2
    movdqu [r8+192+32], xmm3
    movdqu [r8+192+48], xmm4

    movdqu  xmm1, [rcx+64]      ; load src
    movdqu  xmm2, [rcx+64+16]
    movdqu  xmm3, [rcx+64+32]
    movdqu  xmm4, [rcx+64+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+64]      ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+64+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+64+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+64+48]
    movdqu  [r8+64],    xmm1    ; store xor temp
    movdqu  [r8+64+16], xmm2
    movdqu  [r8+64+32], xmm3
    movdqu  [r8+64+48], xmm4

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    movdqu  xmm4, [rcx+448+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+448+48]
    movdqu  [r8+448],    xmm1   ; store xor temp
    movdqu  [r8+448+16], xmm2
    movdqu  [r8+448+32], xmm3
    movdqu  [r8+448+48], xmm4

    movdqu  xmm1, [rcx+320]     ; load src
    movdqu  xmm2, [rcx+320+16]
    movdqu  xmm3, [rcx+320+32]
    movdqu  xmm4, [rcx+320+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+320]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+320+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+320+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+320+48]
    movdqu  [r8+320],    xmm1   ; store xor temp
    movdqu  [r8+320+16], xmm2
    movdqu  [r8+320+32], xmm3
    movdqu  [r8+320+48], xmm4

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    movdqu  [r8+128],    xmm1   ; store xor temp
    movdqu  [r8+128+16], xmm2
    movdqu  [r8+128+32], xmm3
    movdqu  [r8+128+48], xmm4

    movdqu  xmm1, [rcx+256]     ; load src
    movdqu  xmm2, [rcx+256+16]
    movdqu  xmm3, [rcx+256+32]
    movdqu  xmm4, [rcx+256+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+256]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+256+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+256+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+256+48]
    movdqu  [r8+256],    xmm1   ; store xor temp
    movdqu  [r8+256+16], xmm2
    movdqu  [r8+256+32], xmm3
    movdqu  [r8+256+48], xmm4

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    movdqu  [r8+384],    xmm1   ; store xor temp
    movdqu  [r8+384+16], xmm2
    movdqu  [r8+384+32], xmm3
    movdqu  [r8+384+48], xmm4

    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx]         ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+48]
    movdqu  [r8],    xmm1       ; store xor temp
    movdqu  [r8+16], xmm2
    movdqu  [r8+32], xmm3
    movdqu  [r8+48], xmm4
    
  fold_n:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0    

;; Following routine is created to optimize MR3 write operation.
;; temp buffer is always 16-byte aligned and is maintained in L2 cache
;; mmx optimize with prefetch sector (checksum ^= *srcptr, *tgtptr++ = tempptr++ ^ *srcptr++)
;; UINT_32 fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx
;;        (const UINT_32E *srcptr, UINT_32E *tempptr, UINT_32E *tgtptr)
;;  rcx  srcptr
;;  rdx  tempptr
;;  r8   trgptr 
;; returns: eax = 32-bit checksum
ALIGN 16
GLOBAL fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx
fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    test    rcx, 7              ; srcptr 16-byte aligned?
    jnz     fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx_aok ; jump to misaligned routine
    test    r8, 7               ; trgptr 16-byte aligned?
    jnz     fbe_xor_calc_csum_and_xor_with_temp_and_cpy_misaligned_mmx_aok ; jump to misaligned routine
     
    pxor    xmm0, xmm0          ; xmm0 checksum = 0
    test    r8, 15              ; tgtptr 16-byte aligned?
    jz     aligned_tgt_aligned_temp
    ;; if unaligned target, first and last loads are 8-bytes instead of 16.        
    mov     rax,  [rcx]         ; load first 8-bytes of src
    movq    xmm1, qword ptr [rcx]         ; load first 8-bytes of src
    pxor    xmm0, xmm1          ; running src checksum
    xor     rax,  [rdx]         ; xor temp buf into rax
    movnti  [r8], rax           ; store to xor temp
    mov     rax,  [rcx+504]     ; load last 8-bytes of src
    movq    xmm1, qword ptr [rcx+504]     ; load last 8-bytes of src
    pxor    xmm0, xmm1          ; running src checksum
    xor     rax,  [rdx+504]     ; xor temp buf into rax
    movnti  [r8+504], rax       ; store to xor temp
    add     rcx, 8              ; update srcptr, and
    add     rdx, 8              ; tempptr is now unaligned
    add     r8, 8               ; tgtptr now aligned.
    jmp     unaligned_tgt_aligned_temp
    
  aligned_tgt_aligned_temp:
    movdqu  xmm4, [rcx+448+48]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+448+48]
    movdqa [r8+448+48], xmm4
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause
    
    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src1;
    movdqa  xmm1, [rcx+192]     ; load src
    movdqa  xmm2, [rcx+192+16]
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    movdqa [r8+192],    xmm1   ; store xor temp
    movdqa [r8+192+16], xmm2
    movdqa [r8+192+32], xmm3
    movdqa [r8+192+48], xmm4

    movdqa  xmm1, [rcx+64]      ; load src
    movdqa  xmm2, [rcx+64+16]
    movdqa  xmm3, [rcx+64+32]
    movdqa  xmm4, [rcx+64+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+64]      ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+64+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+64+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+64+48]
    movdqa [r8+64],    xmm1    ; store xor temp
    movdqa [r8+64+16], xmm2
    movdqa [r8+64+32], xmm3
    movdqa [r8+64+48], xmm4
    
    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48]  ; This case handled before prefetches 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4          ; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]  ; This case handled before prefetches
    movdqa [r8+448],    xmm1   ; store xor temp
    movdqa [r8+448+16], xmm2
    movdqa [r8+448+32], xmm3
    ;;movdqa [r8+448+48], xmm4   ; This case handled before prefetches

    movdqa  xmm1, [rcx+320]     ; load src
    movdqa  xmm2, [rcx+320+16]
    movdqa  xmm3, [rcx+320+32]
    movdqa  xmm4, [rcx+320+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+320]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+320+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+320+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+320+48]
    movdqa [r8+320],    xmm1   ; store xor temp
    movdqa [r8+320+16], xmm2
    movdqa [r8+320+32], xmm3
    movdqa [r8+320+48], xmm4

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    movdqa [r8+128],    xmm1   ; store xor temp
    movdqa [r8+128+16], xmm2
    movdqa [r8+128+32], xmm3
    movdqa [r8+128+48], xmm4

    movdqa  xmm1, [rcx+256]     ; load src
    movdqa  xmm2, [rcx+256+16]
    movdqa  xmm3, [rcx+256+32]
    movdqa  xmm4, [rcx+256+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+256]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+256+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+256+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+256+48]
    movdqa [r8+256],    xmm1   ; store xor temp
    movdqa [r8+256+16], xmm2
    movdqa [r8+256+32], xmm3
    movdqa [r8+256+48], xmm4

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    movdqa [r8+384],    xmm1   ; store xor temp
    movdqa [r8+384+16], xmm2
    movdqa [r8+384+32], xmm3
    movdqa [r8+384+48], xmm4

    movdqa  xmm1, [rcx]         ; load src
    movdqa  xmm2, [rcx+16]
    movdqa  xmm3, [rcx+32]
    movdqa  xmm4, [rcx+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx]         ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+48]
    movdqa [r8],    xmm1       ; store xor temp
    movdqa [r8+16], xmm2
    movdqa [r8+32], xmm3
    movdqa [r8+48], xmm4
    
  fold_o:
    movdqa  xmm5, xmm0          ; 128-bit checksum
    psrldq  xmm0, 8             ; high 64-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movdqa  xmm5, xmm0          ; 64-bit checksum
    psrldq  xmm0, 4             ; high 32-bits
    pxor    xmm0, xmm5          ; xor hi/lo
    movd    eax, xmm0           ; ret 32-bit checksum 
    ret     0

  unaligned_src1:               ; tgtptr is aligned
    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+192]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+192+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+192+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+192+48]
    movdqa [r8+192],    xmm1   ; store xor temp
    movdqa [r8+192+16], xmm2
    movdqa [r8+192+32], xmm3
    movdqa [r8+192+48], xmm4

    movdqu  xmm1, [rcx+64]      ; load src
    movdqu  xmm2, [rcx+64+16]
    movdqu  xmm3, [rcx+64+32]
    movdqu  xmm4, [rcx+64+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+64]      ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+64+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+64+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+64+48]
    movdqa [r8+64],    xmm1    ; store xor temp
    movdqa [r8+64+16], xmm2
    movdqa [r8+64+32], xmm3
    movdqa [r8+64+48], xmm4

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48]  ; This case handled before prefetches 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+448]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+448+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+448+32]
    ;;pxor    xmm0, xmm4           ; This case handled before prefetches
    ;;pxor    xmm4, [rdx+448+48]   ; This case handled before prefetches
    movdqa [r8+448],    xmm1   ; store xor temp
    movdqa [r8+448+16], xmm2
    movdqa [r8+448+32], xmm3
    ;;movdqa [r8+448+48], xmm4    ; This case handled before prefetches

    movdqu  xmm1, [rcx+320]     ; load src
    movdqu  xmm2, [rcx+320+16]
    movdqu  xmm3, [rcx+320+32]
    movdqu  xmm4, [rcx+320+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+320]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+320+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+320+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+320+48]
    movdqa [r8+320],    xmm1   ; store xor temp
    movdqa [r8+320+16], xmm2
    movdqa [r8+320+32], xmm3
    movdqa [r8+320+48], xmm4

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+128]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+128+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+128+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+128+48]
    movdqa [r8+128],    xmm1   ; store xor temp
    movdqa [r8+128+16], xmm2
    movdqa [r8+128+32], xmm3
    movdqa [r8+128+48], xmm4

    movdqu  xmm1, [rcx+256]     ; load src
    movdqu  xmm2, [rcx+256+16]
    movdqu  xmm3, [rcx+256+32]
    movdqu  xmm4, [rcx+256+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+256]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+256+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+256+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+256+48]
    movdqa [r8+256],    xmm1   ; store xor temp
    movdqa [r8+256+16], xmm2
    movdqa [r8+256+32], xmm3
    movdqa [r8+256+48], xmm4

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx+384]     ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+384+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+384+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+384+48]
    movdqa [r8+384],    xmm1   ; store xor temp
    movdqa [r8+384+16], xmm2
    movdqa [r8+384+32], xmm3
    movdqa [r8+384+48], xmm4

    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48] 
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, [rdx]         ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, [rdx+16]
    pxor    xmm0, xmm3
    pxor    xmm3, [rdx+32]
    pxor    xmm0, xmm4
    pxor    xmm4, [rdx+48]
    movdqa [r8],    xmm1       ; store xor temp
    movdqa [r8+16], xmm2
    movdqa [r8+32], xmm3
    movdqa [r8+48], xmm4
    jmp     fold_o

  unaligned_tgt_aligned_temp:
    prefetchnta [rcx+192]
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]           ; srcptr
    pause      
    test    rcx, 15             ; srcptr 16-byte aligned?
    jnz     unaligned_src2;
    movdqa  xmm1, [rcx+192]     ; load src
    movdqa  xmm2, [rcx+192+16]
    movdqa  xmm3, [rcx+192+32]
    movdqa  xmm4, [rcx+192+48]
    movdqu  xmm5, [rdx+192]     ; temp buff
    movdqu  xmm6, [rdx+192+16]
    movdqu  xmm7, [rdx+192+32]
    movdqu  xmm8, [rdx+192+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+192],    xmm1   ; store xor temp
    movdqa [r8+192+16], xmm2
    movdqa [r8+192+32], xmm3
    movdqa [r8+192+48], xmm4

    movdqa  xmm1, [rcx+64]      ; load src
    movdqa  xmm2, [rcx+64+16]
    movdqa  xmm3, [rcx+64+32]
    movdqa  xmm4, [rcx+64+48]
    movdqu  xmm5, [rdx+64]      ; temp buff
    movdqu  xmm6, [rdx+64+16]
    movdqu  xmm7, [rdx+64+32]
    movdqu  xmm8, [rdx+64+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+64],    xmm1    ; store xor temp
    movdqa [r8+64+16], xmm2
    movdqa [r8+64+32], xmm3
    movdqa [r8+64+48], xmm4

    movdqa  xmm1, [rcx+448]     ; load src
    movdqa  xmm2, [rcx+448+16]
    movdqa  xmm3, [rcx+448+32]
    ;;movdqa  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqu  xmm5, [rdx+448]     ; temp buff
    movdqu  xmm6, [rdx+448+16]
    movdqu  xmm7, [rdx+448+32]
    ;;movdqu  xmm8, [rdx+448+48] ;;This case handled before prefetches
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    ;;pxor    xmm0, xmm4      ;;This case handled before prefetches
    ;;pxor    xmm4, xmm8      ;;This case handled before prefetches
    movdqa [r8+448],    xmm1   ; store xor temp
    movdqa [r8+448+16], xmm2
    movdqa [r8+448+32], xmm3
    ;;movdqa [r8+448+48], xmm4 ;;This case handled before prefetches

    movdqa  xmm1, [rcx+320]     ; load src
    movdqa  xmm2, [rcx+320+16]
    movdqa  xmm3, [rcx+320+32]
    movdqa  xmm4, [rcx+320+48]
    movdqu  xmm5, [rdx+320]  ; temp buff
    movdqu  xmm6, [rdx+320+16]
    movdqu  xmm7, [rdx+320+32]
    movdqu  xmm8, [rdx+320+48]
    pxor    xmm0, xmm1       ; running src checksum
    pxor    xmm1, xmm5       ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+320],    xmm1   ; store xor temp
    movdqa [r8+320+16], xmm2
    movdqa [r8+320+32], xmm3
    movdqa [r8+320+48], xmm4

    movdqa  xmm1, [rcx+128]     ; load src
    movdqa  xmm2, [rcx+128+16]
    movdqa  xmm3, [rcx+128+32]
    movdqa  xmm4, [rcx+128+48]
    movdqu  xmm5, [rdx+128]     ; temp buff
    movdqu  xmm6, [rdx+128+16]
    movdqu  xmm7, [rdx+128+32]
    movdqu  xmm8, [rdx+128+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+128],    xmm1   ; store xor temp
    movdqa [r8+128+16], xmm2
    movdqa [r8+128+32], xmm3
    movdqa [r8+128+48], xmm4

    movdqa  xmm1, [rcx+256]     ; load src
    movdqa  xmm2, [rcx+256+16]
    movdqa  xmm3, [rcx+256+32]
    movdqa  xmm4, [rcx+256+48]
    movdqu  xmm5, [rdx+256]     ; load src
    movdqu  xmm6, [rdx+256+16]
    movdqu  xmm7, [rdx+256+32]
    movdqu  xmm8, [rdx+256+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+256],    xmm1   ; store xor temp
    movdqa [r8+256+16], xmm2
    movdqa [r8+256+32], xmm3
    movdqa [r8+256+48], xmm4

    movdqa  xmm1, [rcx+384]     ; load src
    movdqa  xmm2, [rcx+384+16]
    movdqa  xmm3, [rcx+384+32]
    movdqa  xmm4, [rcx+384+48]
    movdqu  xmm5, [rdx+384]     ; temp buff
    movdqu  xmm6, [rdx+384+16]
    movdqu  xmm7, [rdx+384+32]
    movdqu  xmm8, [rdx+384+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+384],    xmm1   ; store xor temp
    movdqa [r8+384+16], xmm2
    movdqa [r8+384+32], xmm3
    movdqa [r8+384+48], xmm4

    movdqa  xmm1, [rcx]         ; load src
    movdqa  xmm2, [rcx+16]
    movdqa  xmm3, [rcx+32]
    movdqa  xmm4, [rcx+48]
    movdqu  xmm5, [rdx]         ; temp buff
    movdqu  xmm6, [rdx+16]
    movdqu  xmm7, [rdx+32]
    movdqu  xmm8, [rdx+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8],    xmm1       ; store xor temp
    movdqa [r8+16], xmm2
    movdqa [r8+32], xmm3
    movdqa [r8+48], xmm4
    jmp fold_o

  unaligned_src2:               ; tgtptr is aligned
    movdqu  xmm1, [rcx+192]     ; load src
    movdqu  xmm2, [rcx+192+16]
    movdqu  xmm3, [rcx+192+32]
    movdqu  xmm4, [rcx+192+48]
    movdqu  xmm5, [rdx+192]     ; load temp buff
    movdqu  xmm6, [rdx+192+16]
    movdqu  xmm7, [rdx+192+32]
    movdqu  xmm8, [rdx+192+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+192],    xmm1   ; store xor temp
    movdqa [r8+192+16], xmm2
    movdqa [r8+192+32], xmm3
    movdqa [r8+192+48], xmm4

    movdqu  xmm1, [rcx+64]      ; load src
    movdqu  xmm2, [rcx+64+16]
    movdqu  xmm3, [rcx+64+32]
    movdqu  xmm4, [rcx+64+48]
    movdqu  xmm5, [rdx+64]      ; load temp buff
    movdqu  xmm6, [rdx+64+16]
    movdqu  xmm7, [rdx+64+32]
    movdqu  xmm8, [rdx+64+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+64],    xmm1    ; store xor temp
    movdqa [r8+64+16], xmm2
    movdqa [r8+64+32], xmm3
    movdqa [r8+64+48], xmm4

    movdqu  xmm1, [rcx+448]     ; load src
    movdqu  xmm2, [rcx+448+16]
    movdqu  xmm3, [rcx+448+32]
    ;;movdqu  xmm4, [rcx+448+48] ;;This case handled before prefetches
    movdqu  xmm5, [rdx+448]     ; temp buff
    movdqu  xmm6, [rdx+448+16]
    movdqu  xmm7, [rdx+448+32]
    ;;movdqu  xmm8, [rdx+448+48] ;;This case handled before prefetches
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    ;;pxor    xmm0, xmm4     ;;This case handled before prefetches
    ;;pxor    xmm4, xmm8     ;;This case handled before prefetches
    movdqa [r8+448],    xmm1   ; store xor temp
    movdqa [r8+448+16], xmm2
    movdqa [r8+448+32], xmm3
    ;;movdqa [r8+448+48], xmm4 ;;This case handled before prefetches

    movdqu  xmm1, [rcx+320]     ; load src
    movdqu  xmm2, [rcx+320+16]
    movdqu  xmm3, [rcx+320+32]
    movdqu  xmm4, [rcx+320+48]
    movdqu  xmm5, [rdx+320]     ; temp buff
    movdqu  xmm6, [rdx+320+16]
    movdqu  xmm7, [rdx+320+32]
    movdqu  xmm8, [rdx+320+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+320],    xmm1   ; store xor temp
    movdqa [r8+320+16], xmm2
    movdqa [r8+320+32], xmm3
    movdqa [r8+320+48], xmm4

    movdqu  xmm1, [rcx+128]     ; load src
    movdqu  xmm2, [rcx+128+16]
    movdqu  xmm3, [rcx+128+32]
    movdqu  xmm4, [rcx+128+48]
    movdqu  xmm5, [rdx+128]     ; temp buff
    movdqu  xmm6, [rdx+128+16]
    movdqu  xmm7, [rdx+128+32]
    movdqu  xmm8, [rdx+128+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+128],    xmm1   ; store xor temp
    movdqa [r8+128+16], xmm2
    movdqa [r8+128+32], xmm3
    movdqa [r8+128+48], xmm4

    movdqu  xmm1, [rcx+256]     ; load src
    movdqu  xmm2, [rcx+256+16]
    movdqu  xmm3, [rcx+256+32]
    movdqu  xmm4, [rcx+256+48]
    movdqu  xmm5, [rdx+256]     ; temp buff
    movdqu  xmm6, [rdx+256+16]
    movdqu  xmm7, [rdx+256+32]
    movdqu  xmm8, [rdx+256+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+256],    xmm1   ; store xor temp
    movdqa [r8+256+16], xmm2
    movdqa [r8+256+32], xmm3
    movdqa [r8+256+48], xmm4

    movdqu  xmm1, [rcx+384]     ; load src
    movdqu  xmm2, [rcx+384+16]
    movdqu  xmm3, [rcx+384+32]
    movdqu  xmm4, [rcx+384+48]
    movdqu  xmm5, [rdx+384]     ; temp buff
    movdqu  xmm6, [rdx+384+16]
    movdqu  xmm7, [rdx+384+32]
    movdqu  xmm8, [rdx+384+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8+384],    xmm1   ; store xor temp
    movdqa [r8+384+16], xmm2
    movdqa [r8+384+32], xmm3
    movdqa [r8+384+48], xmm4

    movdqu  xmm1, [rcx]         ; load src
    movdqu  xmm2, [rcx+16]
    movdqu  xmm3, [rcx+32]
    movdqu  xmm4, [rcx+48]
    movdqu  xmm5, [rdx]         ; temp buff
    movdqu  xmm6, [rdx+16]
    movdqu  xmm7, [rdx+32]
    movdqu  xmm8, [rdx+48]
    pxor    xmm0, xmm1          ; running src checksum
    pxor    xmm1, xmm5          ; xor temp buff
    pxor    xmm0, xmm2 
    pxor    xmm2, xmm6
    pxor    xmm0, xmm3
    pxor    xmm3, xmm7
    pxor    xmm0, xmm4
    pxor    xmm4, xmm8
    movdqa [r8],    xmm1       ; store xor temp
    movdqa [r8+16], xmm2
    movdqa [r8+32], xmm3
    movdqa [r8+48], xmm4   
    jmp     fold_o
    

;; Following routine is created to optimize 468 write operation.
;; This routine is called if blocks are neither all 16-byte aligned nor all 8-byte aligned
;; mmx optimize with prefetch sector (
;; csum[0] ^= *old_dblk, 
;; csum[1] ^= *new_dblk,
;; csum[2] ^= *pblk,
;; *pblk++ ^= old_dblk++ ^ *new_dblk++)
;; void fbe_xor_468_calc_csum_and_update_parity_mmx
;;                      (const UINT_32E *old_dblk, UINT_32E *new_dblk, 
;;                       UINT_32E *pblk, UINT_32E *csum)
;;  rcx  old_dblk
;;  rdx  new_dblk
;;  r8   pblk
;;  r9   csum 
;; returns: None
ALIGN 16
GLOBAL fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx
fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
GLOBAL fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok
fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok:
%ifdef _COUNTERS_
    inc [misaligned_xor_fbe_xor_cnt]
%endif
    prefetchnta [rcx+192]       ;old_dblk
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]    
    prefetchnta [rdx+192]       ;new_dblk
    prefetchnta [rdx+ 64]
    prefetchnta [rdx+448]
    prefetchnta [rdx+320]
    prefetchnta [rdx+128]
    prefetchnta [rdx+256]
    prefetchnta [rdx+384]
    prefetchnta [rdx]       
    prefetchnta [r8+192]        ;pblk
    prefetchnta [r8+ 64]
    prefetchnta [r8+448]
    prefetchnta [r8+320]
    prefetchnta [r8+128]
    prefetchnta [r8+256]
    prefetchnta [r8+384]
    prefetchnta [r8]
    pause
;;Perform computation on 64 byte chunk starting at offset 192
    ;;First, load data 
    movdqu  xmm0,  [rcx+192]    ; load old_dblk
    movdqu  xmm1,  [rcx+192+16]
    movdqu  xmm2,  [rcx+192+32]
    movdqu  xmm3,  [rcx+192+48]
    movdqu  xmm4,  [rdx+192]    ; load new_dblk
    movdqu  xmm5,  [rdx+192+16]
    movdqu  xmm6,  [rdx+192+32]
    movdqu  xmm7,  [rdx+192+48]
    movdqu  xmm8,  [r8+192]     ; load pblk
    movdqu  xmm9,  [r8+192+16]
    movdqu  xmm10, [r8+192+32]
    movdqu  xmm11, [r8+192+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+192],    xmm8   ; Store to pblk
    movdqu  [r8+192+16], xmm9
    movdqu  [r8+192+32], xmm10
    movdqu  [r8+192+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    movdqa  xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    movdqa  xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 64
    ;;First, load data 
    movdqu  xmm0,  [rcx+64]     ; load old_dblk
    movdqu  xmm1,  [rcx+64+16]
    movdqu  xmm2,  [rcx+64+32]
    movdqu  xmm3,  [rcx+64+48]
    movdqu  xmm4,  [rdx+64]     ; load new_dblk
    movdqu  xmm5,  [rdx+64+16]
    movdqu  xmm6,  [rdx+64+32]
    movdqu  xmm7,  [rdx+64+48]
    movdqu  xmm8,  [r8+64]      ; load pblk
    movdqu  xmm9,  [r8+64+16]
    movdqu  xmm10, [r8+64+32]
    movdqu  xmm11, [r8+64+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+64],    xmm8    ; Store to pblk
    movdqu  [r8+64+16], xmm9
    movdqu  [r8+64+32], xmm10
    movdqu  [r8+64+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;First, load data 
    movdqu  xmm0,  [rcx+448]    ; load old_dblk
    movdqu  xmm1,  [rcx+448+16]
    movdqu  xmm2,  [rcx+448+32]
    movdqu  xmm3,  [rcx+448+48]
    movdqu  xmm4,  [rdx+448]    ; load new_dblk
    movdqu  xmm5,  [rdx+448+16]
    movdqu  xmm6,  [rdx+448+32]
    movdqu  xmm7,  [rdx+448+48]
    movdqu  xmm8,  [r8+448]     ; load pblk
    movdqu  xmm9,  [r8+448+16]
    movdqu  xmm10, [r8+448+32]
    movdqu  xmm11, [r8+448+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+448],    xmm8        ; Store to pblk
    movdqu  [r8+448+16], xmm9
    movdqu  [r8+448+32], xmm10
    movdqu  [r8+448+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 320
    ;;First, load data 
    movdqu  xmm0,  [rcx+320]    ; load old_dblk
    movdqu  xmm1,  [rcx+320+16]
    movdqu  xmm2,  [rcx+320+32]
    movdqu  xmm3,  [rcx+320+48]
    movdqu  xmm4,  [rdx+320]    ; load new_dblk
    movdqu  xmm5,  [rdx+320+16]
    movdqu  xmm6,  [rdx+320+32]
    movdqu  xmm7,  [rdx+320+48]
    movdqu  xmm8,  [r8+320]     ; load pblk
    movdqu  xmm9,  [r8+320+16]
    movdqu  xmm10, [r8+320+32]
    movdqu  xmm11, [r8+320+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+320],    xmm8   ; Store to pblk
    movdqu  [r8+320+16], xmm9
    movdqu  [r8+320+32], xmm10
    movdqu  [r8+320+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 128
    ;;First, load data 
    movdqu  xmm0,  [rcx+128]    ; load old_dblk
    movdqu  xmm1,  [rcx+128+16]
    movdqu  xmm2,  [rcx+128+32]
    movdqu  xmm3,  [rcx+128+48]
    movdqu  xmm4,  [rdx+128]    ; load new_dblk
    movdqu  xmm5,  [rdx+128+16]
    movdqu  xmm6,  [rdx+128+32]
    movdqu  xmm7,  [rdx+128+48]
    movdqu  xmm8,  [r8+128]     ; load pblk
    movdqu  xmm9,  [r8+128+16]
    movdqu  xmm10, [r8+128+32]
    movdqu  xmm11, [r8+128+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+128],    xmm8   ; Store to pblk
    movdqu  [r8+128+16], xmm9
    movdqu  [r8+128+32], xmm10
    movdqu  [r8+128+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqu  xmm0,  [rcx+256]    ; load old_dblk
    movdqu  xmm1,  [rcx+256+16]
    movdqu  xmm2,  [rcx+256+32]
    movdqu  xmm3,  [rcx+256+48]
    movdqu  xmm4,  [rdx+256]    ; load new_dblk
    movdqu  xmm5,  [rdx+256+16]
    movdqu  xmm6,  [rdx+256+32]
    movdqu  xmm7,  [rdx+256+48]
    movdqu  xmm8,  [r8+256]     ; load pblk
    movdqu  xmm9,  [r8+256+16]
    movdqu  xmm10, [r8+256+32]
    movdqu  xmm11, [r8+256+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+256],    xmm8   ; Store to pblk
    movdqu  [r8+256+16], xmm9
    movdqu  [r8+256+32], xmm10
    movdqu  [r8+256+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqu  xmm0,  [rcx+384]    ; load old_dblk
    movdqu  xmm1,  [rcx+384+16]
    movdqu  xmm2,  [rcx+384+32]
    movdqu  xmm3,  [rcx+384+48]
    movdqu  xmm4,  [rdx+384]    ; load new_dblk
    movdqu  xmm5,  [rdx+384+16]
    movdqu  xmm6,  [rdx+384+32]
    movdqu  xmm7,  [rdx+384+48]
    movdqu  xmm8,  [r8+384]     ; load pblk
    movdqu  xmm9,  [r8+384+16]
    movdqu  xmm10, [r8+384+32]
    movdqu  xmm11, [r8+384+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8+384],    xmm8   ; Store to pblk
    movdqu  [r8+384+16], xmm9
    movdqu  [r8+384+32], xmm10
    movdqu  [r8+384+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;First, load data 
    movdqu  xmm0,  [rcx]        ; load old_dblk
    movdqu  xmm1,  [rcx+16]
    movdqu  xmm2,  [rcx+32]
    movdqu  xmm3,  [rcx+48]
    movdqu  xmm4,  [rdx]        ; load new_dblk
    movdqu  xmm5,  [rdx+16]
    movdqu  xmm6,  [rdx+32]
    movdqu  xmm7,  [rdx+48]
    movdqu  xmm8,  [r8]         ; load pblk
    movdqu  xmm9,  [r8+16]
    movdqu  xmm10, [r8+32]
    movdqu  xmm11, [r8+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqu  [r8],    xmm8       ; Store to pblk
    movdqu  [r8+16], xmm9
    movdqu  [r8+32], xmm10
    movdqu  [r8+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7
       
  fold_p:
    ;; Fold csum of old_dblk which is in xmm13 using xmm0 as scratch 
    movdqa  xmm0,  xmm13        ; 128-bit checksum
    psrldq  xmm13, 8            ; high 64-bits
    pxor    xmm13, xmm0         ; xor hi/lo
    movdqa  xmm0,  xmm13        ; 64-bit checksum
    psrldq  xmm13, 4            ; high 32-bits
    pxor    xmm13, xmm0         ; xor hi/lo
    movd    dword ptr [r9],  xmm13        ; ret 32-bit checksum 

    ;; Fold csum of old_dblk which is in xmm14 using xmm1 as scratch 
    movdqa  xmm1,  xmm14        ; 128-bit checksum
    psrldq  xmm14, 8            ; high 64-bits
    pxor    xmm14, xmm1         ; xor hi/lo
    movdqa  xmm1,  xmm14        ; 64-bit checksum
    psrldq  xmm14, 4            ; high 32-bits
    pxor    xmm14, xmm1         ; xor hi/lo
    movd    dword ptr [r9+4],xmm14        ; ret 32-bit checksum  
    ret     0



;; Following routine is created to optimize 468 write operation.
;; mmx optimize with prefetch sector (
;; csum[0] ^= *old_dblk, 
;; csum[1] ^= *new_dblk,
;; *pblk++ ^= old_dblk++ ^ *new_dblk++)
;; void fbe_xor_468_calc_csum_and_update_parity_mmx
;;                      (const UINT_32E *old_dblk, UINT_32E *new_dblk, 
;;                       UINT_32E *pblk, UINT_32E *csum)
;;  rcx  old_dblk
;;  rdx  new_dblk
;;  r8   pblk
;;  r9   csum 
;; returns: None
ALIGN 16
GLOBAL fbe_xor_468_calc_csum_and_update_parity_mmx
fbe_xor_468_calc_csum_and_update_parity_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    ;;Make sure that all pointers are 8 or 16 byte aligned
    test    r8,  7              ; pblk 8 or 16-byte aligned?
    jnz     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine     
    test    rcx, 7              ; old_dblk 8 or 16-byte aligned?
    jnz     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine
    test    rdx, 7              ; new_dblk 8 or 16-byte aligned?
    jnz     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine     
    
    ;;Make sure that if pblk is 8-byte aligned, old_dblk and new_dblk are also 8-byte aligned
    test    r8, 15              ; pblk 16-byte aligned?
    jz      alignment_16
;;    jmp     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; BROKEN 
    test    rcx,  15            ; old_dblk 16-byte aligned?
    jz      fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine 
    test    rdx, 15             ; new_dblk 16-byte aligned?
    jz      fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine     
    jmp     alignment_8
    
 alignment_16:
    ;;Make sure that new_dblk and pblk are also 16-byte aligned
    test    rcx, 15             ; old_dblk 16-byte aligned?
    jnz     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine     
    test    rdx,  15            ; new_dblk 8 or 16-byte aligned?
    jnz     fbe_xor_468_calc_csum_and_update_parity_misaligned_mmx_aok ; jump to misaligned routine

    pxor    xmm13, xmm13        ; xmm13 = 0
    pxor    xmm14, xmm14        ; xmm14 = 0
    movdqa  xmm3,  [rcx+448+48] ; load old_dblk
    movdqa  xmm7,  [rdx+448+48] ; load new_dblk
    movdqa  xmm11, [r8+448+48]  ; load p_dblk
    pxor    xmm11, xmm3         ; XOR old_dblk with pblk
    pxor    xmm11, xmm7         ; XOR new_dblk with pblk
    movdqa [r8+448+48], xmm11  ; Store to pblk
    pxor    xmm13, xmm3         ; accumulate csum of old_dblk into xmm13
    pxor    xmm14, xmm7         ; accumulate csum of new_dblk into xmm14  

    prefetchnta [rcx+192]       ;old_dblk
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]    
    prefetchnta [rdx+192]       ;new_dblk
    prefetchnta [rdx+ 64]
    prefetchnta [rdx+448]
    prefetchnta [rdx+320]
    prefetchnta [rdx+128]
    prefetchnta [rdx+256]
    prefetchnta [rdx+384]
    prefetchnta [rdx]       
    prefetchnta [r8+192]        ;pblk
    prefetchnta [r8+ 64]
    prefetchnta [r8+448]
    prefetchnta [r8+320]
    prefetchnta [r8+128]
    prefetchnta [r8+256]
    prefetchnta [r8+384]
    prefetchnta [r8]
    pause
    ;;Perform computation on 64 byte chunk starting at offset 192
    ;;First, load data 
    movdqa  xmm0,  [rcx+192]    ; load old_dblk
    movdqa  xmm1,  [rcx+192+16]
    movdqa  xmm2,  [rcx+192+32]
    movdqa  xmm3,  [rcx+192+48]
    movdqa  xmm4,  [rdx+192]    ; load new_dblk
    movdqa  xmm5,  [rdx+192+16]
    movdqa  xmm6,  [rdx+192+32]
    movdqa  xmm7,  [rdx+192+48]
    movdqa  xmm8,  [r8+192]     ; load pblk
    movdqa  xmm9,  [r8+192+16]
    movdqa  xmm10, [r8+192+32]
    movdqa  xmm11, [r8+192+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+192],    xmm8   ; Store to pblk
    movdqa [r8+192+16], xmm9
    movdqa [r8+192+32], xmm10
    movdqa [r8+192+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 64
    ;;First, load data 
    movdqa  xmm0,  [rcx+64]     ; load old_dblk
    movdqa  xmm1,  [rcx+64+16]
    movdqa  xmm2,  [rcx+64+32]
    movdqa  xmm3,  [rcx+64+48]
    movdqa  xmm4,  [rdx+64]     ; load new_dblk
    movdqa  xmm5,  [rdx+64+16]
    movdqa  xmm6,  [rdx+64+32]
    movdqa  xmm7,  [rdx+64+48]
    movdqa  xmm8,  [r8+64]      ; load pblk
    movdqa  xmm9,  [r8+64+16]
    movdqa  xmm10, [r8+64+32]
    movdqa  xmm11, [r8+64+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+64],    xmm8    ; Store to pblk
    movdqa [r8+64+16], xmm9
    movdqa [r8+64+32], xmm10
    movdqa [r8+64+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;First, load data 
    movdqa  xmm0,  [rcx+448]    ; load old_dblk
    movdqa  xmm1,  [rcx+448+16]
    movdqa  xmm2,  [rcx+448+32]
    ;;movdqa  xmm3,  [rcx+448+48] ; This case handled before prefetches
    movdqa  xmm4,  [rdx+448]    ; load new_dblk
    movdqa  xmm5,  [rdx+448+16]
    movdqa  xmm6,  [rdx+448+32]
    ;;movdqa  xmm7,  [rdx+448+48] ; This case handled before prefetches
    movdqa  xmm8,  [r8+448]     ; load pblk
    movdqa  xmm9,  [r8+448+16]
    movdqa  xmm10, [r8+448+32]
    ;;movdqa  xmm11, [r8+448+48]  ; This case handled before prefetches 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    ;;pxor    xmm11, xmm3       ; This case handled before prefetches
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    ;;pxor    xmm11, xmm7       ; This case handled before prefetches 
    movdqa [r8+448],    xmm8   ; Store to pblk
    movdqa [r8+448+16], xmm9
    movdqa [r8+448+32], xmm10
    ;;movdqa [r8+448+48], xmm11  ; This case handled before prefetches
    
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    ;;pxor    xmm13, xmm3         ; This case handled before prefetches 
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    ;;pxor    xmm14, xmm7         ; This case handled before prefetches 

    ;;Perform computation on 64 byte chunk starting at offset 320
    ;;First, load data 
    movdqa  xmm0,  [rcx+320]    ; load old_dblk
    movdqa  xmm1,  [rcx+320+16]
    movdqa  xmm2,  [rcx+320+32]
    movdqa  xmm3,  [rcx+320+48]
    movdqa  xmm4,  [rdx+320]    ; load new_dblk
    movdqa  xmm5,  [rdx+320+16]
    movdqa  xmm6,  [rdx+320+32]
    movdqa  xmm7,  [rdx+320+48]
    movdqa  xmm8,  [r8+320]     ; load pblk
    movdqa  xmm9,  [r8+320+16]
    movdqa  xmm10, [r8+320+32]
    movdqa  xmm11, [r8+320+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+320],    xmm8   ; Store to pblk
    movdqa [r8+320+16], xmm9
    movdqa [r8+320+32], xmm10
    movdqa [r8+320+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 128
    ;;First, load data 
    movdqa  xmm0,  [rcx+128]    ; load old_dblk
    movdqa  xmm1,  [rcx+128+16]
    movdqa  xmm2,  [rcx+128+32]
    movdqa  xmm3,  [rcx+128+48]
    movdqa  xmm4,  [rdx+128]    ; load new_dblk
    movdqa  xmm5,  [rdx+128+16]
    movdqa  xmm6,  [rdx+128+32]
    movdqa  xmm7,  [rdx+128+48]
    movdqa  xmm8,  [r8+128]     ; load pblk
    movdqa  xmm9,  [r8+128+16]
    movdqa  xmm10, [r8+128+32]
    movdqa  xmm11, [r8+128+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+128],    xmm8   ; Store to pblk
    movdqa [r8+128+16], xmm9
    movdqa [r8+128+32], xmm10
    movdqa [r8+128+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqa  xmm0,  [rcx+256]    ; load old_dblk
    movdqa  xmm1,  [rcx+256+16]
    movdqa  xmm2,  [rcx+256+32]
    movdqa  xmm3,  [rcx+256+48]
    movdqa  xmm4,  [rdx+256]    ; load new_dblk
    movdqa  xmm5,  [rdx+256+16]
    movdqa  xmm6,  [rdx+256+32]
    movdqa  xmm7,  [rdx+256+48]
    movdqa  xmm8,  [r8+256]     ; load pblk
    movdqa  xmm9,  [r8+256+16]
    movdqa  xmm10, [r8+256+32]
    movdqa  xmm11, [r8+256+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+256],    xmm8   ; Store to pblk
    movdqa [r8+256+16], xmm9
    movdqa [r8+256+32], xmm10
    movdqa [r8+256+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqa  xmm0,  [rcx+384]    ; load old_dblk
    movdqa  xmm1,  [rcx+384+16]
    movdqa  xmm2,  [rcx+384+32]
    movdqa  xmm3,  [rcx+384+48]
    movdqa  xmm4,  [rdx+384]    ; load new_dblk
    movdqa  xmm5,  [rdx+384+16]
    movdqa  xmm6,  [rdx+384+32]
    movdqa  xmm7,  [rdx+384+48]
    movdqa  xmm8,  [r8+384]     ; load pblk
    movdqa  xmm9,  [r8+384+16]
    movdqa  xmm10, [r8+384+32]
    movdqa  xmm11, [r8+384+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+384],    xmm8   ; Store to pblk
    movdqa [r8+384+16], xmm9
    movdqa [r8+384+32], xmm10
    movdqa [r8+384+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;First, load data 
    movdqa  xmm0,  [rcx]        ; load old_dblk
    movdqa  xmm1,  [rcx+16]
    movdqa  xmm2,  [rcx+32]
    movdqa  xmm3,  [rcx+48]
    movdqa  xmm4,  [rdx]        ; load new_dblk
    movdqa  xmm5,  [rdx+16]
    movdqa  xmm6,  [rdx+32]
    movdqa  xmm7,  [rdx+48]
    movdqa  xmm8,  [r8]         ; load pblk
    movdqa  xmm9,  [r8+16]
    movdqa  xmm10, [r8+32]
    movdqa  xmm11, [r8+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8],    xmm8       ; Store to pblk
    movdqa [r8+16], xmm9
    movdqa [r8+32], xmm10
    movdqa [r8+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7
       
  fold_x:
    ;; Fold csum of old_dblk which is in xmm13 using xmm0 as scratch 
    movdqa  xmm0,  xmm13        ; 128-bit checksum
    psrldq  xmm13, 8            ; high 64-bits
    pxor    xmm13, xmm0         ; xor hi/lo
    movdqa  xmm0,  xmm13        ; 64-bit checksum
    psrldq  xmm13, 4            ; high 32-bits
    pxor    xmm13, xmm0         ; xor hi/lo
    movd    dword ptr [r9],  xmm13        ; ret 32-bit checksum 

    ;; Fold csum of old_dblk which is in xmm14 using xmm1 as scratch 
    movdqa  xmm1,  xmm14        ; 128-bit checksum
    psrldq  xmm14, 8            ; high 64-bits
    pxor    xmm14, xmm1         ; xor hi/lo
    movdqa  xmm1,  xmm14        ; 64-bit checksum
    psrldq  xmm14, 4            ; high 32-bits
    pxor    xmm14, xmm1         ; xor hi/lo
    movd    dword ptr [r9+4],xmm14        ; ret 32-bit checksum  
    ret     0

 alignment_8:   
    pxor    xmm13, xmm13        ; xmm13 = 0
    pxor    xmm14, xmm14        ; xmm14 = 0
    ;;Since blocks are 8-byte aligned, compute on first 8 bytes and last 8 bytes of block  
    ;;Load first and last 8 bytes    
    movq    xmm0, qword ptr [rcx]
    movq    xmm1, qword ptr [rcx+504]  
    movq    xmm4, qword ptr [rdx]
    movq    xmm5, qword ptr [rdx+504]
    movq    xmm8, qword ptr [r8]
    movq    xmm9, qword ptr [r8+504]
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk 
    pxor    xmm9,  xmm1    
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5  
    movq    qword ptr [r8],  xmm8         ; Store to pblk
    movq    qword ptr [r8+504], xmm9      
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13
    pxor    xmm13, xmm1 
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14
    pxor    xmm14, xmm5 

    prefetchnta [rcx+192]       ;old_dblk
    prefetchnta [rcx+ 64]
    prefetchnta [rcx+448]
    prefetchnta [rcx+320]
    prefetchnta [rcx+128]
    prefetchnta [rcx+256]
    prefetchnta [rcx+384]
    prefetchnta [rcx]    
    prefetchnta [rdx+192]       ;new_dblk
    prefetchnta [rdx+ 64]
    prefetchnta [rdx+448]
    prefetchnta [rdx+320]
    prefetchnta [rdx+128]
    prefetchnta [rdx+256]
    prefetchnta [rdx+384]
    prefetchnta [rdx]       
    prefetchnta [r8+192]        ;pblk
    prefetchnta [r8+ 64]
    prefetchnta [r8+448]
    prefetchnta [r8+320]
    prefetchnta [r8+128]
    prefetchnta [r8+256]
    prefetchnta [r8+384]
    prefetchnta [r8]
    pause
    ;;Since blocks are 8-byte aligned, increment pointers by 8 to make blocks 16-byte aligned
    add     rcx,   8
    add     rdx,   8
    add     r8,    8
    ;;Perform computation on 64 byte chunk starting at offset 192
    ;;First, load data 
    movdqa  xmm0,  [rcx+192]    ; load old_dblk
    movdqa  xmm1,  [rcx+192+16]
    movdqa  xmm2,  [rcx+192+32]
    movdqa  xmm3,  [rcx+192+48]
    movdqa  xmm4,  [rdx+192]    ; load new_dblk
    movdqa  xmm5,  [rdx+192+16]
    movdqa  xmm6,  [rdx+192+32]
    movdqa  xmm7,  [rdx+192+48]
    movdqa  xmm8,  [r8+192]     ; load pblk
    movdqa  xmm9,  [r8+192+16]
    movdqa  xmm10, [r8+192+32]
    movdqa  xmm11, [r8+192+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+192],    xmm8   ; Store to pblk
    movdqa [r8+192+16], xmm9
    movdqa [r8+192+32], xmm10
    movdqa [r8+192+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 64
    ;;First, load data 
    movdqa  xmm0,  [rcx+64]     ; load old_dblk
    movdqa  xmm1,  [rcx+64+16]
    movdqa  xmm2,  [rcx+64+32]
    movdqa  xmm3,  [rcx+64+48]
    movdqa  xmm4,  [rdx+64]     ; load new_dblk
    movdqa  xmm5,  [rdx+64+16]
    movdqa  xmm6,  [rdx+64+32]
    movdqa  xmm7,  [rdx+64+48]
    movdqa  xmm8,  [r8+64]      ; load pblk
    movdqa  xmm9,  [r8+64+16]
    movdqa  xmm10, [r8+64+32]
    movdqa  xmm11, [r8+64+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+64],    xmm8    ; Store to pblk
    movdqa [r8+64+16], xmm9
    movdqa [r8+64+32], xmm10
    movdqa [r8+64+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;In this chunk we do not need to compute for offset 448+48, since its already handled      
    ;;First, load data 
    movdqa  xmm0,  [rcx+448]    ; load old_dblk
    movdqa  xmm1,  [rcx+448+16]
    movdqa  xmm2,  [rcx+448+32]
    ;;movdqa  xmm3,  [rcx+448+48] ;;This case handled before prefetches 
    movdqa  xmm4,  [rdx+448]    ; load new_dblk
    movdqa  xmm5,  [rdx+448+16]
    movdqa  xmm6,  [rdx+448+32]
    ;;movdqa  xmm7,  [rdx+448+48] ;;This case handled before prefetches
    movdqa  xmm8,  [r8+448]     ; load pblk
    movdqa  xmm9,  [r8+448+16]
    movdqa  xmm10, [r8+448+32]
    ;;movdqa  xmm11, [r8+448+48]  ;;This case handled before prefetches  
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    ;;pxor    xmm11, xmm3         ;;This case handled before prefetches
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    ;;pxor    xmm11, xmm7         ;;This case handled before prefetches
    movdqa [r8+448],    xmm8   ; Store to pblk
    movdqa [r8+448+16], xmm9
    movdqa [r8+448+32], xmm10
    ;;movdqa [r8+448+48], xmm11  ;;This case handled before prefetches
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    ;;pxor    xmm13, xmm3         ;;This case handled before prefetches
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    ;;pxor    xmm14, xmm7         ;;This case handled before prefetches

    ;;Perform computation on 64 byte chunk starting at offset 320
    ;;First, load data 
    movdqa  xmm0,  [rcx+320]    ; load old_dblk
    movdqa  xmm1,  [rcx+320+16]
    movdqa  xmm2,  [rcx+320+32]
    movdqa  xmm3,  [rcx+320+48]
    movdqa  xmm4,  [rdx+320]    ; load new_dblk
    movdqa  xmm5,  [rdx+320+16]
    movdqa  xmm6,  [rdx+320+32]
    movdqa  xmm7,  [rdx+320+48]
    movdqa  xmm8,  [r8+320]     ; load pblk
    movdqa  xmm9,  [r8+320+16]
    movdqa  xmm10, [r8+320+32]
    movdqa  xmm11, [r8+320+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+320],    xmm8   ; Store to pblk
    movdqa [r8+320+16], xmm9
    movdqa [r8+320+32], xmm10
    movdqa [r8+320+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 128
    ;;First, load data 
    movdqa  xmm0,  [rcx+128]    ; load old_dblk
    movdqa  xmm1,  [rcx+128+16]
    movdqa  xmm2,  [rcx+128+32]
    movdqa  xmm3,  [rcx+128+48]
    movdqa  xmm4,  [rdx+128]    ; load new_dblk
    movdqa  xmm5,  [rdx+128+16]
    movdqa  xmm6,  [rdx+128+32]
    movdqa  xmm7,  [rdx+128+48]
    movdqa  xmm8,  [r8+128]     ; load pblk
    movdqa  xmm9,  [r8+128+16]
    movdqa  xmm10, [r8+128+32]
    movdqa  xmm11, [r8+128+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+128],    xmm8   ; Store to pblk
    movdqa [r8+128+16], xmm9
    movdqa [r8+128+32], xmm10
    movdqa [r8+128+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqa  xmm0,  [rcx+256]    ; load old_dblk
    movdqa  xmm1,  [rcx+256+16]
    movdqa  xmm2,  [rcx+256+32]
    movdqa  xmm3,  [rcx+256+48]
    movdqa  xmm4,  [rdx+256]    ; load new_dblk
    movdqa  xmm5,  [rdx+256+16]
    movdqa  xmm6,  [rdx+256+32]
    movdqa  xmm7,  [rdx+256+48]
    movdqa  xmm8,  [r8+256]     ; load pblk
    movdqa  xmm9,  [r8+256+16]
    movdqa  xmm10, [r8+256+32]
    movdqa  xmm11, [r8+256+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+256],    xmm8   ; Store to pblk
    movdqa [r8+256+16], xmm9
    movdqa [r8+256+32], xmm10
    movdqa [r8+256+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 256
    ;;First, load data 
    movdqa  xmm0,  [rcx+384]    ; load old_dblk
    movdqa  xmm1,  [rcx+384+16]
    movdqa  xmm2,  [rcx+384+32]
    movdqa  xmm3,  [rcx+384+48]
    movdqa  xmm4,  [rdx+384]    ; load new_dblk
    movdqa  xmm5,  [rdx+384+16]
    movdqa  xmm6,  [rdx+384+32]
    movdqa  xmm7,  [rdx+384+48]
    movdqa  xmm8,  [r8+384]     ; load pblk
    movdqa  xmm9,  [r8+384+16]
    movdqa  xmm10, [r8+384+32]
    movdqa  xmm11, [r8+384+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8+384],    xmm8   ; Store to pblk
    movdqa [r8+384+16], xmm9
    movdqa [r8+384+32], xmm10
    movdqa [r8+384+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4           ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7

    ;;Perform computation on 64 byte chunk starting at offset 448
    ;;First, load data 
    movdqa  xmm0,  [rcx]        ; load old_dblk
    movdqa  xmm1,  [rcx+16]
    movdqa  xmm2,  [rcx+32]
    movdqa  xmm3,  [rcx+48]
    movdqa  xmm4,  [rdx]        ; load new_dblk
    movdqa  xmm5,  [rdx+16]
    movdqa  xmm6,  [rdx+32]
    movdqa  xmm7,  [rdx+48]
    movdqa  xmm8,  [r8]         ; load pblk
    movdqa  xmm9,  [r8+16]
    movdqa  xmm10, [r8+32]
    movdqa  xmm11, [r8+48] 
    ;;Next, old_dblk ^ new_dblk ^ pblk - > pblk
    pxor    xmm8,  xmm0         ; XOR old_dblk with pblk
    pxor    xmm9,  xmm1
    pxor    xmm10, xmm2
    pxor    xmm11, xmm3
    pxor    xmm8,  xmm4         ; XOR new_dblk with pblk
    pxor    xmm9,  xmm5
    pxor    xmm10, xmm6
    pxor    xmm11, xmm7 
    movdqa [r8],    xmm8       ; Store to pblk
    movdqa [r8+16], xmm9
    movdqa [r8+32], xmm10
    movdqa [r8+48], xmm11
    ;;Next, accumulate checksums into xmm13, xmm14
    pxor    xmm13, xmm0         ; accumulate csum of old_dblk into xmm13 
    pxor    xmm13, xmm1 
    pxor    xmm13, xmm2
    pxor    xmm13, xmm3
    pxor    xmm14, xmm4         ; accumulate csum of new_dblk into xmm14 
    pxor    xmm14, xmm5 
    pxor    xmm14, xmm6
    pxor    xmm14, xmm7 
    ;; Now jump to fold and return   
    jmp     fold_x    
    

;; Below assembly corresponds to the following C code.
;;  return (lba ^ lba > 16 ^ lba > 32 ^ lba > 48);
;;
;; UINT_16E fbe_xor_generate_lba_stamp_mmx(ULONGLONG lba);
;;  rcx  lba
;; returns: ax = 16-bit LBA stamp
;; Note: mmx registers are not used
;; History: Feb-04-08 - Created - V. Vankamamidi
ALIGN 16
GLOBAL fbe_xor_generate_lba_stamp_mmx
fbe_xor_generate_lba_stamp_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    mov     rax, rcx
    shr     rcx, 10h
    xor     rax, rcx
    shr     rcx, 10h
    xor     rax, rcx
    shr     rcx, 10h
    xor     rax, rcx
    ret     0                   

;; Below assembly corresponds to the following C code.
;; stamp = (lba ^ lba > 16 ^ lba > 32 ^ lba > 48);
;; return ( (lba_stamp == 0) ? ((lba_stamp = stamp), TRUE) : (lba_stamp == stamp) );
;;
;; BOOL fbe_xor_is_valid_lba_stamp_mmx (UINT_16E *lba_stamp, ULONGLONG lba);
;; rc   lba_stamp
;; rdx  lba
;; returns: eax = 0 (False) or 1 (True)
;; Note: mmx registers are not used
;; History: Feb-04-08 - Created - V. Vankamamidi
ALIGN 16
GLOBAL fbe_xor_is_valid_lba_stamp_mmx
fbe_xor_is_valid_lba_stamp_mmx PROC 
%ifdef __SAFE__
    mov    r8, rdx ;; get arg3 into right register
    mov    r9, rcx ;; get arg4 into right register
    mov    rcx, rdi ;; get arg1 into right register
    mov    rdx, rsi ;; get arg2 into right register
;;calling convention differences
%endif ;; __SAFE__ SAFEMESS
    mov     rax, rdx            ; Generate LBA Stamp 
    shr     rdx, 10h
    xor     rax, rdx
    shr     rdx, 10h
    xor     rax, rdx
    shr     rdx, 10h
    xor     rax, rdx
    mov     dx, [rcx]           ; Validate passed in LBA stamp with generated stamp
    test    dx, dx
    jne     compare_q
    mov     [rcx], ax
    mov     eax, 1
    ret     0
  compare_q:
    cmp     ax, dx 
    jne     retfalse
    mov     eax, 1
    ret     0
  retfalse:
    mov     eax, 0      
    ret     0                   

%endif
END
