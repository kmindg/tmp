#ifndef _KTRACE_H_
#define _KTRACE_H_

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *              ktrace.h
 ***************************************************************************
 *
 * Description:
 *
 *   Defines and constants for the ktrace driver
 *
 * Notes:
 *
 *   Low overhead trace information in circular buffer
 *
 *   TRACE_RECORD has TSTAMP first to force alignment.
 *   Both structures are 32 bytes long.
 *
 * History:
 *
 *   Greg Schaffer 99/08/12, Feb 2001
 *   Jim Williams  2000, 2001 
 *   David Union   Feb 2001
 *   Anshuman Ratnani Apr 2005
 *   Gary Peterson Feb 2006
 ***************************************************************************/

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif


#include "stdarg.h" // Required for va_list
#include "bool.h"   // Required for use of bool below
#include "csx_ext.h" // required for CSX_MOD_IMPORT
#include "EmcPAL_Configuration.h"

#if !defined(ALAMOSA_WINDOWS_ENV) || defined(EMCPAL_CASE_C4_OR_WU)
#define KTRACE_FORWARD_OUTPUT
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - output wiring differences */

/*
 * CONDITIONALS
 *
 *  _GRASP_IGNORE        -- set when processing under "prettyprint" jGRASP
 *  _KTRACE_             -- define inside ktrace - "export" DLL linkage
 *  KT_STR_LEN           -- string length for KvTrace* svprintf
 *  KTRACE_RING_CNT      -- number of separate rings supported
 *  KTRACEAPI            -- prefix for declaration of external functions
 *  NO_KTRACE            -- control inclusion/exclusion of active tracing
 */

/* C O N D I T I O N A L S */

#if !defined(KTRACE_RING_CNT)   /* number of supported rings */
#define KTRACE_RING_CNT  24
#endif

// Note: The above value defines now many rings CAN exist, not
//       necessarily how many rings DO exist.
//
//       This value is kept large so that when rings are added,
//       a new version of ktcons.exe (which is run outside the
//       SP a lot) isn't needed.
//
//   This value must be greater than the TRC_K_MAX enum below.
//       
//       Should this value need to be raised in the future, it
//       it strongly encouraged that it raise in larger increments
//       to allow for future growth, for the reasons stated above.
//       (don't add 1, add 5).
//       
//       GSP, 2/06

#if !defined(KT_STR_LEN)    /* length for expanded strings */
#define KT_STR_LEN 128
#endif
#define KT_FUNCTION_STR_LEN 32
#define KT_LINE_STR_LEN 16

/*
 * Note, UMODE_ENV links ktrace.lib as a static library,
 * WHile SIMMODE_ENV links to ktrace.dll
 */
#if defined(UMODE_ENV) || defined(_GRASP_IGNORE) || defined(DISABLE_KTRACE_IMPORTS)

#define KTRACEAPI
#else
#if !defined(_KTRACE_)
#define KTRACEAPI CSX_MOD_IMPORT
#else
#define KTRACEAPI CSX_MOD_EXPORT
#endif
#endif

/* T Y P E D E F s */
/*
 * KTRACE_ring_id_T
 *
 * DESCRIPTION: This enumeration type defines the codes used to identify the
 *   different ring buffers.  The highest ring buffer number is also assigned
 *   to TRC_K_MAX.  TRC_K_MAX must be less than KTRACE_RING_CNT.
 *
 */
typedef enum KTRACE_ring_id_enum
{
   TRC_K_STD        = 0,    /* standard ("low-speed") ring buffer */
   TRC_K_START      = 1,    /* startup ring buffer */
   TRC_K_IO         = 2,    /* io high-speed ring buffer */
   TRC_K_USER       = 3,    /* user-level application ring buffer */
   TRC_K_TRAFFIC    = 4,    /* host traffic ring buffer */
   TRC_K_CONFIG     = 5,    /* Hostside configuration ring buffer */
   TRC_K_REDIR      = 6,    /* Redirector debug ring buffer */
   TRC_K_EVT_LOG    = 7,    /* event log ring buffer */
   TRC_K_PSM        = 8,    /* PSM activity ring buffer */
   TRC_K_ENV_POLL   = 9,    /* Environmental "polling" ring buffer */
   TRC_K_XOR_START  = 10,   /* First `n' XOR events */
   TRC_K_XOR        = 11,   /* Last `m' XOR events */
   TRC_K_MLU        = 12,   /* MLU / CBFS */
   TRC_K_PEER       = 13,   /* Peer Ktrace ring buffer */
   TRC_K_CBFS       = 14,   /* CBFS ring buffer */
   TRC_K_RBAL       = 15,   /* RBAL ring buffer */
   TRC_K_THRPRI     = 16,   /* system thread priority buffer */
   TRC_K_SADE       = 17,   /* Dart log ring buffer */
   TRC_K_MAX        = TRC_K_SADE,  /* highest valid buffer id */
   TRC_K_NONE       = 0xff  /* Invalid ktrace buffer */
} KTRACE_ring_id_T;

// Note: As stated above, the value of KTRACE_RING_CNT must be
//       greater than TRC_K_MAX.
//       This #if's enforces that policy.

#if defined(TRC_K_MAX) && (TRC_K_MAX >= KTRACE_RING_CNT)
#error KTRACE_RING_CNT is not greater than TRC_K_MAX
#endif


/*
 * KUTRACE_INFO_NO_ARG & KUTRACE_INFO_ARG
 *
 * DESCRIPTION: This structure is used as the input for IOCTL_KTRACE_KUTRACEX.
 * The difference between these two structures is ktrace arguments.
 *
 */
#pragma pack(1)
typedef struct _kutrace_info_no_arg
{
    csx_u64_t   ring_id;
    csx_char_t  message[KT_STR_LEN];
    csx_u64_t   length;
} KUTRACE_INFO_NO_ARG;

typedef struct _kutrace_info_arg
{
    csx_u64_t   ring_id;
    csx_char_t  message[KT_STR_LEN];
    csx_u64_t   length;
    csx_u64_t   a4[4];
} KUTRACE_INFO_ARG;
#pragma pack()


/*
 * Special values for "id", otherwise used as "fmt" string pointer
 */
#define KT_NULL     0x00000000
#define KT_TIME     0x00000001
#define KT_LOST     0x00000002
#define KT_FILE_OPEN_WRITE 0x00000003
#define KT_FILE_OPEN_READ  0x00000004
#define KT_FILE_CLOSE      0x00000005
#define KT_FILE_CLOSED     0x00000006
#define KT_TCD_TRAFFIC     0x00000007
#define KT_PSM_TRAFFIC     0x00000008
#define KT_LUN_TRAFFIC     0x00000009
#define KT_FRU_TRAFFIC     0x0000000a
#define KT_DBE_TRAFFIC     0x0000000b
#define KT_SFE_TRAFFIC     0x0000000c /* 29-APR-2005. DIMS-124667. Add tracing at SFE to log IOs in the ktrace traffic ring buffer*/
#define KT_DML_TRAFFIC     0x0000000d
#define KT_MVS_TRAFFIC     0x0000000e
#define KT_MVA_TRAFFIC     0x0000000f
#define KT_MLU_TRAFFIC     0x00000010
#define KT_SNAP_TRAFFIC    0x00000011         //DIMS 188125 RBA tracing
#define KT_AGG_TRAFFIC     0x00000012      
#define KT_MIG_TRAFFIC     0x00000013     
#define KT_CLONES_TRAFFIC  0x00000014         //DIMS 188132 SnapClones RBA tracing  Check this!!!
#define KT_CPM_TRAFFIC     0x00000015
#define KT_FEDISK_TRAFFIC  0x00000016
#define KT_COMPRESSION_TRAFFIC      0x00000017 // DIMS 248727 Compression RBA tracing
#define KT_FBE_TRAFFIC     0x00000018
#define KT_FCT_TRAFFIC     0x00000019
#define KT_FBE_LUN_TRAFFIC     0x0000001A
#define KT_FBE_RG_FRU_TRAFFIC  0x0000001B
#define KT_FBE_RG_TRAFFIC      0x0000001C
#define KT_FBE_LDO_TRAFFIC     0x00000020
#define KT_FBE_PDO_TRAFFIC     0x00000021
#define KT_FBE_PORT_TRAFFIC     0x00000022
#define KT_FBE_VD_TRAFFIC       0x00000023
#define KT_FBE_PVD_TRAFFIC      0x00000024
#define KT_DDS_TRAFFIC          0x00000025
#define KT_SPC_TRAFFIC          0X00000026
#define KT_CBFS_TRAFFIC         0X00000027
#define KT_CCB_TRAFFIC          0x00000028
#define KT_MPT_TRAFFIC          0x00000029
#define KT_MCIO_TRAFFIC         0x0000002A
#define KT_TDX_TRAFFIC          0x0000002B
#define KT_LAST           (KT_TDX_TRAFFIC+1) 

/*
 * function declarations
 */
#if defined(SIMMODE_ENV)
KTRACEAPI void KtraceStartTracing (KTRACE_ring_id_T ringA, csx_pchar_t traceFileName);
KTRACEAPI void KtraceUninit(void);
#endif //defined(SIMMODE_ENV)

KTRACEAPI char * _KtCprint(KTRACE_ring_id_T buf, 
                           const char *fmt, 
                           va_list argptr);
KTRACEAPI void Ktrace(csx_u64_t id,
              csx_u64_t a0,
              csx_u64_t a1,
              csx_u64_t a2,
              csx_u64_t a3);
KTRACEAPI void Ktracex(KTRACE_ring_id_T buf,
               csx_u64_t id,
               csx_u64_t a0,
               csx_u64_t a1,
               csx_u64_t a2,
               csx_u64_t a3);
/*
 * varargs tracing - replace DbgPrint (see ntddk.h) with KvPrint 
 */
KTRACEAPI void KvTrace(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI void KvTracep(const char *fmt, va_list argptr) __attribute__((format(__printf_func__,1,0)));
KTRACEAPI void KvTraceStart(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI void KvTraceIo(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI void KvTracex(const KTRACE_ring_id_T buf, const char *fmt, ...) __attribute__((format(__printf_func__,2,3)));
KTRACEAPI void KvTracexp(const KTRACE_ring_id_T buf, const char *fmt, va_list ap) __attribute__((format(__printf_func__,2,0))); 
KTRACEAPI void KvPrintCopyPeerp(const char *fmt, va_list ap) __attribute__((format(__printf_func__,1,0)));
KTRACEAPI unsigned long KvPrint(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI unsigned long KvPrintStart(const char *fmt, ...) __attribute__((format(__printf_func__,1,0)));
KTRACEAPI unsigned long KvPrintIo(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI unsigned long KvPrintx(const KTRACE_ring_id_T buf, const char *fmt, ...) __attribute__((format(__printf_func__,2,3)));
KTRACEAPI unsigned long KvPrintxp(const KTRACE_ring_id_T buf, const char *fmt, va_list ap) __attribute__((format(__printf_func__,2,0)));
KTRACEAPI unsigned long NtDbgPrint(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI unsigned long NtSimpleDbgPrint(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));

KTRACEAPI void KvTraceCopyPeer(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI void KvPrintCopyPeer(const char *fmt, ...) __attribute__((format(__printf_func__,1,2)));
KTRACEAPI void KvTraceToPeerRegister(void (*xmit_fcn)(const char *,unsigned long));
KTRACEAPI long KtraceEntriesRemaining(const KTRACE_ring_id_T buf);
KTRACEAPI void KvTraceLocation(const KTRACE_ring_id_T buf, bool trace_location,
                               const char *function, unsigned long line, 
                               const char *fmt, va_list argptr) __attribute__((format(__printf_func__,5,0)));

KTRACEAPI void KvPanicToPeerRegister(bool (*xmit_panic_fcn)(unsigned long));
KTRACEAPI bool KvOkToPanic(unsigned long tiebreaker);

#define KT_PANIC_CLEAR  0   /*  The local peer is up and running.   */
#define KT_PANIC_NOW    1   /*  The peer is going down and can not wait.    */

KTRACEAPI void KvOutputAssertInformation(const char* expression, const char* filename, const unsigned int linenumber, 
                                         const void* thisPtr, const void* saveArea, const char* msg);

/*
 * Utility function(s) used to process messages put in ktrace buffers
 */
KTRACEAPI char * catmergeFile(const char *);

/* M A C R O   F U N C T I O N   D E F I N I T I O N S */

#ifdef NO_KTRACE
#define KHERE()
#define KHEREX(buf)
#define KPRINT(id, ...)
#define KPRINTX(buf, id, ...)
#define KTRACE(id, a0, a1, a2, a3)
#define KTRACEX(buf, id, a0, a1, a2, a3)
#else /* NO_KTRACE */

// KHERE() records SystemTime, __FILE__, __LINE__
#define KHERE() {                   \
    EMCPAL_LARGE_INTEGER clock;                \
    EmcpalQuerySystemTime(&clock);          \
    KTRACE(KT_TIME, clock.LowPart, clock.HighPart,  \
           catmergeFile(__FILE__), __LINE__);           \
}
#define KHEREX(buf) {                       \
    EMCPAL_LARGE_INTEGER clock;                    \
    EmcpalQuerySystemTime(&clock);              \
    KTRACEX(buf, KT_TIME, clock.LowPart, clock.HighPart,    \
        catmergeFile(__FILE__), __LINE__);              \
}
// KPRINT now uses KvPrint, to both trace & print
#define KPRINT  KvPrint
#define KPRINTX KvPrintx

#define KTRACE(id, a0, a1, a2, a3)      \
    Ktrace((ULONG64)(id),       \
           (ULONG64)(a0),       \
           (ULONG64)(a1),       \
           (ULONG64)(a2),       \
           (ULONG64)(a3))

#define KTRACEX(buf, id, a0, a1, a2, a3)    \
    Ktracex(buf,                \
        (ULONG64)(id),      \
        (ULONG64)(a0),      \
        (ULONG64)(a1),      \
        (ULONG64)(a2),      \
        (ULONG64)(a3))
#endif /* NO_KTRACE */

#ifdef KTRACE_FORWARD_OUTPUT
/*
 * 
 */
KTRACEAPI bool KtraceConsoleEnable(bool state);

/*
 * Set KTrace output function
 */
KTRACEAPI void KtraceSetPrintfFunc(void (* func)(const char *, ...));

/*
 * Set Ktrace prefix. Initialized to SPA/SPB to differentiate console output 
 */
KTRACEAPI const char *KtraceSetPrefix(const char *prefix);
#endif

#ifdef __cplusplus
};
#endif

#endif /* _KTRACE_H_ */
