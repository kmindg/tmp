#ifndef FBE_WINDDK_H
#define FBE_WINDDK_H

#include "EmcPAL.h"
#include "EmcPAL_Memory.h"
#include "EmcPAL_Irp.h"
#include "EmcPAL_DriverShell.h"
#include "EmcUTIL.h"

#include "fbe/fbe_platform.h"
#include "EmcPAL_Misc.h"

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
//PZPZ #include <windows.h>
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#include "fbe/fbe_types.h"

//PZPZ
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
//PZPZ undef  printf
//PZPZ undef  DbgPrint
#endif

#include <stdio.h>
#include <stdarg.h>
#if (defined(UMODE_ENV) || defined(SIMMODE_ENV))
#ifndef I_AM_NONNATIVE_CODE
#include <stdlib.h>
#include <conio.h>
#endif
#endif

//PZPZ
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
//PZPZ undef  printf
//PZPZ define printf DbgPrint
#define fbe_printf EmcpalDbgPrint
#else
#define fbe_printf printf
#endif

//PZPZ
#include "fbe/fbe_ktrace_interface.h"
/*! @todo Currently there are fbe methods that are directly calling KvTrace. 
 *        Once all those calls are converted to call fbe_KvTrace the following
 *        define should be removed.
 */
#include "ktrace.h"
#define KvTrace     fbe_KvTrace

/******************************/

typedef struct fbe_memory_call_stack_trace_debug_s{
	void *		mem_ptr;
	fbe_u32_t *	stack1_ptr;
	fbe_u32_t *	stack2_ptr;
	fbe_u32_t	block_lengh;
}fbe_memory_call_stack_trace_debug_t;

/******************************/

#define FBE_MAX_PATH 260

/******************************/

#if defined(K10_ENV)
/*! @todo For methods still using Flare we need to continue to use the ktrace 
 *        driver.  Once Flare is removed the following override should be
 *        removed.
 *   
 */
#define fbe_KvTrace     KvTrace
#endif /* in Flare environment */

/******************************/

//PZPZ typedef VOID (*fbe_thread_user_root_t) ( IN PVOID StartContext ); //PZPZ

/******************************/

#define ADD_TO_BACKTRACE_TABLE(_trace_table_ptr, _mem_p, _bytes, _index,_max)
#define REMOVE_FROM_BACKTRACE_TABLE(_trace_table_ptr, mem_p, _index, max)
__forceinline static void memory_analyze_native_leak(fbe_memory_call_stack_trace_debug_t *trace_table_ptr, fbe_u32_t max_entries)
{
}

#if 0
/*walk the stack:
start with ebp+4 which is the address of the caller on the stack before us
then we look waht's in ebp itself which point to the caller before it*/
#if !defined(_WIN64) 
#define ADD_TO_BACKTRACE_TABLE(_trace_table_ptr, _mem_p, _bytes, _index,_max) \
	while (_index < _max){ \
        if (_trace_table_ptr[_index].mem_ptr ==NULL) { \
	        fbe_u32_t *			addr_ptr = NULL; \
	        fbe_u32_t *			addr_ptr2 = NULL; \
	        _trace_table_ptr[_index].mem_ptr = _mem_p; \
	         __asm \
	         {\
		         __asm push ebx \
		         __asm push ecx \
                 __asm mov ebx,[ebp + 4] \
		         __asm mov addr_ptr, ebx \
		         __asm mov ebx, [ebp] \
		         __asm mov ecx, ebx \
		         __asm mov ebx, [ecx + 4] \
                 __asm mov addr_ptr2, ebx \
		         __asm pop ecx \
                 __asm pop ebx \
	        } \
	        _trace_table_ptr[_index].stack1_ptr = addr_ptr; \
	        _trace_table_ptr[_index].stack2_ptr = addr_ptr2; \
	        _trace_table_ptr[_index].block_lengh = _bytes; \
            break; \
        } \
        _index ++; \
    } \
	
#else
#define GET_RETURN_ADDRESS (_ReturnAddress())
#define ADD_TO_BACKTRACE_TABLE(_trace_table_ptr, _mem_p, _bytes, _index,_max) \
	while (_index < _max){ \
        if (_trace_table_ptr[_index].mem_ptr ==NULL) { \
            fbe_u32_t *			addr_ptr = NULL; \
            fbe_u32_t *			addr_ptr2 = NULL; \
            _trace_table_ptr[_index].mem_ptr = _mem_p; \
            _trace_table_ptr[_index].stack1_ptr = GET_RETURN_ADDRESS; \
            _trace_table_ptr[_index].block_lengh = _bytes; \
            break; \
        } \
        _index ++; \
    } \

#endif /*!defined(_WIN64)*/

#define REMOVE_FROM_BACKTRACE_TABLE(_trace_table_ptr, mem_p, _index, max)\
	{\
		fbe_u32_t idx = 0;\
		for (;idx < max; idx++) {\
			if (_trace_table_ptr[idx].mem_ptr == mem_p) {\
				_trace_table_ptr[idx].mem_ptr = NULL;\
				_trace_table_ptr[idx].stack1_ptr = NULL;\
				_trace_table_ptr[idx].stack2_ptr = NULL;\
                if (idx < _index) { \
                    _index = idx; \
                } \
				break;\
			}\
		}\
	}\

__forceinline static void memory_analyze_native_leak(fbe_memory_call_stack_trace_debug_t *trace_table_ptr, fbe_u32_t max_entries)
{
	#if !defined(_WIN64)
	fbe_u32_t	idx = 0;
	DWORD 		pid =  GetProcessId (GetCurrentProcess());
	
	printf("\n\nConnect Windbg to process ID:%d and in the command line, type 'ln <ADDR>'\nADDR is the address in call stack[0] or call stack[1]\n", pid); 
    printf("memory_ptr points to the start of the memory which was used and not freed\n\n");
	for (;idx < max_entries; idx++) {
		if (trace_table_ptr[idx].stack1_ptr != NULL) {
			printf("Entry %d\n===========\n", idx);
			printf("call stack[0]:0x%p\ncall stack[1]:0x%p\nmemory_ptr:0x%p, mem_size = %d bytes\n\n", 
				   (void *)trace_table_ptr[idx].stack1_ptr,
				   (void *)trace_table_ptr[idx].stack2_ptr,
				   (void *)trace_table_ptr[idx].mem_ptr,
				    trace_table_ptr[idx].block_lengh);

		}
	}
	#endif
}
#endif

/******************************/

#if 0 //PZPZ
void fbe_get_number_of_cpu_cores(fbe_u32_t *n_cores);
void * fbe_allocate_contiguous_memory(fbe_u32_t number_of_bytes);
void fbe_release_contiguous_memory(void * memory_ptr);
void * fbe_allocate_nonpaged_pool_with_tag(fbe_u32_t number_of_bytes, fbe_u32_t tag);
void fbe_release_nonpaged_pool(void * memory_ptr);
void fbe_release_nonpaged_pool_with_tag(void * memory_ptr, fbe_u32_t tag);
fbe_s32_t fbe_compare_string(fbe_u8_t * src1, fbe_u32_t s1_length, fbe_u8_t * src2, fbe_u32_t s2_length, fbe_bool_t case_sensitive);
void fbe_sprintf(char* pszDest, size_t cbDest, const char* pszFormat, ...);
void fbe_debug_break();
void fbe_DbgBreakPoint(void);
#endif

/******************************/

#if 0 //PZPZ
//PQPQ typedef int fbe_irql_t;
fbe_irql_t fbe_set_irql_to_dpc(void);
void fbe_restore_irql(fbe_irql_t irql);
#if 0
typedef EMCPAL_KIRQL fbe_irql_t;
static __forceinline fbe_u32_t fbe_set_irql_to_dpc(void)
{   
    fbe_irql_t  irql;
    irql = EmcpalGetCurrentLevel();
    
    // avoid the IQRL raise which stalls pipe-line if it
    // would be a no-op.
    if (irql != DISPATCH_LEVEL) {
        if ((EMCPAL_THIS_LEVEL_IS_NOT_PASSIVE(irql)) && (irql != APC_LEVEL)) {
            fbe_KvTrace("%s: irql %d unexpected\n",
                        __FUNCTION__, irql);
        }
        EmcpalLevelRaiseToDispatch(&OldIrql);
    }
    return irql;
}

static __forceinline void fbe_restore_irql(fbe_irql_t irql)
{
    if (irql != DISPATCH_LEVEL) {
        EmcpalLevelLower(irql);
    }
}
#endif
#endif

/******************************/

/* Memory manipulators */

__forceinline static void fbe_zero_memory(void * dst,  fbe_u32_t length)
{
    EmcpalZeroMemory(dst,length);
}

__forceinline static void fbe_copy_memory(void * dst, const void * src, fbe_u32_t length)
{
    EmcpalCopyMemory(dst,src,length);
}

__forceinline static void fbe_move_memory(void * dst, const void * src, fbe_u32_t length)
{
    EmcpalMoveMemory(dst,src,length);
}

__forceinline static void fbe_set_memory(void * dst, unsigned char fill, fbe_u32_t length)
{
    EmcpalFillMemory(dst,length, fill);
}

__forceinline static fbe_bool_t fbe_equal_memory(const void * src1, const void * src2, fbe_u32_t length)
{
    return(EmcpalEqualMemory(src1,src2,length));
}
#endif /* FBE_WINDDK_H */
