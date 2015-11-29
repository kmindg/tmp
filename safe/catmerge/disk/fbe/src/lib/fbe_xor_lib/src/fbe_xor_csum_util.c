/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 ***************************************************************************
 *
 * DESCRIPTION:
 *     This file contains mmx and non-mmx checksum utility functions used 
 *     by the XOR driver.
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "fbe/fbe_types.h"
#include "fbe_xor_private.h"
#include "fbe_xor_csum_util.h"
#include "fbe/fbe_library_interface.h"

#if 0
/* 
 * Pentium-II/III: MMX optimized fbe_sector_t xor/checksum routines
 * mmx and C unrolled xor_sector for XorBuffer()
 * Greg Schaffer 99/07/23
 */
fbe_u32_t fbe_UseMMX = MMX_UNKNOWN;  // Test for MMX

#ifndef _AMD64_ // SSE2 assembler in xor_csum_a64.asm
fbe_u32_t fbe_mmx_cpuid(void)
{
	__asm {
		push ebx			; save & restore regs
		push ecx
		push edx
		mov eax, 1			; feature flags
		_emit 0x0f			; cpuid
		_emit 0xa2
		xor eax, eax		        ; clear
		test edx, 0x00800000            ; bit 23: MMX
		setnz al
		pop edx
		pop ecx
		pop ebx
	}
}

// mmx optimize fbe_sector_t (checksum ^= *srcptr++)
fbe_u32_t fbe_xor_calc_csum_mmx(const fbe_u32_t *srcptr)
{
	__asm {
		prefetchnta srcptr
		push ebx
		mov ebx, srcptr

		prefetchnta [ebx+128]

		pxor mm0, mm0	; mm0 checksum = 0
		prefetchnta [ebx+256]
//              We have prefetched 2 ahead. Therefore we will
//              fetch off of the end.  This is intentional,
//              size it sets us up for the next fbe_sector_t.

		pxor mm0, [ebx] ; 8-byte xor 0 - 127
		pxor mm0, [ebx+8]
		pxor mm0, [ebx+16]
		pxor mm0, [ebx+24]
		pxor mm0, [ebx+32]
		pxor mm0, [ebx+40]
		pxor mm0, [ebx+48]
		pxor mm0, [ebx+56]
		pxor mm0, [ebx+64]
		pxor mm0, [ebx+64+8]
		pxor mm0, [ebx+64+16]
		pxor mm0, [ebx+64+24]
		pxor mm0, [ebx+64+32]
		pxor mm0, [ebx+64+40]
		pxor mm0, [ebx+64+48]
		pxor mm0, [ebx+64+56]
		add ebx, 128
        prefetchnta [ebx+256]   ; [srcptr+384]
		pxor mm0, [ebx] ; 8-byte xor 128 - 255
		pxor mm0, [ebx+8]
		pxor mm0, [ebx+16]
		pxor mm0, [ebx+24]
		pxor mm0, [ebx+32]
		pxor mm0, [ebx+40]
		pxor mm0, [ebx+48]
		pxor mm0, [ebx+56]
		pxor mm0, [ebx+64]
		pxor mm0, [ebx+64+8]
		pxor mm0, [ebx+64+16]
		pxor mm0, [ebx+64+24]
		pxor mm0, [ebx+64+32]
		pxor mm0, [ebx+64+40]
		pxor mm0, [ebx+64+48]
		pxor mm0, [ebx+64+56]
		add ebx, 128
        prefetchnta [ebx+256]   ; [srcptr+512] -- prefetch next fbe_sector_t (if contiguous)
		pxor mm0, [ebx] ; 8-byte xor 256 - 383
		pxor mm0, [ebx+8]
		pxor mm0, [ebx+16]
		pxor mm0, [ebx+24]
		pxor mm0, [ebx+32]
		pxor mm0, [ebx+40]
		pxor mm0, [ebx+48]
		pxor mm0, [ebx+56]
		pxor mm0, [ebx+64]
		pxor mm0, [ebx+64+8]
		pxor mm0, [ebx+64+16]
		pxor mm0, [ebx+64+24]
		pxor mm0, [ebx+64+32]
		pxor mm0, [ebx+64+40]
		pxor mm0, [ebx+64+48]
		pxor mm0, [ebx+64+56]
		add ebx, 128
		pxor mm0, [ebx] ; 8-byte xor 384 - 511
		pxor mm0, [ebx+8]
		pxor mm0, [ebx+16]
		pxor mm0, [ebx+24]
		pxor mm0, [ebx+32]
		pxor mm0, [ebx+40]
		pxor mm0, [ebx+48]
		pxor mm0, [ebx+56]
		pxor mm0, [ebx+64]
		pxor mm0, [ebx+64+8]
		pxor mm0, [ebx+64+16]
		pxor mm0, [ebx+64+24]
		pxor mm0, [ebx+64+32]
		pxor mm0, [ebx+64+40]
		pxor mm0, [ebx+64+48]
		pxor mm0, [ebx+64+56]

		movq mm5, mm0	; 64bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor hi/lo
		movd eax, mm0	; ret 32bit checksum 
		emms			; MMX end
		pop ebx
	}
}

// mmx optimize fbe_sector_t (checksum ^= *srcptr, *tgtptr++ = *srcptr++)
fbe_u32_t fbe_xor_calc_csum_and_cpy_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr)
{
	__asm {
		prefetchnta srcptr
		push ebx
		push ecx
		mov ebx, srcptr;
		mov ecx, tgtptr

		prefetchnta [ebx+128]

        pxor mm0, mm0	; mm0 checksum = 0
		mov eax, 16		; 16 loops of 32 bytes
	do_loop:
//              We have prefetched 2 ahead. Therefore we will
//              fetch off of the end.  This is intentional,
//              size it sets us up for the next fbe_sector_t.

		prefetchnta [ebx+256]

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		movq [ecx], mm1	; copy target
		pxor mm0, mm1	; running checksum
		movq [ecx+8], mm2
		pxor mm0, mm2
		movq [ecx+16], mm3
		pxor mm0, mm3
		movq [ecx+24], mm4
		pxor mm0, mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		dec eax
		jnz do_loop

        movq mm5, mm0	; 64bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor hi/lo
		movd eax, mm0	; ret 32bit checksum 
		emms			; MMX end
		pop ecx
		pop ebx
	}
}

// mmx optimize with prefetch fbe_sector_t (checksum ^= *srcptr, *tgtptr++ ^= *srcptr++)
fbe_u32_t fbe_xor_calc_csum_and_xor_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr)
{
	__asm {
		push ebx
		push ecx
		mov ebx, srcptr
		mov ecx, tgtptr
	       
		prefetchnta [ebx]

		prefetchnta [ebx+128]

		prefetchnta [ecx]

		prefetchnta [ecx+128]


		pxor mm0, mm0		; mm0 = 0 = checksum = 0
		mov eax, 16			; 16 loops of 32 bytes
	do_loop:
//             We have prefetched 2 ahead. Therefore we will
//              fetch off of the end.  This is intentional,
//              size it sets us up for the next fbe_sector_t.

		prefetchnta [ebx+256]

		prefetchnta [ecx+256]

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		pxor mm0, mm1	; running src checksum
		pxor mm1, [ecx]	; xor target
		pxor mm0, mm2
		pxor mm2, [ecx+8]
		pxor mm0, mm3
		pxor mm3, [ecx+16]
		pxor mm0, mm4
		pxor mm4, [ecx+24]
		movq [ecx], mm1	; store xor target
		movq [ecx+8], mm2
		movq [ecx+16], mm3
		movq [ecx+24], mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		dec eax
		jnz do_loop






		movq mm5, mm0		; 64-bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor high/low
		movd eax, mm0	; return 32bit checksum 
		emms			; MMX end
		pop ecx
		pop ebx
	}
}

/* mmx optimize fbe_sector_t (checksum ^= *srcptr, 
 *                      *tgtptr2++ = *srcptr++, 
 *                      *tgtptr1++ = *srcptr++)
 */
fbe_u32_t fbe_xor_calc_csum_and_cpy_vault_mmx(const fbe_u32_t *srcptr, 
                                        fbe_u32_t *tgtptr1, 
                                        fbe_u32_t *tgtptr2)
{
	__asm {
		push ebx
		push ecx
		push edx
		mov ebx, srcptr  ;
		mov ecx, tgtptr1
        mov edx, tgtptr2

 //		prefetchnta [ebx]
		_emit 0x0f
		_emit 0x18
		_emit 0x03
//		prefetchnta [ebx+32]
		_emit 0x0f
		_emit 0x18
		_emit 0x43
		_emit 0x20

        pxor mm0, mm0	; mm0 checksum = 0
		mov eax, 14		; 14 loops of 32 bytes
	do_loop:
//		prefetchnta [ebx+64]
		_emit 0x0f
		_emit 0x18
		_emit 0x43
		_emit 0x40

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		movq [ecx], mm1	; copy target1
		movq [edx], mm1	; copy target2
		pxor mm0, mm1	; running checksum
		movq [ecx+8], mm2
		movq [edx+8], mm2
		pxor mm0, mm2
		movq [ecx+16], mm3
		movq [edx+16], mm3
		pxor mm0, mm3
		movq [ecx+24], mm4
		movq [edx+24], mm4
		pxor mm0, mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		add edx, 32
		dec eax
		jnz do_loop

		; the 15th 32 bytes
		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		movq [ecx], mm1	; copy target1
		movq [edx], mm1	; copy target2
		pxor mm0, mm1	; running checksum
		movq [ecx+8], mm2
		movq [edx+8], mm2
		pxor mm0, mm2
		movq [ecx+16], mm3
		movq [edx+16], mm3
		pxor mm0, mm3
		movq [ecx+24], mm4
		movq [edx+24], mm4
		pxor mm0, mm4

		add ebx, 32
		add ecx, 32
		add edx, 32

		; the 16th 32 bytes
		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		movq [ecx], mm1	; copy target1
		movq [edx], mm1	; copy target2
		pxor mm0, mm1	; running checksum
		movq [ecx+8], mm2
		movq [edx+8], mm2
		pxor mm0, mm2
		movq [ecx+16], mm3
		movq [edx+16], mm3
		pxor mm0, mm3
		movq [ecx+24], mm4
		movq [edx+24], mm4
		pxor mm0, mm4

        movq mm5, mm0	; 64bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor hi/lo
		movd eax, mm0	; ret 32bit checksum 
		emms			; MMX end
		pop edx
		pop ecx
		pop ebx
	}
}

/* mmx optimize with prefetch fbe_sector_t 
 *      (checksum ^= *srcptr, 
 *       *tgtptr1++ = *srcptr++, 
 *       *tgtptr2++ ^= *srcptr++)
 */
fbe_u32_t fbe_xor_calc_csum_and_xor_vault_mmx(const fbe_u32_t *srcptr, 
                                        fbe_u32_t *tgtptr1, 
                                        fbe_u32_t *tgtptr2)
{
	__asm {
		push ebx
		push ecx
		push edx
		mov ebx, srcptr
		mov ecx, tgtptr2
		mov edx, tgtptr1
	       
		; prefetchnta [ebx]
		_emit 0x0f
		_emit 0x18
		_emit 0x03

		; prefetchnta [ebx+32]
		_emit 0x0f
		_emit 0x18
		_emit 0x43
		_emit 0x20

		; prefetchnta [ecx]
		_emit 0x0f
		_emit 0x18
		_emit 0x01

		;prefetchnta [ecx+32]
		_emit 0x0f
		_emit 0x18
		_emit 0x41
		_emit 0x20

		pxor mm0, mm0		; mm0 = 0 = checksum = 0
		mov eax, 14			; 14 loops of 32 bytes
	do_loop:

		; prefetchnta [ebx+64]
		_emit 0x0f
		_emit 0x18
		_emit 0x43
		_emit 0x40

		; prefetchnta[ecx+64]
		_emit 0x0f
		_emit 0x18
		_emit 0x41
		_emit 0x40

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		pxor mm0, mm1	; running src checksum
		movq [edx], mm1	; copy target2
		pxor mm1, [ecx]	; xor target
		pxor mm0, mm2
		movq [edx+8], mm2
		pxor mm2, [ecx+8]
		pxor mm0, mm3
		movq [edx+16], mm3
		pxor mm3, [ecx+16]
		pxor mm0, mm4
		movq [edx+24], mm4
		pxor mm4, [ecx+24]
		movq [ecx], mm1	; store xor target
		movq [ecx+8], mm2
		movq [ecx+16], mm3
		movq [ecx+24], mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		add edx, 32
		dec eax
		jnz do_loop

		;15th 32 bytes
		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		pxor mm0, mm1	; running src checksum
		movq [edx], mm1	; copy target2
		pxor mm1, [ecx]	; xor target
		pxor mm0, mm2
		movq [edx+8], mm2
		pxor mm2, [ecx+8]
		pxor mm0, mm3
		movq [edx+16], mm3
		pxor mm3, [ecx+16]
		pxor mm0, mm4
		movq [edx+24], mm4
		pxor mm4, [ecx+24]
		movq [ecx], mm1	; store xor target
		movq [ecx+8], mm2
		movq [ecx+16], mm3
		movq [ecx+24], mm4		


		add ebx, 32
		add ecx, 32
		add edx, 32

		;16th 32 bytes
		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		pxor mm0, mm1	; running src checksum
		movq [edx], mm1	; copy target2
		pxor mm1, [ecx]	; xor target
		pxor mm0, mm2
		movq [edx+8], mm2
		pxor mm2, [ecx+8]
		pxor mm0, mm3
		movq [edx+16], mm3
		pxor mm3, [ecx+16]
		pxor mm0, mm4
		movq [edx+24], mm4
		pxor mm4, [ecx+24]
		movq [ecx], mm1	; store xor target
		movq [ecx+8], mm2
		movq [ecx+16], mm3
		movq [ecx+24], mm4		


		movq mm5, mm0		; 64-bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor high/low
		movd eax, mm0	; return 32bit checksum 
		emms			; MMX end
		pop edx
		pop ecx
		pop ebx
	}
}

/* The below assembly corresponds to the following C code.
 * for all words in fbe_sector_t {
 * checksum ^= *srcptr, comparison |= (*tgtptr++ ^ *srcptr++);
 * }
 *
 *   *cmpptr = ((comparison == 0) ? FBE_XOR_COMPARISON_RESULT_SAME_DATA : FBE_XOR_COMPARISON_RESULT_DIFF_DATA);
 */
fbe_u32_t fbe_xor_calc_csum_and_cmp_mmx(const fbe_u32_t * srcptr,
                          const fbe_u32_t * tgtptr,
                          fbe_xor_comparison_result_t * cmpptr)
{
	__asm {
		push ebx
		push ecx
		push edx
		mov ebx, srcptr
		mov ecx, tgtptr
		mov edx, cmpptr

		prefetchnta [ebx]

		prefetchnta [ebx+128]
		prefetchnta [ebx+256]

		prefetchnta [ecx]

		prefetchnta [ecx+128]		
		prefetchnta [ecx+256]
		prefetchnta [ebx+384]

		pxor mm0, mm0		; mm0 = 0 = checksum = 0
		pxor mm6, mm6       ; mm5 = 0 = comparison = 0
		mov eax, 16			; 16 loops of 32 bytes
	do_loop:
//             We have prefetched 4 ahead. Therefore we will
//              fetch off of the end.  This is intentional,
//              size it sets us up for the next fbe_sector_t.

		prefetchnta [ecx+512]

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		pxor mm0, mm1	; running src checksum
		pxor mm1, [ecx]	; xor target (src ^ tgt)
		pxor mm0, mm2
		pxor mm2, [ecx+8]
		pxor mm0, mm3
		pxor mm3, [ecx+16]
		pxor mm0, mm4
		pxor mm4, [ecx+24]
		por  mm6, mm1   ; cumulate xors comparison |= (src ^ tgt)
		por  mm6, mm2
		por  mm6, mm3
		por  mm6, mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		dec eax
		jnz do_loop

		movq mm5, mm0		; 64-bit checksum
		psrlq mm0, 32	; high 32bits
		pxor mm0, mm5	; xor high/low
		movd eax, mm0	; return 32bit checksum 
		mov ebx, FBE_XOR_COMPARISON_RESULT_DIFF_DATA    ; assume miss match
		movd ecx, mm6   ; get 32 bits
		test ecx, ecx
		jnz done        ; part different
		psrlq mm6, 32
		movd ecx, mm6
		test ecx, ecx
		jnz done
        mov ebx, FBE_XOR_COMPARISON_RESULT_SAME_DATA
done:
		mov [edx], ebx ;
		emms			; MMX end
		pop edx
		pop ecx
		pop ebx
	}

}

/****************************************************************
 *                     fbe_xor_cpy_only_mmx
 ****************************************************************
 *
 * DESCRIPTION:
 *  This function copies only data from using MMX.
 *
 *  PARAMETERS:
 *   srcptr,     [I]  - ptr to source buffer
 *   tgtptr,     [I]  - ptr to destination buffer 
 *
 * RETURN VALUES/ERRORS:
 *   none
 *
 *  NOTES:
 *
 * HISTORY:
 *  1/15/07: JHP - Created.
 *
 ****************************************************************/
void fbe_xor_cpy_only_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr)
{
	__asm {
		prefetchnta srcptr
		push ebx
		push ecx
		mov ebx, srcptr;
		mov ecx, tgtptr

        prefetchnta [ebx+128]

		mov eax, 16		; 16 loops of 32 bytes
	do_loop:
//              We have prefetched 2 ahead. Therefore we will
//              fetch off of the end.  This is intentional,
//              size it sets us up for the next fbe_sector_t.

		prefetchnta [ebx+256]

		movq mm1, [ebx]	; load src
		movq mm2, [ebx+8]
		movq mm3, [ebx+16]
		movq mm4, [ebx+24]
		movq [ecx], mm1	; copy target
		movq [ecx+8], mm2
		movq [ecx+16], mm3
		movq [ecx+24], mm4

		; while (--loop != 0)
		add ebx, 32
		add ecx, 32
		dec eax
		jnz do_loop

		emms			; MMX end
		pop ecx
		pop ebx
	}
}
#else

// MMX-SSE2 assembler in xor_csum_a64.asm
extern fbe_u32_t fbe_mmx_cpuid(void);
extern fbe_u32_t fbe_xor_calc_csum_mmx(const fbe_u32_t *srcptr);
extern fbe_u32_t fbe_xor_calc_csum_and_cpy_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr);
extern fbe_u32_t fbe_xor_calc_csum_and_cpy_sequential_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr);
extern fbe_u32_t fbe_xor_calc_csum_and_xor_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr);
extern fbe_u32_t fbe_xor_calc_csum_and_cpy_vault_mmx(const fbe_u32_t *srcptr, 
                                               fbe_u32_t *tgtptr1, 
                                               fbe_u32_t *tgtptr2);
extern fbe_u32_t fbe_xor_calc_csum_and_xor_vault_mmx(const fbe_u32_t *srcptr, 
                                               fbe_u32_t *tgtptr1, 
                                               fbe_u32_t *tgtptr2);
extern fbe_u32_t fbe_xor_calc_csum_and_cmp_mmx(const fbe_u32_t * srcptr,
    const fbe_u32_t * tgtptr,
    fbe_xor_comparison_result_t * cmpptr);

#endif

#ifdef _AMD64_
/* The below functions were only introduced for AMD64.
 */
extern fbe_u32_t fbe_xor_calc_csum_and_cpy_to_temp_mmx(const fbe_u32_t *srcptr, 
                                                 fbe_u32_t *tempptr);
extern fbe_u32_t fbe_xor_calc_csum_and_xor_to_temp_mmx(const fbe_u32_t *srcptr, 
                                                 fbe_u32_t *tempptr);
extern fbe_u32_t fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx(const fbe_u32_t *srcptr, 
                                                           const fbe_u32_t *tempptr, 
                                                           fbe_u32_t *trgptr);
extern void fbe_xor_468_calc_csum_and_update_parity_mmx(const fbe_u32_t * old_dblk, 
                                                    const fbe_u32_t * new_dblk, 
                                                    fbe_u32_t * pblk, 
                                                    fbe_u32_t * csum);
extern fbe_u16_t fbe_xor_generate_lba_stamp_mmx(ULONGLONG lba);
extern fbe_bool_t fbe_xor_is_valid_lba_stamp_mmx(fbe_u16_t *lba_stamp, ULONGLONG lba);
#endif

/*****************************************************
 * LOCAL FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/

fbe_u32_t fbe_xor_calc_csum_non_mmx(const fbe_u32_t * srcptr);

fbe_u32_t fbe_xor_calc_csum_and_cmp_non_mmx(const fbe_u32_t * srcptr,
                            const fbe_u32_t * tgtptr,
                            fbe_xor_comparison_result_t * cmpptr);
fbe_u32_t fbe_xor_calc_csum_and_cpy_non_mmx(const fbe_u32_t * srcptr,
                            fbe_u32_t * tgtptr);

static fbe_u16_t fbe_xor_generate_lba_stamp_non_mmx(ULONGLONG lba);
static fbe_bool_t fbe_xor_is_valid_lba_stamp_non_mmx(fbe_u16_t *lba_stamp, ULONGLONG lba);

fbe_u32_t fbe_xor_calc_csum_and_cpy_vault_non_mmx(const fbe_u32_t * srcptr,
                                            fbe_u32_t * tgtptr,
                                            fbe_u32_t * tgtptr2);
fbe_u32_t fbe_xor_calc_csum_and_cpy_vault_mmx(const fbe_u32_t * srcptr,
                                            fbe_u32_t * tgtptr,
                                            fbe_u32_t * tgtptr2);

fbe_u32_t fbe_xor_calc_csum_and_xor_vault_non_mmx(const fbe_u32_t * srcptr,
                                            fbe_u32_t * tgtptr,
                                            fbe_u32_t * tgtptr2);

fbe_u32_t fbe_xor_calc_csum_and_xor_vault_mmx(const fbe_u32_t * srcptr,
                                        fbe_u32_t * tgtptr,
                                        fbe_u32_t * tgtptr2);

fbe_u32_t fbe_xor_calc_csum_and_xor_non_mmx(const fbe_u32_t * srcptr,
fbe_u32_t * tgtptr);

fbe_u32_t fbe_xor_calc_csum_and_xor_with_temp_and_cpy_non_mmx(const fbe_u32_t *srcptr, 
                                                              const fbe_u32_t *tempptr, 
                                                              fbe_u32_t *trgptr);

void fbe_xor_468_calc_csum_and_update_parity_non_mmx(const fbe_u32_t * old_dblk, 
                                                       const fbe_u32_t * new_dblk, 
                                                       fbe_u32_t * pblk, 
                                                       fbe_u32_t * csum);

fbe_u16_t fbe_xor_cook_fixd_csum(fbe_u32_t rawcsum,
  fbe_u32_t seed);

fbe_u32_t (*fbe_xor_calc_csum) (const fbe_u32_t * srcptr) 
     = fbe_xor_calc_csum_non_mmx;

fbe_u32_t (*fbe_xor_calc_csum_and_cmp) (const fbe_u32_t * srcptr,
                           const fbe_u32_t * tgtptr,
                           fbe_xor_comparison_result_t * cmpptr)
     = fbe_xor_calc_csum_and_cmp_non_mmx;
    

fbe_u32_t (*fbe_xor_calc_csum_and_cpy) (const fbe_u32_t * srcptr,
                           fbe_u32_t * tgtptr)
     = fbe_xor_calc_csum_and_cpy_non_mmx;

/* This routine copies the block sequentially, and allows there to be
 * some overlap between source and target. 
 */
fbe_u32_t (*fbe_xor_calc_csum_and_cpy_sequential) (const fbe_u32_t * srcptr,
                                                   fbe_u32_t * tgtptr)
     = fbe_xor_calc_csum_and_cpy_non_mmx;

fbe_u32_t (*fbe_xor_calc_csum_and_xor) (const fbe_u32_t * srcptr,
                           fbe_u32_t * tgtptr)
     = fbe_xor_calc_csum_and_xor_non_mmx;


fbe_u32_t (*fbe_xor_calc_csum_and_cpy_vault) (const fbe_u32_t * srcptr,
                                              fbe_u32_t * tgtptr1, 
                                              fbe_u32_t * tgtptr2)
    = fbe_xor_calc_csum_and_cpy_vault_non_mmx;


fbe_u32_t (*fbe_xor_calc_csum_and_xor_vault) (const fbe_u32_t * srcptr,
                                              fbe_u32_t * tgtptr1, 
                                              fbe_u32_t * tgtptr2)
     = fbe_xor_calc_csum_and_xor_vault_non_mmx;

fbe_u32_t (*fbe_xor_calc_csum_and_cpy_to_temp) (const fbe_u32_t *srcptr, fbe_u32_t *tempptr)
     = fbe_xor_calc_csum_and_cpy_non_mmx; 
fbe_u32_t (*fbe_xor_calc_csum_and_xor_to_temp) (const fbe_u32_t *srcptr, fbe_u32_t *tempptr)
     = fbe_xor_calc_csum_and_xor_non_mmx;
fbe_u32_t (*fbe_xor_calc_csum_and_xor_with_temp_and_cpy) (const fbe_u32_t *srcptr, 
                                                          const fbe_u32_t *tempptr, 
                                                          fbe_u32_t *trgptr)
     = fbe_xor_calc_csum_and_xor_with_temp_and_cpy_non_mmx;

void (*fbe_xor_468_calc_csum_and_update_parity)(const fbe_u32_t * old_dblk, 
                                                  const fbe_u32_t * new_dblk, 
                                                  fbe_u32_t * pblk, fbe_u32_t * csum)
     = fbe_xor_468_calc_csum_and_update_parity_non_mmx; 

fbe_u16_t (*fbe_xor_cook_csum) (fbe_u32_t rawcsum, fbe_u32_t seed)
		   = fbe_xor_cook_fixd_csum;

/* LBA Stamp */
fbe_u16_t (*fbe_xor_generate_lba_stamp_local) (ULONGLONG lba)
		   = fbe_xor_generate_lba_stamp_non_mmx;

fbe_bool_t (*fbe_xor_is_valid_lba_stamp_local) (fbe_u16_t *lba_stamp, ULONGLONG lba)
		   = fbe_xor_is_valid_lba_stamp_non_mmx;

/* type of checksum algorithm */
fbe_xor_csum_alg_t fbe_xor_chksum_algorithm = FBE_XOR_CSUM_ALG_FULL_FIXED;

void fbe_xor_cpy_only_non_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr);
void (*fbe_xor_cpy_only)(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr) = fbe_xor_cpy_only_non_mmx;

/***************************************************************************
 *		fbe_xor_calc_csum_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *		17-Sep-98	-SPN-	Created.
 *              01-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_non_mmx(const fbe_u32_t * srcptr)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT(checksum ^= *srcptr++);
    }

    return (checksum);
}

/***************************************************************************
 *		fbe_xor_calc_csum_and_cmp_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm; the
 *		data is compared to the data from a second "target" fbe_sector_t during
 *		the process.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *		tgtptr	- [I]	ptr to first word of target fbe_sector_t data
 *		cmpptr	- [O]	ptr to area for result of comparison
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *		17-Sep-98	-SPN-	Created.
 *              01-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_cmp_non_mmx(const fbe_u32_t * srcptr,
                          const fbe_u32_t * tgtptr,
                          fbe_xor_comparison_result_t * cmpptr)
{
    fbe_u32_t checksum;
    fbe_u32_t comparison;
    fbe_u16_t passcnt;

    checksum = 0u,
        comparison = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr,
                       comparison |= (*tgtptr++ ^ *srcptr++)));
    }

    *cmpptr = ((comparison == 0u) ? FBE_XOR_COMPARISON_RESULT_SAME_DATA : FBE_XOR_COMPARISON_RESULT_DIFF_DATA);

    return (checksum);
}

/***************************************************************************
 *		fbe_xor_calc_csum_and_cpy_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm; the
 *		data is copied to another location during the process.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *		tgtptr	- [O]	ptr to first word of target fbe_sector_t data
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *		17-Sep-98	-SPN-	Created.
 *              01-Aug-01       A. Vankamamidi  Ported to XOR driver
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_cpy_non_mmx(const fbe_u32_t * srcptr,
                          fbe_u32_t * tgtptr)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr,
                       *tgtptr++ = *srcptr++));
    }

    return (checksum);
}

/***************************************************************************
 *		fbe_xor_calc_csum_and_xor_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm; the
 *		data is XORed into the data at another location during the process.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *		tgtptr	- [IO]	ptr to first word of target fbe_sector_t data
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *		17-Sep-98	-SPN-	Created.
 *              01-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_u32_t
fbe_xor_calc_csum_and_xor_non_mmx(const fbe_u32_t * srcptr,
                          fbe_u32_t * tgtptr)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr, *tgtptr++ ^= *srcptr++));
    }

    return (checksum);
}/* fbe_xor_calc_csum_and_xor_non_mmx */

/***************************************************************************
 *      fbe_xor_calc_csum_and_xor_with_temp_and_cpy_non_mmx()
 ***************************************************************************
 *  DESCRIPTION:
 *      This function calculates a "raw" checksum (unseeded, unfolded) for
 *      data within a fbe_sector_t using the "classic" checksum algorithm; the
 *      data is XORed with the data at another location and stored at a
 *      third location.
 *
 *  PARAMETERS:
 *      srcptr  - [I]   ptr to first word of the first source fbe_sector_t data
 *      tempptr - [I]   ptr to first word of the second source fbe_sector_t data       
 *      tgtptr  - [IO]  ptr to first word of target fbe_sector_t data
 *
 *  RETURN VALUE:
 *      The "raw" checksum for the data in the first source fbe_sector_t.
 *
 *
 *  HISTORY:
 *      23-Mar-08   -Vamsi Vankamammidi-    Created.
 *                                 - Ported from fbe_xor_calc_csum_and_xor_non_mmx 
 ***************************************************************************/

fbe_u32_t
fbe_xor_calc_csum_and_xor_with_temp_and_cpy_non_mmx(const fbe_u32_t * srcptr,
                                                const fbe_u32_t * tempptr, 
                                                fbe_u32_t * tgtptr)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr, *tgtptr++ = *tempptr++ ^ *srcptr++));
    }

    return (checksum);
}/* fbe_xor_calc_csum_and_xor_with_temp_and_cpy_non_mmx */

/***************************************************************************
 *      fbe_xor_468_calc_csum_and_update_parity_non_mmx()
 ***************************************************************************
 *  DESCRIPTION:
 *      This function calculates a "raw" checksum (unseeded, unfolded) for
 *      data within three passed in sectors using the "classic" 
 *      checksum algorithm;
 *      data in all three sectors is XORed and stored in the third fbe_sector_t
 *
 *  PARAMETERS:
 *      old_dblk  - [I]   ptr to first word of the old data block fbe_sector_t
 *      new_dblk  - [I]   ptr to first word of the new data block fbe_sector_t       
 *      pblk      - [I]   ptr to first word of the parity block fbe_sector_t
 *      csum      - [I]   ptr to UINT32 buffer for returning checksums of old 
 *                        and new data sectors
 *
 *  RETURN VALUE:
 *      NoneS
 *
 *
 *  HISTORY:
 *      02-APR-08   -Vamsi Vankamammidi-    Created.
 *                                 - Ported from fbe_xor_calc_csum_and_xor_non_mmx 
 ***************************************************************************/

void
fbe_xor_468_calc_csum_and_update_parity_non_mmx(const fbe_u32_t * old_dblk, 
                                            const fbe_u32_t * new_dblk, 
                                            fbe_u32_t * pblk, 
                                            fbe_u32_t * csum)
{
    fbe_u16_t passcnt;

    csum[0] = 0u;
    csum[1] = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((csum[0] ^= *old_dblk,
                    csum[1] ^= *new_dblk, 
                    *pblk++ ^= *old_dblk++ ^ *new_dblk++));
    }

    return;
}/* fbe_xor_468_calc_csum_and_update_parity_non_mmx */

/***************************************************************************
 *		fbe_xor_calc_csum_and_cpy_vault_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm; the
 *		data is copied to another 2 locations during the process.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *		tgtptr1	- [IO]	ptr to first word of target1 fbe_sector_t data
 *                      this is the destination where we are copying
 *		tgtptr2	- [IO]	ptr to first word of target2 fbe_sector_t data
 *                      this is the 2nd destination where we are copying
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *      09/17/01: RPF Created.
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_cpy_vault_non_mmx(const fbe_u32_t * srcptr,
                                            fbe_u32_t * tgtptr1,
                                            fbe_u32_t * tgtptr2)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr,
                       *tgtptr1++ = *srcptr,
                       *tgtptr2++ = *srcptr++));
    }

    return (checksum);
} /* fbe_xor_calc_csum_and_cpy_vault_non_mmx */

/***************************************************************************
 *		fbe_xor_calc_csum_and_xor_vault_non_mmx()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function calculates a "raw" checksum (unseeded, unfolded) for
 *		data within a fbe_sector_t using the "classic" checksum algorithm; the
 *      data is first copied into a target1 location and then the
 *		data is XORed into the data at another target2 location.
 *
 *	PARAMETERS:
 *		srcptr	- [I]	ptr to first word of source fbe_sector_t data
 *		tgtptr1	- [IO]	ptr to first word of target1 fbe_sector_t data
 *                      this is the destination of a copy operation
 *		tgtptr2	- [IO]	ptr to first word of target2 fbe_sector_t data
 *                      this is a parity XOR destination
 *
 *	RETURN VALUE:
 *		The "raw" checksum for the data in the fbe_sector_t.
 *
 *	NOTES:
 *		Both full fbe_sector_t or partial fbe_sector_t (Raid-3 style) checksum
 *		methods are supported.
 *
 *	HISTORY:
 *      09/17/01: RPF Created.
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_xor_vault_non_mmx(const fbe_u32_t * srcptr,
                                            fbe_u32_t * tgtptr1,
                                            fbe_u32_t * tgtptr2)
{
    fbe_u32_t checksum;
    fbe_u16_t passcnt;

    checksum = 0u;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT((checksum ^= *srcptr,
                    *tgtptr1++ = *srcptr, 
                    *tgtptr2++ ^= *srcptr++));
    }

    return (checksum);
} /* fbe_xor_calc_csum_and_xor_vault_non_mmx */

/****************************************************************
 *                     fbe_xor_cpy_only_non_mmx
 ****************************************************************
 *
 * DESCRIPTION:
 *  This function copies one fbe_sector_t of data. This is the C version of 
 *  xor_cpy_mmx.
 *
 *  PARAMETERS:
 *   srcptr,     [I]  - ptr to source buffer
 *   tgtptr,     [I]  - ptr to destination buffer 
 *
 * RETURN VALUES/ERRORS:
 *   none
 *
 *  NOTES:
 *
 * HISTORY:
 *  4/23/07: Shuyu Lee - Created.
 *
 ****************************************************************/
void fbe_xor_cpy_only_non_mmx(const fbe_u32_t *srcptr, fbe_u32_t *tgtptr)
{
    fbe_u16_t passcnt;

    /* Implement the "classic" algorithm for the data
     * in the buffer.
     */
    FBE_XOR_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        FBE_XOR_REPEAT(*tgtptr++ = *srcptr++);
    }

    return;
} /* fbe_xor_cpy_only_non_mmx */

/***************************************************************************
 *		fbe_xor_cook_fixd_csum()
 ***************************************************************************
 *	DESCRIPTION:
 *		This function "seeds" a "raw" checksum and "folds" it to
 *		create the 16-bit value to be attached to a fbe_sector_t as its
 *		"real" checksum. A fixed value is supplied as the seed.
 *
 *	PARAMETERS:
 *		checksum	- [I]	the "raw" checksum
 *		seed		- [I]	ignored
 *
 *	RETURN VALUE:
 *		The seeded, folded & rotated checksum, suitable for storage.
 *
 *	NOTES:
 *		Implements fixed-seed checksums.
 *
 *	HISTORY:
 *		17-Sep-98	-SPN-	Created.
 *              01-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_u16_t fbe_xor_cook_fixd_csum(fbe_u32_t checksum,
                  fbe_u32_t ignored)
{
    /* Add seed to checksum.
     */
    checksum ^= FBE_SECTOR_CHECKSUM_SEED_CONST;

    /* Rotate.
     */
    if (checksum & 0x80000000u)
    {
        checksum <<= 1;
        checksum |= 1;
    }
    else
    {
        checksum <<= 1;
    }

    /* Fold and return.
     */
    return (0xFFFFu & ((checksum >> 16) ^ checksum));
}

/*!**************************************************************************
 * fbe_xor_csum_init()
 ***************************************************************************
 * @brief
 *  This function indicates the preferred checksum algorithm.
 * 
 * @param - None.
 * 
 * @return fbe_status_t
 * 
 * @author
 *  09-Mar-99       -SPN-   Created.
 *  01-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_status_t fbe_xor_csum_init(void)
{
    if ((fbe_UseMMX == MMX_UNKNOWN) && (fbe_UseMMX = fbe_mmx_cpuid()))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_XOR,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "XOR Library is using MMX Functions.\n");
        fbe_xor_calc_csum = fbe_xor_calc_csum_mmx,
        fbe_xor_calc_csum_and_cpy = fbe_xor_calc_csum_and_cpy_mmx,
        fbe_xor_calc_csum_and_xor = fbe_xor_calc_csum_and_xor_mmx,
        fbe_xor_calc_csum_and_cpy_vault = fbe_xor_calc_csum_and_cpy_vault_mmx,
        fbe_xor_calc_csum_and_xor_vault = fbe_xor_calc_csum_and_xor_vault_mmx,
        fbe_xor_calc_csum_and_cmp = fbe_xor_calc_csum_and_cmp_mmx;
        fbe_xor_cpy_only = fbe_xor_cpy_only_non_mmx;
#if defined (__AMD64_) && !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
        fbe_xor_calc_csum_and_cpy_to_temp = fbe_xor_calc_csum_and_cpy_to_temp_mmx; 
        fbe_xor_calc_csum_and_xor_to_temp = fbe_xor_calc_csum_and_xor_to_temp_mmx;
        fbe_xor_calc_csum_and_xor_with_temp_and_cpy = fbe_xor_calc_csum_and_xor_with_temp_and_cpy_mmx;
        fbe_xor_468_calc_csum_and_update_parity = fbe_xor_468_calc_csum_and_update_parity_mmx;
        fbe_xor_generate_lba_stamp_local = fbe_xor_generate_lba_stamp_mmx; 
        fbe_xor_is_valid_lba_stamp_local = fbe_xor_is_valid_lba_stamp_mmx;

        /* The standard copy routine is optimized under amd 64 to not copy
         * the block sequentially.  Cases that require the block to be copied 
         * sequentially need to rely on this function. 
         */
        fbe_xor_calc_csum_and_cpy_sequential = fbe_xor_calc_csum_and_cpy_sequential_mmx;
#else
        /* When amd 64 is not defined, we should use the standard inline mmx
         * routine, which will copy the block sequentially. 
         */
        fbe_xor_calc_csum_and_cpy_sequential = fbe_xor_calc_csum_and_cpy_mmx;
#endif /* _AMD64_ */
    }
	return FBE_STATUS_OK;
}
/**************************************
 * end fbe_xor_csum_init
 **************************************/
/***************************************************************************
 *              fbe_xor_is_supported_csum()
 ***************************************************************************
 *      DESCRIPTION:
 *              This function indicates whether the checksum algorithm 
 *              indicated is supported.
 *
 *      PARAMETERS:
 *              csum_algorithm  - [I]   a formal value from fbe_xor_csum_alg_t
 *
 *      RETURN VALUE:
 *              None.
 *
 *      NOTES:
 *
 *      HISTORY:
 *              09-Mar-99       -SPN-   Created.
 *              02-Aug-01       A. Vankamamidi Ported to XOR driver
 ***************************************************************************/

fbe_bool_t fbe_xor_is_supported_csum(fbe_xor_csum_alg_t csum_algorithm)
{
    return (fbe_xor_chksum_algorithm == csum_algorithm);
}

/***************************************************************************
 *              fbe_xor_compute_chksum()
 ***************************************************************************
 *      DESCRIPTION:
 *              This function computes the buffer checksum using the full 
 *              fixed checksum algorithm.
 *
 *      NOTES:
 *
 *      HISTORY:
 *              31-Aug-01       A. Vankamamidi Created
 ***************************************************************************/

fbe_u16_t fbe_xor_compute_chksum(void *srcptr, fbe_u32_t seed)
{
  fbe_u32_t raw_chksum;
  fbe_u16_t chksum;

  raw_chksum = fbe_xor_calc_csum((fbe_u32_t *)srcptr);
  chksum = fbe_xor_cook_csum(raw_chksum, seed);

  return (chksum);
}

/**************************************************************************
 * fbe_xor_generate_lba_stamp()
 **************************************************************************
 *
 * DESCRIPTION:
 *   Generates LBA stamp from the provided LBA. To calculate LBA
 *   stamp, 64-bit LBA is sliced and XORed along 16-bit boundry to give
 *   16-bit LBA stamp.
 *
 * PARAMETERS:
 *   lba       [I] - Logical Block Address.
 *
 * RETURN VALUE:
 *   lba_stamp  - LBA stamp.
 *
 * NOTES:
 *
 * HISTORY:
 *   01-02-08       Vamsi Vankamamidi   Created.
 **************************************************************************/
static fbe_u16_t  fbe_xor_generate_lba_stamp_non_mmx(ULONGLONG lba)
{
    return (fbe_u16_t)((lba) ^ (lba >> 16) ^ (lba >> 32) ^ (lba >> 48)); 
}/* fbe_xor_generate_lba_stamp */

/**************************************************************************
 * fbe_xor_is_valid_lba_stamp_non_mmx()
 **************************************************************************
 *
 * DESCRIPTION:
 *   Determines whether the passed in LBA stamp is valid. Valid is defined
 *   as matching either the stamp generated from passed in lba or 0. 
 *   If passed in lba stamp is 0, it is changed to stamp generated from the 
 *   passed in lba.
 *
 * PARAMETERS:
 *   lba_stamp  [I] - LBA stamp to validate.
 *   lba        [I] - LBA (Logical Block Address).
 *
 * RETURN VALUES/ERRORS:
 *   fbe_bool_t - TRUE if a valid stamp.
 *
 * NOTES:
 *
 * HISTORY:
 *   01-02-08       Vamsi Vankamamidi   Created.
 **************************************************************************/
static fbe_bool_t fbe_xor_is_valid_lba_stamp_non_mmx(fbe_u16_t *lba_stamp, ULONGLONG lba)
{
    fbe_u16_t stamp;
    // Generate LBA stamp from the passed in LBA
    stamp = (fbe_u16_t)((lba) ^ (lba >> 16) ^ (lba >> 32) ^ (lba >> 48)); 
    // Validate passed in LBA stamp.
    return ( (*lba_stamp == 0) ? ((*lba_stamp = stamp), TRUE) : (*lba_stamp == stamp) );
}/* fbe_xor_is_valid_lba_stamp_non_mmx */

/*!***************************************************************************
 *          fbe_xor_generate_lba_stamp()
 *****************************************************************************
 *
 * @brief   The xor library `seed' lba does not contain the offset from the
 *          the downstream edge.  This method adds the passed offset to the
 *          seed (lba) provided and then invoke the method that computes the
 *          lba stamp.
 *
 * @param   lba - the lba (a.k.a. seed) to compute the LBA stamp for
 * @param   offset - the edge offset to be added to the lba whne generating the
 *                   LBA stamp
 *
 * @return  lba stamp - The 16-bit `folded' LBA stamp value
 *
 * @author
 *  07/20/2010  Ron Proulx  - Created
 *****************************************************************************/
fbe_u16_t fbe_xor_generate_lba_stamp(fbe_lba_t lba, fbe_lba_t offset)
{
    return(fbe_xor_generate_lba_stamp_local(lba + offset));
}
/* end of fbe_xor_generate_lba_stamp_with_offset*/

/*!***************************************************************************
 *          fbe_xor_is_valid_lba_stamp()
 *****************************************************************************
 *
 * @brief   Determines whether the passed in LBA stamp is valid. Valid is
 *          defined as matching either the stamp generated from the passed
 *          lba and offset or 0. If passed in lba stamp is 0, it is changed to
 *          stamp generated from the passed in lba and offset.
 *
 * @param   lba_stamp_p - Address of LBA stamp to validate.
 * @param   lba - LBA (Logical Block Address) that doesn't include offset
 * @param   offset - The offset to add to lba when validating stamp
 *
 * @return  fbe_bool_t - FBE_TRUE if a valid stamp.
 *
 * @author
 *  07/20/2010  Ron Proulx - Created
 **************************************************************************/
fbe_bool_t fbe_xor_is_valid_lba_stamp(fbe_u16_t *lba_stamp_p, fbe_lba_t lba, fbe_lba_t offset)
{
    /* Simply invoke the local method with the lba plus offset
     */
    return(fbe_xor_is_valid_lba_stamp_local(lba_stamp_p, (lba + offset)));
}/* fbe_xor_is_valid_lba_stamp */


/******************************
 * end file fbe_xor_csum_util.c
 ******************************/
#endif
