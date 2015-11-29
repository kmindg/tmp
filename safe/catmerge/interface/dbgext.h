
//
//  HERE BEGINS A MAGICAL SEQUENCE OF HEADER FILES AND DEFINTIONS.
//  IT HAS BEEN STOLEN FROM THE BAKER BOOK.  DON'T CHANGE ANYTHING !!!
//
#ifndef __DBG_EXT_H__
#define __DBG_EXT_H__

#ifndef I_AM_DEBUG_EXTENSION
#error "dbgext.h should only be included for code being built for TARGETTYPE CSX_DEBUG_EXTENSION or CSX_DEBUG_EXTENSION_LIBRARY"
#endif

#ifndef ALAMOSA_WINDOWS_ENV
#define _WDBGEXTS_
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - insanity as other places use this NT header guard to deselect crap that has no business in debugger extensions */

#ifdef USE_CSX_DBGEXT
#include "dbgext_csx.h"
#else /* USE_CSX_DBGEXT */

# if defined(__cplusplus)
extern "C" {
#include <ntddk.h>
}
# else 
#include <ntddk.h>
# endif

#include <windef.h>
#include <stddef.h>
#include <string.h>

#define SIZE_PTR_64    8
#define SIZE_PTR_32    4

#ifdef ALAMOSA_WINDOWS_ENV

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
WINBASEAPI
HLOCAL
WINAPI
LocalAlloc(
    UINT uFlags,
    SIZE_T uBytes
    );

WINBASEAPI
HLOCAL
WINAPI
LocalFree(
    HLOCAL hMem
    );

WINBASEAPI
VOID
WINAPI
RaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    );
#ifdef __cplusplus
}
#endif

#endif /* ALAMOSA_WINDOWS_ENV - debug extension differences */

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

//
//  END OF MAGICAL HEADER SEQUENCE.  NOW BACK TO NORMAL STUFF.
//

#define READ_SIZE 1024

//  printf modifiers for windows and linux.
#ifdef ALAMOSA_WINDOWS_ENV
// For VC compiler
#define DBGEXT_64BIT_PRINTF_MODIFIER           "%I64x"
#define DBGEXT_64BIT_PRINTF_MODIFIER_WIDTH16   "%16I64x"
#else   
// For GCC compiler
#define DBGEXT_64BIT_PRINTF_MODIFIER           "%llx"
#define DBGEXT_64BIT_PRINTF_MODIFIER_WIDTH16   "%16llx"
#endif

extern WINDBG_EXTENSION_APIS    ExtensionApis;
extern BOOL                     globalCtrlC;

/*
 * Macros
 */
/* These macros are used for a fixed length array destination string
 * to ensure proper termination.  DO NOT USE if the destination
 * is just a pointer.
 */
#define STRNCPY_FIXED_LENGTH_DEST( m_to, m_from ) \
{   m_to[sizeof(m_to) -1] = 0; \
  strncpy(m_to, m_from, sizeof(m_to) -1); }

/*
 * Macros to get offset for a particular field .
 * On failure print message and return void. This will be used
 * in function which does not return any values. 
 */
#define GETFIELDOFFSET(type, field, pOffset)                            \
    if (GetFieldOffset(type, field, pOffset))                           \
    {                                                                   \
        nt_dprintf("Error in reading %s offset of %s \n", field, type);    \
        return;                                                         \
    }

/*
 * Macros to get offset for a particular field .
 * On failure print message and return values. This will be used 
 * function which returns values. 
 */
#define GETFIELDOFFSET_RT(type, field, pOffset)                         \
    if (GetFieldOffset(type, field, pOffset))                           \
    {                                                                   \
        nt_dprintf("Error in reading %s offset of %s \n", field, type);    \
        return 0;                                                       \
    }

/*
 * Macros to read debuggee memory.
 * On failure print message and return.
 * Break up large reads into small chunks, checking for Control-C
 *
 * ExprIn("&FLAREDRV!cm_local, &cm_local);
 * ReadIn(addr, "TRACE_RECORD", &t);
 */
#define ExprIn(s, location) \
{   ULONG64 _addr = GetExpression(s); \
    if (!_addr) { \
        nt_dprintf( "GetExpression(%s) failed.\n", s ); \
        return; \
    } \
    ReadIn(_addr, s, location); \
}

/*
 * GetFieldValue64()
 *
 * DESCRIPTION: Define function to ignore the value returned
 *              by GetFieldValue.
 *
 * PARAMETERS:  Addr - Fixed address of field to get value for
 *              Type - The Type of field
 *              Field - The field to get the value for
 *              OutValue - Pointer to place returned value
 *              
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS      None
 *
 * ERRORS
 *
 */
extern void _GetFieldValue64Private(ULONG64 Addr, const char *const Type, const char *const Field, ULONG OutSize, void *pOutValue);
#define GetFieldValue64(Addr, Type, Field, OutValue) _GetFieldValue64Private(Addr, Type, Field, sizeof(OutValue), (PVOID)&(OutValue))

/*!**************************************************************
 * @brief
 *
 * This macro is used by FDB macro routines to retrieve a field value
 * from a well-known structure address that resides within FLARE.
 *
 * @param Addr - This is the address of an instance of the structure
 * from which the value of a member field is to be retrieved. This
 * structure instance address was previously obtained by first
 * invoking the macro FLAREDRV_ADDR(), followed by invocation of the
 * macro READMEMORY_ADDR(). For example, there is a global structure
 * pointer in FLARE that is defined as: DST_DB_T
 * *drive_standby_db_p. Within our FDB support routines, we declare
 * some local storage to hold the pointer value: ULONG64
 * fdb_drive_standby_db_p. Then we can retrieve the actual pointer
 * value in this manner:
 * READMEMORY_ADDR(FLAREDRV_ADDR(drive_standby_db_p),
 * &fdb_drive_standby_db_p, DST_DB_T);
 *
 * @param Type - This is a quoted string which is the name of the
 * structure type (the typedef) for the structure instance address
 * specified in the Addr parameter.
 *
 * @param Field - This is a quoted string which is the name of the
 * field whose value we wish to retrieve from the structure instance
 * specified by Addr.
 *
 * @param OutValue - This is a locally declared variable, of the same
 * typedef as the field, into which we want to store the retrieved
 * structure field value. Specify the name of the locally declared
 * variable, not a pointer to it nor its address.
 *
 * @return void - None.
 *
 * @version:
 *  02/10/2009 - Commentary added. Michael J. Polia
 *
 ****************************************************************/
#define GETFIELDVALUE(Addr, Type, Field, OutValue)                                  \
  if(GetFieldValue(Addr, Type, Field, OutValue))                                    \
    {                                                                               \
      nt_dprintf("Err reading %s field:%s at 0x" DBGEXT_64BIT_PRINTF_MODIFIER"\n",   \
                 Type, Field, (unsigned long long)Addr);                            \
      return;                                                                       \
    }

#define GETFIELDVALUE_RT(Addr, Type, Field, OutValue)                               \
  if(GetFieldValue(Addr, Type, Field, OutValue))                                    \
    {                                                                               \
      nt_dprintf("Err reading %s field:%s at 0x" DBGEXT_64BIT_PRINTF_MODIFIER"\n",   \
                 Type, Field, (unsigned long long)Addr);                            \
      return -1;                                                                    \
    }

#define ReadIn(addr, s, location)                                           \
  {                                                                         \
    ULONG _bytes;                                                           \
    ULONG64 _adr = addr;                                                    \
    char *_loc = (char *)(location);                                        \
    ULONG _len = sizeof(*(location));                                       \
    while(_len > 1024) {                                                    \
      if( CheckControlC() ) return;                                         \
      if( !ReadMemory( _adr, _loc, 1024, &_bytes ) ) {                      \
        nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", \
                    s, (unsigned long long)addr );                          \
        return;                                                             \
      }                                                                     \
      _adr += 1024; _loc += 1024; _len -= 1024;                             \
    }                                                                       \
    if (_len > 0) {                                                         \
      if( !ReadMemory( _adr, _loc, _len, &_bytes ) ) {                      \
        nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", \
                    s, (unsigned long long)addr );                          \
        return;                                                             \
      }                                                                     \
    }                                                                       \
  }

#define ReadIn_RT(addr, s, location)                                            \
{                                                                               \
    ULONG _bytes;                                                               \
    ULONG64 _adr = addr;                                                        \
    char *_loc = (char *)(location);                                            \
    ULONG _len = sizeof(*(location));                                           \
    while(_len > 1024) {                                                        \
        if( CheckControlC() ) return;                                           \
        if( !ReadMemory( _adr, _loc, 1024, &_bytes ) ) {                        \
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", \
                        s, (unsigned long long)addr );                          \
            return FALSE;                                                       \
        }                                                                       \
        _adr += 1024; _loc += 1024; _len -= 1024;                               \
    }                                                                           \
    if (_len > 0) {                                                             \
        if( !ReadMemory( _adr, _loc, _len, &_bytes ) ) {                        \
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", \
                        s, (unsigned long long)addr );                          \
            return FALSE;                                                       \
        }                                                                       \
    }                                                                           \
    return TRUE;                                                                \
}

/*
 * Changed from macro to function. DIMS 160244.
 * Previously, there was "return (nothing)" statement which was causing the calling function to return
 * without freeing the allocated memory, in case of failure.
*/
static __inline BOOL ReadInLen(ULONG_PTR addr, char* s, void* location, ULONG len)
{
    ULONG _bytes;
    ULONG_PTR _adr = addr;
    char *_loc = (char *)(location);
    ULONG _len = len;
    while(_len > 1024) {
        if( CheckControlC() ) return FALSE;
        if( !ReadMemory( _adr, _loc, 1024, &_bytes ) ) {
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", s,
                        (unsigned long long)addr );
            return FALSE;
        }
        _adr += 1024; _loc += 1024; _len -= 1024;
    }
    if (_len > 0) {
        if( !ReadMemory( _adr, _loc, _len, &_bytes ) ) {
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", s,
                        (unsigned long long)addr );
            return FALSE;
        }
    }
    return TRUE;
}

#define ReadInStr(addr, s, location, maxlen) \
{ \
    ULONG _bytes; \
    ULONG64 _adr = addr; \
    void *_loc = (location); \
    ULONG _len = 0; \
    ULONG done = 0; \
    while(done == 0 && _len < maxlen) { \
        if( CheckControlC() ) return; \
        if( !ReadMemory( _adr, _loc, 1, &_bytes ) ) { \
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", \
                        s, (unsigned long long)addr ); \
            return; \
        } \
        if (*(char *)_loc == '\0') \
            { \
                done = 1; \
            } \
        _adr += 1; _loc = (char *)_loc + 1; _len++; \
    } \
}

#define ReadInStr_RT(addr, s, location, maxlen) \
{ \
    ULONG _bytes; \
    ULONG64 _adr = addr; \
    void *_loc = (location); \
    ULONG _len = 0; \
    ULONG done = 0; \
    while(done == 0 && _len < maxlen) { \
        if( CheckControlC() ) return 0; \
        if( !ReadMemory( _adr, _loc, 1, &_bytes ) ) { \
            nt_dprintf( "Read %s @ 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed.\n", s, addr ); \
            return 0; \
        } \
        if (*(char *)_loc == '\0') \
            { \
               done = 1; \
            } \
        _adr += 1; _loc = (char *)_loc + 1; _len++; \
    } \
}

// reads in the specified struct member value
#define Get_Struct_Member(struct_type, symbol_sn, struct_member) \
{ \
    ULONG64 _addr = GetExpression(symbol_sn); \
    ReadIn(_addr + offsetof(struct_type, struct_member), \
        symbol_sn "." #struct_member, &struct_member); \
}

// reads in array element value from specified struct member
#define Get_Array_Element_From_Struct(struct_type, symbol_sn, struct_member, element) \
{ \
    ULONG64 _addr = GetExpression(symbol_sn); \
    ReadIn(_addr + offsetof(struct_type, struct_member) + \
        (sizeof(struct_member) * element), symbol_sn "." #struct_member, \
        &struct_member); \
}
    

bool Expr_In (char *s, size_t sizeA, void* destPA);

INT_PTR GetArgument(const char * String, ULONG Count);
LONGLONG GetArgument64(const char * String, ULONG Count);
ULONGLONG atoi64(const char * string);

bool Read_In (void *addrPA, const char *const symbolPA,
              void *destPA, size_t sizeA);

bool
Read_In64 (ULONG64 addrPA, const char *const symbolPA,
             void* destPA, size_t sizeA);


bool Read_In_Str (void *addrPA, const char *const symbolPA, void *destPA,
                  size_t sizeA);

#define CMIDRV(x) ExprIn("CMI!" #x,&x)
#define CMIDDRV(x) ExprIn("CMID!" #x,&x)

#define CMISCDDRV(x) ExprIn("CMISCD!" #x,&x)
#define CMISCDDRV2(x,y) ExprIn("CMISCD!" #x,&y)
#define CMISCDDRV_ADDR(x) GetExpression("CMISCD!" #x)
#define CMISCDDRV_PTR(x) {CMISCDDRV(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

#define CMIPCIDRV(x) ExprIn("CMIPCI!" #x,&x)
#define CMIPCIDRV2(x,y) ExprIn("CMIPCI!" #x,&y)
#define CMIPCIDRV_ADDR(x) GetExpression("CMIPCI!" #x)
#define CMIPCIDRV_PTR(x) {CMIPCIDRV(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

#define PCIHALPLXDRV(x) ExprIn("PCIHALPLX!" #x,&x)
#define PCIHALPLXDRV2(x,y) ExprIn("PCIHALPLX!" #x,&y)
#define PCIHALPLXDRV_ADDR(x) GetExpression("PCIHALPLX!" #x)
#define PCIHALPLXDRV_PTR(x) {PCIHALPLXDRV(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

#define PCIHALEXPDRV(x) ExprIn("PCIHALEXP!" #x,&x)
#define PCIHALEXPDRV2(x,y) ExprIn("PCIHALEXP!" #x,&y)
#define PCIHALEXPDRV_ADDR(x) GetExpression("PCIHALEXP!" #x)
#define PCIHALEXPDRV_PTR(x) {PCIHALEXPDRV(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

#define CMM_ADDR(x) GetExpression("CMM!" #x)
#define CMM(x) ExprIn("CMM!" #x,&x)
#define CMM2(x,y) ExprIn("CMM!" #x,&y)
#define CMM_PTR(x) {CMM(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

#define HBDDRV(x) ExprIn("HBD!" #x,&x)
#define HBDDRV2(x,y) ExprIn("HBD!" #x,&y)
#define HBDDRV_ADDR(x) GetExpression("HBD!" #x)
#define HBDDRV_PTR(x) {HBDDRV(x);if(x){ReadIn((ULONG)x,#x,&_##x);x=&_##x;}else{nt_dprintf(#x "=NULL\n");}}

/*Macro to Read addresses for pointer to make it 64 and 32 bit compatible*/
#define READMEMORY_ADDR(addr,dest,label) {\
ULONG _bytes = 0;\
ULONG size_ptr = IsPtr64() ? SIZE_PTR_64 : SIZE_PTR_32;\
ReadMemory(addr,dest,size_ptr,&_bytes);\
if(_bytes != size_ptr){\
        nt_dprintf("Error:Failed to Read address from 0x" DBGEXT_64BIT_PRINTF_MODIFIER" for "#label"\n", \
                   (unsigned long long)addr);\
}\
}


// CheckControlC() macro to check/set globalCtrlC
// Note: In CheckVersion() globalCtrlC = FALSE;
#ifdef ALAMOSA_WINDOWS_ENV
#undef CheckControlC
extern BOOL globalCtrlC;
#define CheckControlC()    (((ExtensionApis.lpCheckControlCRoutine)() || globalCtrlC) ? (globalCtrlC = 1) : 0)
#endif /* ALAMOSA_WINDOWS_ENV - debug extension differences */

static __inline void set_global_control_c(void)
{
    /* Simply set the global control c value based on 
     * calling check control c.  This way of doing this operation 
     * avoids ambiguity in the way globalCtrlC is set since it 
     * also can get set inside of CheckControlC(). 
     */
    BOOL new_global_control_c = CheckControlC();
    globalCtrlC = new_global_control_c;
    return;
}
/**2  Macro To handle ReadMemory Api **/
#define READMEMORY(source, destination, size)\
{\
    ULONG _bytes = 0;\
    ReadMemory(source, destination, size, &_bytes);\
    if(size != _bytes)\
    {\
        nt_dprintf("ReadMemory at 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed\n", \
                   (unsigned long long)source);\
        return;\
    }\
}
 
#define READMEMORY_RT(source, destination, size)\
{\
    ULONG _bytes = 0;\
    ReadMemory(source, destination, size, &_bytes);\
    if(size != _bytes)\
   {\
       nt_dprintf("ReadMemory at 0x" DBGEXT_64BIT_PRINTF_MODIFIER" failed\n", \
                  (unsigned long long)source);\
       return 0;\
   }\
}

static __inline void retrieve_global_tick_count(EMCPAL_LARGE_INTEGER* pTickCount) 
{
#define NT_USER_SHARED_DATA CSX_CONST_U64(0xFFFFF78000000000)
#define NT_TICK_COUNT_POINTER (NT_USER_SHARED_DATA + 0x320)
    if(IsPtr64())
    {
        READMEMORY(NT_TICK_COUNT_POINTER, pTickCount, sizeof(EMCPAL_LARGE_INTEGER));
    }
    else
    {
        ExprIn("nt!KeTickCount", pTickCount);
    }
}

static __inline void retrieve_global_time_increment(PULONG pTimeIncrement) 
{
    ULONG64 NtBaseAddress, pNtBase_Addr, PU_TimeIncrement;
  
    pNtBase_Addr = GetExpression("scsitarg!TcdNtBaseAddress");
    READMEMORY(pNtBase_Addr, &NtBaseAddress, sizeof(ULONG64));

#define KeTimeIncrement_Offset 0x2A8084
#define KeMaximumIncrement_Offset 0x2A8028
#define NT_TimeIncrement (NtBaseAddress + KeTimeIncrement_Offset)
#define NT_MaximumIncrement (NtBaseAddress + KeMaximumIncrement_Offset)

    if(IsPtr64())
    {
        PU_TimeIncrement = NT_MaximumIncrement;
    }
    else
    {
        PU_TimeIncrement = GetExpression("nt!KeMaximumIncrement");
    }
    READMEMORY(PU_TimeIncrement,pTimeIncrement,sizeof(ULONG));
}

static __inline void retrieve_global_system_time(EMCPAL_LARGE_INTEGER* pSystemTime) 
{
#define NT_USER_SHARED_DATA CSX_CONST_U64(0xFFFFF78000000000)
#define NT_SYSTEM_TIME_POINTER (NT_USER_SHARED_DATA + 0x14)
    EMCPAL_LARGE_INTEGER TickCount;
    ULONG TimeIncrement;

    if(IsPtr64())
    {
        READMEMORY(NT_SYSTEM_TIME_POINTER, pSystemTime, sizeof(EMCPAL_LARGE_INTEGER));
    }
    else
    {
        retrieve_global_tick_count(&TickCount);
        retrieve_global_time_increment(&TimeIncrement);
        pSystemTime->QuadPart = TickCount.QuadPart * TimeIncrement;
    }
}

static __inline BOOLEAN PointerInKernelSpace(ULONG64  MemPtr)
{
    BOOLEAN     RetVal              = FALSE;
    csx_size_t  Size                = 0;
    ULONG64     StartOfKernelSpace  = 0;

    Size = GetTypeSize("EmcPAL!" "PCHAR");

    if( Size == 4 )
    {
        StartOfKernelSpace = (ULONG64)0x80000000;
    }
    else if ( Size == 8 )
    {
        StartOfKernelSpace = (ULONG64)0xFFFF080000000000ULL;
    }
    else
    {
        nt_dprintf("Error in determining kernel memory space\n");
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
}

#endif /* USE_CSX_DBGEXT */

#endif // __DBG_EXT_H__
