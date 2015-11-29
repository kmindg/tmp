#ifndef __DBG_EXT_CSX_H__
#define __DBG_EXT_CSX_H__

#ifndef I_AM_DEBUG_EXTENSION
#error "dbgext_csx.h should only be included for code being built for TARGETTYPE CSX_DEBUG_EXTENSION or CSX_DEBUG_EXTENSION_LIBRARY"
#endif

#include "csx_ext_dbgext.h"
#include "EmcUTIL_DbgHelp.h"
#include "EmcPAL.h"

#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <windef.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - debug extension differences */
#include <stddef.h>
#include <string.h>

#if defined(DBGEXT_PROVIDE_NT_COMPAT_DEFS) && defined(ALAMOSA_WINDOWS_ENV)

#define SIZE_PTR_64    8
#define SIZE_PTR_32    4

#ifndef ALAMOSA_WINDOWS_ENV
#include <wdbgexts.h>
#else

#define LMEM_FIXED              0x0000
#define LMEM_MOVEABLE           0x0002
#define LMEM_NOCOMPACT          0x0010
#define LMEM_NODISCARD          0x0020
#define LMEM_ZEROINIT           0x0040
#define LMEM_MODIFY             0x0080
#define LMEM_DISCARDABLE        0x0F00
#define LMEM_VALID_FLAGS        0x0F72
#define LMEM_INVALID_HANDLE     0x8000
#define LPTR                    (LMEM_FIXED | LMEM_ZEROINIT)
#define WINBASEAPI

# if defined(__cplusplus)
extern "C" {
#endif
WINBASEAPI HLOCAL WINAPI LocalAlloc(UINT uFlags, SIZE_T uBytes);
WINBASEAPI HLOCAL WINAPI LocalFree(HLOCAL hMem);
WINBASEAPI VOID WINAPI RaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments);
#ifdef __cplusplus
}
#endif
#define EXCEPTION_ACCESS_VIOLATION          STATUS_ACCESS_VIOLATION

#ifndef CopyMemory
#define CopyMemory memcpy
#define ZeroMemory(a,b) memset(a,0,b)
#define FillMemory(a,b,c) memset(a,c,b)
#endif

#define KDEXT_64BIT
#include <wdbgexts.h>
#include "k10ntddk.h"
#include "bool.h"

#if defined(__cplusplus)
extern "C" {
#endif
extern WINDBG_EXTENSION_APIS    ExtensionApis;
extern BOOL                     globalCtrlC;
#if defined(__cplusplus)
};
#endif

#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - debug extension differences */

#endif /* defined(DBGEXT_PROVIDE_NT_COMPAT_DEFS) && defined(ALAMOSA_WINDOWS_ENV) */

/* Perform large memory reads in READ_SIZE chunks, checking for ctrl/C on each
 * iteration to avoid a monolithic large read hanging the debugger with no way to break out */

#define READ_SIZE 1024

#ifndef SIZE_PTR_64
#define SIZE_PTR_64     8
#endif

/*
 * Macros
 */

#define STRNCPY_FIXED_LENGTH_DEST( m_to, m_from )	\
  {							\
    m_to[sizeof(m_to) -1] = 0;				\
    strncpy(m_to, m_from, sizeof(m_to) -1);		\
  }

#define GETFIELDOFFSET(type, field, pOffset)CSX_DBG_EXT_UTIL_GET_FIELD_OFFSET(type, field, pOffset)

#define GETFIELDOFFSET_RT(type, field, pOffset)				\
  {									\
    csx_status_e _status;						\
    csx_nuint_t * _pOffset = (csx_nuint_t*)(pOffset);			\
    _status = csx_dbg_ext_get_field_offset(type, field, _pOffset);	\
    if (CSX_FAILURE(_status)) {						\
      CSX_DBG_EXT_PRINT("%s: failed to get field offset read for %s:%s (status 0x%x)\n", CSX_MY_FUNC, type, field, _status); \
      return 0;								\
    }									\
  }

#define ExprIn(module_name, symbol_name, location)			\
  {									\
    ULONG64 _addr = csx_dbg_ext_lookup_symbol_address(module_name,symbol_name);	\
    if (!_addr) {							\
      csx_dbg_ext_print( "csx_dbg_ext_lookup_symbol_address(%s) failed.\n", symbol_name ); \
      return;								\
    }									\
    ReadIn(_addr, symbol_name, location);				\
  }

extern void _csx_dbg_ext_read_in_field64Private(csx_u64_t Addr, const char *const Type, const char *const Field, csx_u32_t OutSize, void *pOutValue);
#define csx_dbg_ext_read_in_field64(Addr, Type, Field, OutValue) _csx_dbg_ext_read_in_field64Private(Addr, Type, Field, sizeof(OutValue), (PVOID)&(OutValue))

#define GETFIELDVALUE(Addr, Type, Field, OutValue) CSX_DBG_EXT_UTIL_READ_IN_FIELD(Addr, Type, Field, sizeof(OutValue), (csx_pvoid_t)&(OutValue))

#define GETFIELDVALUE_RT(Addr, Type, Field, OutValue)			\
  {									\
    csx_status_e _status;						\
    csx_ptrmax_t _addr = Addr;						\
    csx_pvoid_t _outvalue = (csx_pvoid_t)(&OutValue);			\
    _status = csx_dbg_ext_read_in_field(_addr, Type, Field, sizeof(OutValue), _outvalue); \
    if (CSX_FAILURE(_status)) {						\
      csx_dbg_ext_print("%s: failed to read in field %s:%s (type: 0x%llx) (status: 0x%x)\n", CSX_MY_FUNC, Type, Field, (unsigned long long)_addr, _status); \
      return -1;							\
    }									\
  }

#define ReadIn(addr, s, location)					\
  {									\
    csx_status_e _status;						\
    csx_ptrmax_t _adr = addr; \
    csx_size_t _len = sizeof(*(location)); \
    _status = csx_dbg_ext_read_in_len(_adr, location, _len);			\
    if (CSX_FAILURE(_status))						\
      {									\
	csx_dbg_ext_print("csx_dbg_ext_read_in_len %s @ %llx failed\n", s, (unsigned long long)_adr); \
	return;								\
      }									\
  }

#define ReadIn_RT(addr, s, location)					\
  {									\
    csx_status_e _status;						\
    csx_ptrmax_t _adr = addr; \
    csx_size_t _len = sizeof(*(location)); \
    _status = csx_dbg_ext_read_in_len(_adr, location, _len);			\
    if (CSX_FAILURE(_status))						\
      {									\
	csx_dbg_ext_print("csx_dbg_ext_read_in_len %s @ %llx failed\n", s, (unsigned long long)_adr); \
	return CSX_FALSE;								\
      }									\
  }

#define ReadInStr(addr, s, location, maxlen) CSX_DBG_EXT_UTIL_READ_IN_STRING(addr, location, maxlen)
#define ReadInStr_RT(addr, s, location, maxlen) CSX_DBG_EXT_UTIL_READ_IN_STRING_RT(addr, location, maxlen)

csx_sptrhld_t GetArgument(const char * String, csx_u32_t Count);
csx_s64_t GetArgument64(const char * String, csx_u32_t Count);
csx_u64_t atoi64(const char * string);

csx_bool_t Read_In (void *addrPA, const char *const symbolPA,
	      void *destPA, csx_size_t sizeA);

csx_bool_t
Read_In64 (csx_u64_t addrPA, const char *const symbolPA,
             void* destPA, csx_size_t sizeA);


csx_bool_t Read_In_Str (void *addrPA, const char *const symbolPA, void *destPA,
		  csx_size_t sizeA);

/*Macro to Read addresses for pointer to make it 64 and 32 bit compatible*/
#define READMEMORY_ADDR(addr,dest,label)			      \
  {								      \
  csx_ptrmax_t remote_addr = addr;				      \
  csx_ptrmax_t local_addr;					      \
  csx_status_e status;						      \
  status = csx_dbg_ext_util_read_in_pointer(remote_addr,&local_addr); \
  if(CSX_FAILURE(status))					      \
    {								      \
      csx_dbg_ext_print("READMEMORY_ADDR: Failed to read in pointer at 0x%llx for "#label"\n", (unsigned long long)remote_addr); \
      *dest = 0; \
    }								      \
  else								      \
    {								      \
      *dest = local_addr;					      \
    }								      \
  }

// CheckControlC() macro to check/set globalCtrlC
// Note: In CheckVersion() globalCtrlC = FALSE;

#if defined(__cplusplus)
extern "C" {
extern csx_bool_t globalCtrlC;
}
#else
extern csx_bool_t globalCtrlC;
#endif

#define CHECK_CONTROL_C() ((csx_dbg_ext_was_abort_requested() || globalCtrlC) ? (globalCtrlC = 1) : 0)

#define CHECK_POINTER64() ( (csx_dbg_ext_util_get_target_pointer_size() == 8) ? 1 : 0 )

static __inline void set_global_control_c(void)
{
    /* Simply set the global control c value based on 
     * calling check control c.  This way of doing this operation 
     * avoids ambiguity in the way globalCtrlC is set since it 
     * also can get set inside of CheckControlC(). 
     */
    csx_bool_t new_global_control_c = CHECK_CONTROL_C();
    globalCtrlC = new_global_control_c;
    return;
}
/**2  Macro To handle ReadMemory Api **/


#define READMEMORY(source, destination, size)			\
  {								\
    csx_status_e status;					\
    csx_ptrmax_t csx_source = source;				\
    csx_pvoid_t csx_dest = destination;				\
    status = csx_dbg_ext_read_in_len(csx_source,csx_dest,size);	\
    if(CSX_FAILURE(status))					\
      {									\
	csx_dbg_ext_print("csx_dbg_ext_read_in_len at %llx failed\n", (unsigned long long)source); \
	return;								\
      }									\
  }

/* READMEMORY has two problems: 
 * 1) If it takes an error, the caller/utilizer doesn't know it and may process uninitialized data. 
 * 2) If it takes an error, it does a "return." That means the calling/utilizing function returns.
 *
 * What makes READMEMORY_OR_ZERO different is it will zero-out the destination buffer if the read fails,
 * and it doesn't have a return statement at all so the function that utilizes it won't return.
 */
#define READMEMORY_OR_ZERO(source, destination, size)			\
  {								\
    csx_status_e status;					\
    csx_ptrmax_t csx_source = source;				\
    csx_pvoid_t csx_dest = destination;				\
    status = csx_dbg_ext_read_in_len(csx_source,csx_dest,size);	\
    if(CSX_FAILURE(status))					\
      {									\
	  csx_dbg_ext_print("csx_dbg_ext_read_in_len at %llx failed\n", (unsigned long long)source); \
      memset(csx_dest,0,size);          \
      }									\
  }

#define READMEMORY_RT(source, destination, size)		\
  {								\
    csx_status_e status;					\
    status = csx_dbg_ext_read_in_len(source,destination,size);	\
    if(CSX_FAILURE(status))						\
      {									\
	csx_dbg_ext_print("csx_dbg_ext_read_in_len at %llx failed\n", (unsigned long long)source); \
	return 0;								\
      }									\
  }

#ifndef ALAMOSA_WINDOWS_ENV
#define CSX_DBG_EXT_UTIL_NAME_OF_USER_POINTERS "dcall_user_pointers"
#else
#define CSX_DBG_EXT_UTIL_NAME_OF_USER_POINTERS "Tail.Overlay.DriverContext"
#endif

CSX_LOCAL CSX_INLINE void retrieve_global_tick_count(PVOID pTCount) 
{									
#ifndef ALAMOSA_WINDOWS_ENV
    // CSX uses millisecond resolution 
    // so tick count is the time in msec
    ULONG64 addr;
    ULONG64 current_time_msecs = 0;
    csx_lis64_t* pTickCount = (csx_lis64_t*)pTCount;
    addr = csx_dbg_ext_lookup_symbol_address( "csx_krt", "csx_rt_sked_time_ref_current_time_msecs");
    if (!addr) addr = csx_dbg_ext_lookup_symbol_address( "csx_rt", "csx_rt_sked_time_ref_current_time_msecs");
    if (addr) {
        csx_dbg_ext_read_in_len(addr, &current_time_msecs, sizeof(current_time_msecs));
        pTickCount->QuadPart = current_time_msecs;
    }
#else
#define NT_USER_SHARED_DATA CSX_CONST_U64(0xFFFFF78000000000)		
#define NT_TICK_COUNT_POINTER (NT_USER_SHARED_DATA + 0x320)		
  csx_status_e status;							
  EMCPAL_LARGE_INTEGER* pTickCount = (EMCPAL_LARGE_INTEGER*)pTCount;
  if(CHECK_POINTER64())							
    {									
      status = csx_dbg_ext_read_in_len(NT_TICK_COUNT_POINTER, pTickCount, sizeof(EMCPAL_LARGE_INTEGER));  
      if(CSX_FAILURE(status))
	{
	  csx_dbg_ext_print("retrieve_global_tick_count failed to read in NT_TICK_COUNT_POINTER from %llx\n", (unsigned long long)NT_TICK_COUNT_POINTER);
	  return;
	}
    }
  else
    {
      ExprIn("nt","KeTickCount", pTickCount);
    }
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - debug */
}


CSX_LOCAL CSX_INLINE void retrieve_global_time_increment(PULONG pTimeIncrement) 
{
#ifndef ALAMOSA_WINDOWS_ENV
    // CSX uses millisecond resolution
    // Convert to 100-nano second unit
    *pTimeIncrement = 1 * CSX_XSEC_PER_MSEC;
#else
  ULONG64 PU_TimeIncrement;
    
  if(CHECK_POINTER64())
    {
      PU_TimeIncrement = (ULONG64)csx_dbg_ext_lookup_symbol_address("scsitarg","TcdNtTimeIncrement");	  
    }
  else
    {
      PU_TimeIncrement = csx_dbg_ext_lookup_symbol_address ("nt","KeMaximumIncrement");	  
    }

  READMEMORY(PU_TimeIncrement,pTimeIncrement,sizeof(ULONG));
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - debug */
}

CSX_LOCAL CSX_INLINE void retrieve_global_system_time(PVOID pSysTime) 
{
#ifndef ALAMOSA_WINDOWS_ENV
    ULONG64 addr;
    ULONG64 current_time_msecs = 0;
    csx_lis64_t* pSystemTime = (csx_lis64_t*)pSysTime;
    addr = csx_dbg_ext_lookup_symbol_address( "csx_krt", "csx_rt_sked_time_ref_current_time_msecs");
    if (!addr) addr = csx_dbg_ext_lookup_symbol_address( "csx_rt", "csx_rt_sked_time_ref_current_time_msecs");
    if (addr) {
        csx_dbg_ext_read_in_len(addr, &current_time_msecs, sizeof(current_time_msecs));
        pSystemTime->QuadPart = current_time_msecs;
    }
#else
#define NT_USER_SHARED_DATA CSX_CONST_U64(0xFFFFF78000000000)
#define NT_SYSTEM_TIME_POINTER (NT_USER_SHARED_DATA + 0x14)
    EMCPAL_LARGE_INTEGER TickCount;
    ULONG TimeIncrement;
    EMCPAL_LARGE_INTEGER* pSystemTime = (EMCPAL_LARGE_INTEGER*)pSysTime;

    if(CHECK_POINTER64())							
    {									
        READMEMORY(NT_SYSTEM_TIME_POINTER, pSystemTime, sizeof(EMCPAL_LARGE_INTEGER));
    }
    else
    {
        retrieve_global_tick_count(&TickCount);
        retrieve_global_time_increment(&TimeIncrement);
        pSystemTime->QuadPart = TickCount.QuadPart * TimeIncrement;
    }
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - debug */
}

CSX_LOCAL CSX_INLINE BOOLEAN PointerInKernelSpace(ULONG64 MemPtr)
{
#ifndef ALAMOSA_WINDOWS_ENV
    ULONG64 Value;
    csx_status_e status = CSX_STATUS_SUCCESS;
    status = csx_dbg_ext_util_read_in_pointer(MemPtr, &Value);
    if (CSX_SUCCESS(status)) 
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    BOOLEAN     RetVal              = FALSE;
    csx_size_t  Size                = 0;
    ULONG64     StartOfKernelSpace  = 0;

    Size = csx_dbg_ext_get_type_size("csx_pchar_t");

    if( Size == 4 )
    {
        StartOfKernelSpace = (ULONG64)0x80000000;
    }
    else if ( Size == 8 )
    {
        StartOfKernelSpace = (ULONG64)0xFFFF080000000000;
    }
    else
    {
        csx_dbg_ext_print("Error in determining kernel memory space\n");
    }

    if( MemPtr >= StartOfKernelSpace )
    {
        RetVal = TRUE;
    }
    else
    {
        RetVal = FALSE;
    }

    return RetVal;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - debug */
}

#endif // __DBG_EXT_CSX_H__
