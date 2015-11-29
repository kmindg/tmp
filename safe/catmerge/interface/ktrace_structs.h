/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef __KTRACE_STRUCT_H__
#define __KTRACE_STRUCT_H__

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 */
extern "C" {
#endif

// Ktrace_Struct.h
// This file contains the structure information for the
// ktrace records, and ring info extracted from ktrace.h
//
/*
 * CONDITIONALS
 *
 *  _GRASP_IGNORE        -- set when processing under "prettyprint" jGRASP
 *  KT_TABLE_S           -- control generation of Kt_table_s[]
 *                                            and Kt_ring_s[]
 *  KT_TABLE_Z           -- control generation of Kt_size[]
 *  UMODE_ENV            -- user/kernel environment selector
 */

/* Store CPU Number in low 4 bits of tr_thread */
#define KT_CPU_MASK 0xF

typedef __int64 TSTAMP;

/*
 * TRACE_RECORD
 *
 * DESCRIPTION: This structure describes the layout of a trace ring buffer
 *   entry.
 *
 *    +----+----+----+----+----+----+----+----+
 *  0:|tr_stamp                               | time stamp
 *    +----+----+----+----+-------------------+
 *  8:|tr_thread          |                     thread id
 *    +----+----+----+----+
 * 16:|tr_id              |                     code or string address
 *    +----+----+----+----+----+----+----+----+
 * 24:|tr_a0                                  | arbitrary argument
 *    +----+----+----+----+----+----+----+----+
 * 32:|tr_a1                                  | arbitrary argument
 *    +----+----+----+----+----+----+----+----+
 * 40:|tr_a2                                  | arbitrary argument
 *    +----+----+----+----+----+----+----+----+
 * 48:|tr_a3                                  | arbitrary argument
 *    +----+----+----+----+----+----+----+----+
 * 56:
 */
typedef struct _trace_record
{
  TSTAMP   tr_stamp;
  ULONG64  tr_thread;
  ULONG64  tr_id;
  ULONG64  tr_a0;
  ULONG64  tr_a1;
  ULONG64  tr_a2;
  ULONG64  tr_a3;
} TRACE_RECORD;

/*
 * TRACE_INFO
 *
 * DESCRIPTION: This structure describes the layout of a trace ring
 *   header.
 *
 * NOTE: If this structure changes in a backwards compatible way, increment
 *   the value of KTRC_info_MINOR.  If this structure changes in a non-
 *   backwards compatible way, increment KTRC_info_MAJOR and zero
 *   KTRC_info_MINOR.
 *
 * NOTE: Functions adding items to ring buffer use ringwrap() to increment
 *   ti_cirbuf, NULL means inactive
 *   Functions writing to ti_cirbuf and ti_altbuf use an atomic 64-bit
 *   CMPXCHG8B instruction.
 *   Functions allocating and deallocating dynamic memory for the buffers
 *   use ti_allocP. Allocation routine must set ti_allocP, ti_altbuf, and
 *   ti_cirbuf in that order, deallocation in the reverse of that.
 *
 *   ??JFW? possible race condition on (de)allocation?
 *   
 *    +----+----+----+----+
 *  0:|ti_cirbuf          | ptr current active buffer address
 *    +----+----+----+----+
 *  4:|ti_altbuf          | ptr currently inactive buffer address (if 2 buffers)
 *    +----+----+---------+
 *  8:|ti_size            | ulong number of elements
 *    +----+----+----+----+
 * 12:|ti_slot            | ulong current element number
 *    +----+----+---------+
 * 16:|ti_flags |           ushort attribute flags
 *    +----+----+
 * 18:|ti_tflag |           ushort traffic flags
 *    +----+----+----+----+
 * 20:|ti_str_ofst        | size_t (uint) offset from beginning of ring buf to string buf
 *    +----+----+----+----+
 * 24:|ti_allocP ==> ti_ring1 | ptr base address of allocated buffer memory
 *    +----+----+----+----+
 * 28:|ti_hFile           | ptr backing store file handle (or INVALID_HANDLE_VALUE)
 *    +----+----+----+----+
 * 32:|ti_fileNameP       | ptr file name address
 *    +----+----+----+----+
 * 36:|ti_name            | ptr ring name address
 *    +----+----+----+----+
 * 40:|ti_text            | ptr ring text address
 *    +----+----+----+----+
 * 44:|ti_hwm             | size_t element number to trigger write (or 0xFFFFFFFF)
 *    +----+----+----+----+
 * 48:|ti_data_size       | size_t number of bytes in a backing store data write
 *    +----+----+----+----+
 * 52:|ti_str_size        | size_t number of bytes in a string entry
 *    +----+----+----+----+----+----+----+----+
 * 56:|ti_seek                                | __int64 current write position
 *    +----+----+----+----+----+----+----+----+
 * 64:|ti_flush           | ulong
 *    +----+----+---------+
 * 68:|ti_lost            | ulong 
 *    +----+----+----+----+
 * 72:|ti_id              char id number for this TRACE_INFO element
 *    +----+----+---------+
 *    +--+----+----+
 ti_alloc_size
 ti_ring_2
 ti_ring_2_ofst
 */

typedef struct _trace_info
{
  TRACE_RECORD*      ti_cirbuf; /* ptr to ring buffer, NULL == unavailable */
  TRACE_RECORD*      ti_altbuf; /* addr of inactive (only) ring buf */ 
  ULONG        ti_size; /* number of entries in ring buffer */
  ULONG        ti_slot; /* current position in ring buffer */
  unsigned int  ti_flags; /* KTRACE_flags_T status bits */
  size_t           ti_str_ofst; /* (->string buf) - (->ring buf) */
  csx_atomic64_t  ti_wrapcount; // keeping an eye on how much times ring been overflown (i.e. ti_slot wrapped)
  TRACE_RECORD*      ti_allocP; /* addr of allocated memory */
  HANDLE              ti_hFile; /* handle of backing file */
  csx_pchar_t        ti_fileNameP; /* ptr to file path/name.type */
  char*                ti_name; /* short name for ring buffer */
  char*                ti_text; /* descriptive text for ring buffer */
  size_t                ti_hwm; /* element number to trigger buffer write */
  size_t          ti_data_size; /* number of bytes in a "data write" for single buffer */
  size_t           ti_str_size; /* length of a string buffer entry */
  __int64              ti_seek; /* seek to before write if DO_SEEK set */
  LONG                ti_flush; /* flushing */
  LONG                 ti_lost; /* lost during wrap */
  char                   ti_id; /* id number for this element */
  size_t         ti_alloc_size; /* Size of memory to allocate for this ring */
  TRACE_RECORD*      ti_ring_2; /* pointer to ring_2 */
  size_t        ti_ring_2_ofst; /* offset to ring_2  */
  unsigned int  ti_tflag; /* traffic flags */  
} TRACE_INFO;


/*
 * macro used to statically init a TRACE_INFO structure
 */
#define TRACE_INFO_INIT(m_size, m_flags, m_name, m_text, m_hwm, m_id)   \
  {                                 \
    (TRACE_RECORD*)  NULL,                 /* ti_cirbuf */      \
    (TRACE_RECORD*)  NULL,                 /* ti_altbuf */      \
                     m_size,               /* ti_size */        \
                     0xFFFFFFFFuL,         /* ti_slot */        \
    (KTRC_flags_T)   m_flags,              /* ti_flags */       \
    (int)((m_flags & KTRC_DO_STR) != 0)                         \
      * (m_size) * sizeof (TRACE_RECORD),  /* ti_string_ofst */ \
    0,                                     /* ti_wrapcount */   \
    (TRACE_RECORD*)  NULL,                 /* ti_allocP */      \
                     INVALID_HANDLE_VALUE, /* ti_handle */      \
    (csx_pchar_t)       NULL,                 /* ti_fileNameP */   \
    (char*)          m_name,               /* ti_name */        \
    (char*)          m_text,               /* ti_text */        \
                     m_hwm,                /* ti_hwm */         \
                     0uL,                  /* ti_data_size */   \
                     0uL,                  /* ti_str_size */    \
    (__int64)        0,                    /* file seek pos */  \
                     0uL,                  /* ti_flush  */      \
                     0uL,                  /* ti_lost   */      \
    (char)           m_id,                 /* ti_id */          \
                     0uL,                  /* ti_alloc_size */  \
    (TRACE_RECORD*)  NULL,                 /* ti_ring2 */       \
                     0uL,                  /* ti_ring_2_ofst */ \
                     0,                    /* ti_tflag */       \
  }

typedef enum KTRC_flags_enum
{
  KTRC_CLR             = 0,                   /* empty                    */
  KTRC_IS_OVERRUN      = 1 <<  0,             /* 1 -- buffer overran      */
  KTRC_IS_FILE_OPEN    = 1 <<  1,             /* 1 -- file is open        */
  KTRC_IS_HEAD_WRITTEN = 1 <<  2,             /* 1 -- header written      */
  KTRC_IS_FILE_ERR     = 1 <<  3,             /* 1 -- I/O error           */
  /*                   = 1 <<  4, */
  KTRC_DO_DURATION   = 1 <<  5,             /* 1 -- duration set for file  */
  KTRC_DO_WRAP        = 1 <<  6,             /* 1 -- wrap curent file    */
  KTRC_DO_FLUSH        = 1 <<  7,             /* 1 -- do a flush write    */
  KTRC_DO_CLOSE        = 1 <<  8,             /* 1 -- close current file  */ 
  KTRC_DO_DISK         = 1 <<  9,             /* 1 -- write to disk       */
  KTRC_DO_SEEK         = 1 << 10,             /* 1 -- use ti_seek value   */
  KTRC_DO_INIT_OPEN    = 1 << 11,             /* 1 -- open file on init   */
  KTRC_DO_APPEND       = 1 << 12,             /* 1 -- open for append     */
  KTRC_DO_1BUF         = 1 << 13,             /* 1 -- do single buffering */
  KTRC_DO_2BUF         = 1 << 14,             /* 1 -- do double buffering */
  KTRC_DO_STR          = 1 << 15,             /* 1 -- expand strings      */
} KTRC_flags_T;

typedef enum KTRC_tflag_enum
{
    KTRC_TOFF          = 0,                   /* trace off - no traffic   */
    KTRC_TTCD          = 1 << 0,              /* trace TCD (host-side)    */
    KTRC_TPSM          = 1 << 1,              /* trace PSM                */
    KTRC_TLUN          = 1 << 2,              /* trace LUN (raid)         */
    KTRC_TFRU          = 1 << 3,              /* trace FRU (at rg +LUN)   */
    KTRC_TDBE          = 1 << 4,              /* trace DBE (at NTBE/DMRB) */
    KTRC_TSFE          = 1 << 5,              /* DIMS 124667. trace SFE IOs*/ 
    KTRC_TDML          = 1 << 6,              /* trace DM Library         */
    KTRC_TMVS          = 1 << 7,              /* trace MVS                */
    KTRC_TTDX          = 1 << 8,              /* trace TDX                */
    KTRC_TMLU_DEV      = 1 << 9,              /* trace MLU device IOs     */
    KTRC_TMLU_CBFS     = 1 << 10,             /* trace MLU CBFS requests  */
    KTRC_TSNAP		   = 1 << 11, 
    KTRC_TAGG          = 1 << 12,             /* trace Aggregate requests */
    KTRC_TMIG          = 1 << 13,             /* trace Migration requests */
    KTRC_TCBFS         = 1 << 14,             /* trace CBFS Internal requests  */
    KTRC_TCLONES       = 1 << 15,             /* trace CLONES - DIMS 188132 SnapClones RBA tracing  check this1!!*/ 
    KTRC_TCPM          = 1 << 16,             /* trace SAN Copy  */
    KTRC_TFED          = 1 << 17,             /* trace FEDisk  */
    KTRC_TCOMPRESSION  = 1 << 18,             /* trace Compression  */
    KTRC_TFBE          = 1 << 19,             /* trace FBE (at the shim) */
    KTRC_TFCT          = 1 << 20,             /* trace Flash Cache */
    KTRC_TPDO          = 1 << 21,             /* trace the FBE Physical Drive Object */
    KTRC_TVD           = 1 << 22,             /* trace the FBE Virtual Drive Object */
    KTRC_TPVD          = 1 << 23,             /* trace the FBE Provisional Drive Object */
    KTRC_TDDS          = 1 << 24,             /* trace the dedup server(DDS) IO*/
	KTRC_TSPC          = 1 << 25,             /* trace SP Cache */
    KTRC_TFBE_LUN      = 1 << 26,             /* trace the FBE LUN Object */
    KTRC_TFBE_RG_FRU   = 1 << 27,             /* trace the FBE RG Object per FRU*/
    KTRC_TCCB          = 1 << 28,             /* trace for CCB  traffic */ 
    KTRC_TFBE_RG       = 1 << 29,             /* trace for FBE RG Object */ 
    KTRC_TMPT          = 1 << 30,             /* trace for MPT  traffic */
    KTRC_TMCIO         = 1 << 31,             /* trace for MCIO  traffic */
} KTRC_tflag_T;

/*
 * KTRC_info_T
 *
 * DESCRIPTION: This structure contains the header information shared
 *   between nest\ktrace\* files and nest\disk\fdb\* files relating to
 *   tracing data.
 *
 * NOTE: If this structure changes in a backwards compatible way, increment
 *   the value of KTRC_info_MINOR.  If this structure changes in a non-
 *   backwards compatible way, increment KTRC_info_MAJOR and zero
 *   KTRC_info_MINOR.
 *
 *    +----+----+----+----+----+----+----+----+
 *  0:|magic == KTRC_info_MAGIC == "KtrcInfo" |
 *    +----+----------------------------------+
 *  8:|    | major == KTRC_info_MAJOR
 *    +----+
 *  9:|    | minor == KTRC_info_MINOR
 *    +----+
 * 10:|    | ring_cnt == KTRACE_RING_CNT
 *    +----+
 * 11:|    | reserved0
 *    +----+----+----+----+
 * 12:|info_size          |
 *    +----+----+----+----+----+----+----+----+
 * 16:|rtc_freq                               |
 *    +----+----+----+----+----+----+----+----+
 * 24:|stamp                                  |
 *    +----+----+----+----+----+----+----+----+
 * 32:|system_time                            |
 *    +----+----+----+----+-------------------+
 * 40:|reserved1          |
 *    +----+----+----+----+
 * 44:|reserved2          |
 *    +----+----+----+----+ 
 * 48: TRACE_INFO[]
 *
 */
typedef struct KTRC_info_struct
{
  char magic[8];  /* identification pattern == KTRC_info_MAGIC              */
  char major;     /* major release; change on incompatible change           */
  char minor;     /* minor release; change on   compatible change           */
  char ring_cnt;  /* number of rings  == KTRACE_RING_CNT                    */
  char reserved0;
  size_t       info_size;        /* this struct size == sizeof(KTRC_info_T) */
  __int64      rtc_freq;         /* cpu clock frequency                     */
  __int64      stamp;            /* a timestamp value                       */
  __int64      system_time;      /* system date / time equal to timestamp   */
  int          reserved1;
  int          reserved2;
  TRACE_INFO   info[KTRACE_RING_CNT]; /* ring header structures             */
} KTRC_info_T;

#define KTRC_info_MAGIC "KtrcInfo"
#define KTRC_info_MAJOR 1
#define KTRC_info_MINOR 0
#define KTRC_info_CNT   KTRACE_RING_CNT
#define KTRC_info_SIZE  sizeof (KTRC_info_T)

extern  KTRC_info_T Ktrace_info;

/*
 * KTRC_file_head_T
 *
 *    +----+----+----+----+----+----+----+----+
 *  0:|magic == ..._MAGIC == "KtrcBack"       |
 *    +----+----------------------------------+
 *  8:|    | major == ..._MAJOR == 1
 *    +----+
 *  9:|    | minor == ..._MINOR == 0
 *    +----+
 * 10:|    | ring_id
 *    +----+
 * 11:|    | reserved0
 *    +----+----+----+----+
 * 12:|head_size          |
 *    +----+----+----+----+
 * 16:|data_size          |
 *    +----+----+----+----+
 * 20:|ring_size          |
 *    +----+----+----+----+
 * 24:|string_size        |
 *    +----+----+----+----+
 * 18:|elements           |
 *    +----+----+----+----+----+----+----+----+
 * 32:|rtc_freq                               |
 *    +----+----+----+----+----+----+----+----+
 * 40:|stamp                                  |
 *    +----+----+----+----+----+----+----+----+
 * 48:|system_time                            |
 *    +----+----+----+----+-------------------+
 * 56:|name_ofst|     
 *    +----+----+
 * 58:|text_ofst|
 *    +----+----+----+----+
 * 60:|base_addr          |
 *    +----+----+----+----+
 * 64:
 */

typedef struct KTRC_file_head
{
  char magic[8];  /* identification pattern == KTRC_head_file_MAGIC         */
  char major;     /* major release == 1; change on incompatible change      */
  char minor;     /* minor release == 0; change on   compatible change      */
  char ring_id;   /* id number of this ring                                 */
  char reserved0;
  int  reserved1; /* align for 64-bit size_t, even for 32-bit dbgext        */
  ULONG64      head_size;        /* bytes in header, bytes to first ring    */
  ULONG64      data_size;        /* bytes in a data write                   */
  ULONG64      ring_size;        /* bytes in a ring buf                     */
  ULONG64      string_size;      /* bytes in a string buf                   */
  unsigned int elements;         /* number of elements in a ring/string buf */
  unsigned int reserved2;
  __int64      rtc_freq;         /* cpu clock frequency                     */
  __int64      stamp;            /* a timestamp value                       */
  __int64      system_time;      /* system date / time equal to timestamp   */
  unsigned short int name_ofst;  /* offset (bytes) in file to ring name     */
  unsigned short int text_ofst;  /* offset (bytes) in file to ring text     */
  unsigned int base_addr;        /* base address of first ring buffer       */
  __int64      wrap_pos;         /* seek for oldest entry on wrap           */
  __int64      relative_time;    /* starting trace time to support split    */
} KTRC_file_head_T;

#define KTRC_file_head_MAGIC "KtrcBack"
#define KTRC_file_head_MAJOR 8
#define KTRC_file_head_MINOR 0
#define HEAD_SIZE 512

// 64K * 56 bytes(~trace record) = 3.5MB
//minimum trace record should be 5MB , 1record = 3.5MB
#define KTRACE_MIN_BUFF_SIZE  5

// 2GB default buffer size  
#define KTRACE_DEFAULT_BUFF_SIZE  2048

// Maximum Size supported is 10GB
#define KTRACE_MAX_BUFF_SIZE  10240

#define ONE_MB (1 * 1024 * 1024) // 1 MB (in bytes)

#define DEFAULT_PRIORITY     8
#define ELEVATED_PRIORITY1 9
#define ELEVATED_PRIORITY2 10
#define ELEVATED_PRIORITY3 11
#define ELEVATED_PRIORITY4 12
#define ELEVATED_PRIORITY5 13
#define ELEVATED_PRIORITY6 14
#define HIGHEST_PRIORITY    15
#define REALTIME_PRIORITY  16

KTRACEAPI KTRC_info_T *KtraceInit( void );
KTRACEAPI TRACE_INFO  *KtraceInitAll( void );
KTRACEAPI TRACE_INFO  *KtraceInitN(const KTRACE_ring_id_T buf);

#ifdef KT_TABLE_S
// Display of trace records based on id range
// (id < KT_LAST) -- KTRACE(KT_xxx, ...) -- format string KT_table_s[id]
// (id > KT_LAST) -- KTRACE("string", ...) -- constant string, 4 hex args
//
char *KT_table_s[KT_LAST] =
{
   "NULL trace entry",             // KT_NULL
   "System Time %08llx:%08llx  %llx:%lld", // KT_TIME
   "Lost %lld",               // KT_LOST
   "Transaction File Open Write",  // KT_FILE_OPEN_WRITE
   "Transaction File Open Read",   // KT_FILE_OPEN_READ
   "Transaction File Start Close", // KT_FILE_CLOSE
   "Transaction File Done Close",  // KT_FILE_CLOSED
   "TCD: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "PSM: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "LUN: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "FRU: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "DBE: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "SFE: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.", /* DIMS 124667. ARatnani */
   "DML: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "MVS: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "MVA: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "MLU: LBA 0x%08llX%08llX  Blks %6llx  LU/Cmd  0x%08llX.",
   "SNAP: LBA 0x%08llX%08llX  Blks %6llx  LU/Cmd  0x%08llX.", //DIMS 188125 RBA tracing
   "AGG: LBA 0x%08llX%08llX  Blks %6llx  LU/Cmd  0x%08llX.",
   "MIG: LBA 0x%08llX%08llX  Blks %6llx  LU/Cmd  0x%08llX.",
   "CLONE: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.", //DIMS 188132 SnapClones RBA tracing
   "CPM: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "FEDISK: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "COMPRESSION: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "FBE: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "FCT: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%016llX.",
   "LUN: LBA 0x%016llX %llX  OBJ/Blks %016llX  Cmd 0x%016llX.",
   "FRU: LBA 0x%016llX POS %08llX  OBJ/Blks %016llX  Cmd 0x%08llX.",
   "RG: LBA 0x%08llX  crc:%08llX  Blks %6llx  Info/Cmd 0x%08llX.",
   "0x1d: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "0x1e: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "0x1f: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "LDO: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "PDO: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "PORT: LBA 0x%08llX%08llX  Blks %6llx  FRU/Cmd 0x%08llX.",
   "VD: LBA 0x%016llX %llX  OBJ/Blks %016llX  Cmd 0x%08llX.",
   "PVD: LBA 0x%016llX %llX  OBJ/Blks %016llX  Cmd 0x%08llX.",
   "DDS: LBA 0x%016llX %llX, PoolLuBlks %llX, Cmd 0x%08llX.", // KT_DDS_TRAFFIC
   "SPC: LBA 0x%016llX %llX, LuBlks %016llX, Cmd 0x%016llX.",
   "CBFS: LBA 0x%016llX %llX, LuBlks %016llX, Cmd 0x%08llX.",
   "CCB: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "MPT: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "MCIO: LBA 0x%08llX%08llX  Blks %6llx  FLU/Cmd 0x%08llX.",
   "TDX: LBA 0x%016llX %llX, LuBlks %016llX, Cmd 0x%016llX.",
};
#endif /* KT_TABLE_S */

// Traffic logging macros for TCD,CCB,MPT, PSM, LUN, FRU, BE, SFE, DML, Clones
#define TCD_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TTCD)

#define CCB_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TCCB )

#define MPT_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TMPT )

#define PSM_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TPSM)

#define LUN_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TLUN)

#define FRU_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TFRU)

#define DBE_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TDBE)

#define FBE_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TFBE)

/* ARatnani DIMS 124667. Adding macro to log SFE traffic */ 
#define SFE_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TSFE)
  
#define DML_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TDML)

#define MVS_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TMVS)

/* MLU allows device and/or cbfs logging*/
#define MLU_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO, pTRACE_TYPE) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & pTRACE_TYPE)

/*RBA Logging for Snap*/
#define SNAP_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TSNAP)
//DIMS 188125 RBA tracing

#define AGG_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TAGG)


#define MIG_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TMIG)

#define CPM_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TCPM)

#define FED_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TFED)


//DIMS 188132 SnapClones RBA tracing
#define CLONES_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TCLONES)


#define COMPRESSION_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TCOMPRESSION)

#define FCT_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TFCT)

#define SPC_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TSPC)

#define TDX_TRAFFIC_LOGGING_ENABLED(pTRACE_INFO) \
  (pTRACE_INFO != NULL && ((TRACE_INFO *) pTRACE_INFO)->ti_tflag & KTRC_TTDX)

/* 
 * Action:
 * Designates if are we starting or completing an I/O.
 *
 * Label:
 * Describes the purpose of the I/O.
 *
 * I/O Type:
 * Designates how we will be using or changing the
 * data at the designated LU/LBA.
 *
 * Layer ID:
 * Designates the Layered Driver that is initiating
 * the I/O (via the Data Movement Library).
 *
 * Session ID:
 * Used to differentiate between different categories of IO. eg, 
 * could be used to differentiate between LU if a single LU
 * has multiple clones, snaps or mirrors
 *
 *
 *    +----+----+----+----+
 *  0:|    | 1:           |     Action, IO Type
 *    +----+----+----+----+
 *  4:|                   |     Label
 *    +----+----+----+----+
 *  8:|                   |     Layer ID
 *    +----+----+----+----+
 * 12:|                   |     Session ID
 *    +----+----+----+----+
 * 16: LUN or FLU.
 */

/* RBA tracing format for Layered Drivers:
 *  KTRACEX(TRC_K_TRAFFIC,
 *      <KT ID>,
 *      lba.HighPart,
 *      lba.LowPart,
 *      <size in blocks>,
 *      KT_TRAFFIC_<layer_label_iotype_action> | 
 *        KT_TRAFFIC_FLU_ID(lun))
 */

/* Action
 */
#define KT_START        0x0000
#define KT_DONE         0x0001

/* I/O Type
 */
#define KT_READ         0x0000
#define KT_WRITE        0x0002
#define KT_ZERO         0x0004
#define KT_CRPT_CRC     0x0006
#define KT_MOVE         0x0008
#define KT_XCHANGE      0x000a
#define KT_COMMIT       0x000c
#define KT_NO_OP        0x000e
#define KT_GET_MDR      0x0008
#define KT_COPY         0x000a
#define KT_PASSTHRU     0x000c



/* CBFS Type
 */
#define KT_EXTENT_CACHE_LOOKUP      0x0000
#define KT_IB_CACHE_LOOKUP          0x0002
#define KT_INDIRECT_BLOCK           0x0004
#define KT_DATA_BLOCK               0x0006
#define KT_BUFFER_WATCH_THROTTLE    0x0008
#define KT_HEAP_WATCH_THROTTLE      0x000a

/* Clones Specific Labels
 */
#define KT_CLONE_PRI                      0x0010
#define KT_CLONE_SEC                      0x0020
#define KT_CLONE_OPERATION                0x0030
#define KT_CLONE_COD                      0x0040
#define KT_CLONE_SYNC                     0x0060
#define KT_CLONE_REV_SYNC                 0x0070
#define KT_CLONE_DM                       0x0080
#define KT_CLONE_SYNC_DM                  0x0090
#define KT_CLONE_REV_SYNC_DM              0x00A0
#define KT_CLONE_ZERO                     0x00B0
#define KT_CLONE_ECN                    0x00C0

/* Flash Cache specific labels */
#define KT_FCT_IDLE_CLEAN           0x0010
#define KT_FCT_CACHE_DEVICE_FLUSH   0x0020
#define KT_FCT_IDLE_FLUSH           0x0030
#define KT_FCT_PROMOTE              0x0040
#define KT_FCT_DIRUPDATE            0x0050
#define KT_FCT_CDUPDATE             0x0060
#define KT_FCT_SYNC                 0x0070
#define KT_FCT_FLASH_REQUEST        0x0080

/*Dedup Server specific labels
  dedup server will use the KT_READ IO type
  and overload IO labels*/
#define KT_DDS_DIGEST_REQUEST               0X0010
#define KT_DDS_DIGEST_IOD                   0X0020
#define KT_DDS_DIGEST_CALCULATION           0X0030
#define KT_DDS_DEDUP_REQUEST                0x0040
#define KT_DDS_READ_CHUNKS                  0X0050
#define KT_DDS_READ_KEY_IOD                 0X0060
#define KT_DDS_BIT_COMPARE                  0X0070
#define KT_DDS_DEDUP_CHUNK                  0X0080

/* SPC specific labels
 */
#define KT_SPC_HOST_REQUEST                 0x0010
#define KT_SPC_BACKFILL                     0x0020
#define KT_SPC_PREFETCH                     0x0030
#define KT_SPC_FLUSH                        0x0040

/* FBE specific labels
 */               
#define KT_FBE_PRIORITY_LOW                 0x0040
#define KT_FBE_PRIORITY_NORMAL              0x0080
#define KT_FBE_PRIORITY_URGENT              0x00C0
#define KT_FBE_ERROR                        0x8000

/* TDX specific labels
*/
#define KT_TDX_HOST_REQUEST                 0x0010

/* Label
 */
#define KT_PRIMARY       0x0010
#define KT_SECONDARY     0x0020
#define KT_SOURCE        0x0030
#define KT_DESTINATION   0x0040
#define KT_METADATA      0x0050
#define KT_SEGMENT       0x0060
#define KT_CBFS          0x0070
#define KT_BG_VERIFY     0x0080
#define KT_REBUILD       0x0090
#define KT_SNIFF_VERIFY  0x00A0
#define KT_ZERO_ON_DMD   0x00B0
#define KT_CRT_FE_DEV    0x00C0


/* Layer ID
 */
#define KT_FLARE         0x0100
#define KT_AGGREGATE     0x0200
#define KT_MIGRATION     0x0300
#define KT_SNAP          0x0400
#define KT_CLONE         0x0500
#define KT_MVA           0x0600
#define KT_MVS           0x0700
#define KT_SANCOPY       0x0800
#define KT_ROLLBACK      0x0900
#define KT_MLU           0x0a00
#define KT_FCT           0x0b00
#define KT_FED           0x0c00
#define KT_CPM           0x0d00
#define KT_CBFS_INTERNAL 0x0e00
#define KT_SPC           0x0f00
#define KT_TDX           0x0600

/* Shortcut macros for use in saving values to the
 * "tr_a3" argument field.
 */
#define KT_TRAFFIC_SESSION_ID(_sid) (_sid << 12)
#define KT_TRAFFIC_FLU_ID(_flu)     (((UINT_16)_flu) << 16)
#define KT_TRAFFIC_CLONE_NUM(_CloneNum)     (((_CloneNum) << 12))

#define KT_TRACE_RECORD(TraceType,InfoPtr)	(TraceType 		| \
												  KT_TRAFFIC_FLU_ID (InfoPtr -> Lun) | \
												  KT_TRAFFIC_CLONE_NUM ((InfoPtr  -> ImageIndex) + 1) \
												)
/*
* Deduplication server
* Expanded trace format: tr_a0 is LBA1, tr_a1 is LBA2, tr_a2 is LU2 | LU1 | BLOCKS.
*/
#define KT_TRAFFIC_BLOCKS(_blks)    (((ULONG64)(_blks)))
#define KT_TRAFFIC_LU1(_flu)        (((ULONG64)((UINT_16E)(_flu))) << 32)
#define KT_TRAFFIC_LU2(_flu)        (((ULONG64)((UINT_16E)(_flu))) << 48)

/*Usage:
    KTRACEX(TRC_K_TRAFFIC, KT_DDS_TRAFFIC,
        lba1,
        lba2,
        KT_TRAFFIC_BLOCKS(blocks) | KT_TRAFFIC_LU1(lu1) | KT_TRAFFIC_LU2(poolid),
        opcode);
*/

/* Traffic IO type / Action definitions
 */

// 0x0000
#define KT_TRAFFIC_READ_START   (KT_START | KT_READ)

// 0x0001
#define KT_TRAFFIC_READ_DONE    (KT_DONE | KT_READ)

// 0x0002
#define KT_TRAFFIC_WRITE_START  (KT_START | KT_WRITE)

// 0x0003
#define KT_TRAFFIC_WRITE_DONE   (KT_DONE | KT_WRITE)

// 0x0004
#define KT_TRAFFIC_ZERO_START   (KT_START | KT_ZERO)

// 0x0005
#define KT_TRAFFIC_ZERO_DONE    (KT_DONE | KT_ZERO)

// 0x0006
#define KT_TRAFFIC_CRPT_CRC_START   (KT_START | KT_CRPT_CRC)

// 0x0007
#define KT_TRAFFIC_CRPT_CRC_DONE    (KT_DONE | KT_CRPT_CRC)

// 0x0008
#define KT_TRAFFIC_MOVE_START   (KT_START | KT_MOVE)

// 0x0009
#define KT_TRAFFIC_MOVE_DONE    (KT_DONE | KT_MOVE)

// 0x000e
#define KT_TRAFFIC_MDR_START   (KT_START | KT_GET_MDR)

// 0x000f
#define KT_TRAFFIC_MDR_DONE    (KT_DONE | KT_GET_MDR)

// 0x000a
#define KT_TRAFFIC_COPY_START   (KT_START | KT_COPY)

// 0x000b
#define KT_TRAFFIC_COPY_DONE    (KT_DONE | KT_COPY)

// 0x0010
#define KT_TRAFFIC_NO_OP    0x0010

/*********************************/
/* Data Movement Library Tracing */
/*********************************/

// Generic DM Start
#define KT_TRAFFIC_DML_START(_layerID) \
    (KT_TRAFFIC_MOVE_START | _layerID)

// Generic DM Done
#define KT_TRAFFIC_DML_DONE(_layerID) \
    (KT_TRAFFIC_MOVE_DONE | _layerID)

// Generic DCA Read Start
#define KT_TRAFFIC_DML_DCA_READ_START(_layerID) \
    (KT_TRAFFIC_READ_START | _layerID)

// Generic DCA Read Done
#define KT_TRAFFIC_DML_DCA_READ_DONE(_layerID) \
    (KT_TRAFFIC_READ_DONE | _layerID)

// Generic DCA Write Start
#define KT_TRAFFIC_DML_DCA_WRITE_START(_layerID) \
    (KT_TRAFFIC_WRITE_START | _layerID)

// Generic DCA Write Done
#define KT_TRAFFIC_DML_DCA_WRITE_DONE(_layerID) \
    (KT_TRAFFIC_WRITE_DONE | _layerID)

// Generic MJ Read Start
#define KT_TRAFFIC_DML_MJ_READ_START(_layerID) \
    (KT_TRAFFIC_READ_START | _layerID)

// Generic MJ Read Done
#define KT_TRAFFIC_DML_MJ_READ_DONE(_layerID) \
    (KT_TRAFFIC_READ_DONE | _layerID)

// Generic MJ Write Start
#define KT_TRAFFIC_DML_MJ_WRITE_START(_layerID) \
    (KT_TRAFFIC_WRITE_START | _layerID)

// Generic MJ Write Done
#define KT_TRAFFIC_DML_MJ_WRITE_DONE(_layerID) \
    (KT_TRAFFIC_WRITE_DONE | _layerID)

//BG_VERIFY
//0x8e
#define KT_TRAFFIC_BG_VERIFY_START  \
        (KT_START | KT_BG_VERIFY | KT_NO_OP)

//0x8f
#define KT_TRAFFIC_BG_VERIFY_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_NO_OP)

//0x80
#define KT_TRAFFIC_BG_VERIFY_READ_START  \
        (KT_START | KT_BG_VERIFY | KT_READ)

//0x81
#define KT_TRAFFIC_BG_VERIFY_READ_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_READ)

//0x82
#define KT_TRAFFIC_BG_VERIFY_WRITE_START  \
        (KT_START | KT_BG_VERIFY | KT_WRITE)

//0x83
#define KT_TRAFFIC_BG_VERIFY_WRITE_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_WRITE)

//0x84
#define KT_TRAFFIC_BG_VERIFY_ZERO_START  \
        (KT_START | KT_BG_VERIFY | KT_ZERO)

//0x85
#define KT_TRAFFIC_BG_VERIFY_ZERO_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_ZERO)

//0x86
#define KT_TRAFFIC_BG_VERIFY_CRPT_CRC_START  \
        (KT_START | KT_BG_VERIFY | KT_CRPT_CRC)

//0x87
#define KT_TRAFFIC_BG_VERIFY_CRPT_CRC_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_CRPT_CRC)

//0x88
#define KT_TRAFFIC_BG_VERIFY_MOVE_START  \
        (KT_START | KT_BG_VERIFY | KT_MOVE)

//0x89
#define KT_TRAFFIC_BG_VERIFY_MOVE_DONE  \
        (KT_DONE | KT_BG_VERIFY | KT_MOVE)


//REBUILD
//0x9e
#define KT_TRAFFIC_REBUILD_START  \
        (KT_START | KT_REBUILD | KT_NO_OP)

//0x9f
#define KT_TRAFFIC_REBUILD_DONE  \
        (KT_DONE | KT_REBUILD | KT_NO_OP)

//0x90
#define KT_TRAFFIC_REBUILD_READ_START  \
        (KT_START | KT_REBUILD | KT_READ)

//0x91
#define KT_TRAFFIC_REBUILD_READ_DONE  \
        (KT_DONE | KT_REBUILD | KT_READ)

//0x92
#define KT_TRAFFIC_REBUILD_WRITE_START  \
        (KT_START | KT_REBUILD | KT_WRITE)

//0x93
#define KT_TRAFFIC_REBUILD_WRITE_DONE  \
        (KT_DONE | KT_REBUILD | KT_WRITE)

//0x94
#define KT_TRAFFIC_REBUILD_ZERO_START  \
        (KT_START | KT_REBUILD | KT_ZERO)

//0x95
#define KT_TRAFFIC_REBUILD_ZERO_DONE  \
        (KT_DONE | KT_REBUILD | KT_ZERO)

//0x96
#define KT_TRAFFIC_REBUILD_CRPT_CRC_START  \
        (KT_START | KT_REBUILD | KT_CRPT_CRC)

//0x97
#define KT_TRAFFIC_REBUILD_CRPT_CRC_DONE  \
        (KT_DONE | KT_REBUILD | KT_CRPT_CRC)

//0x98
#define KT_TRAFFIC_REBUILD_MOVE_START  \
        (KT_START | KT_REBUILD | KT_MOVE)

//0x99
#define KT_TRAFFIC_REBUILD_MOVE_DONE  \
        (KT_DONE | KT_REBUILD | KT_MOVE)


//SNIFF_VERIFY
//0xae
#define KT_TRAFFIC_SNIFF_VERIFY_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_NO_OP)

//0xaf
#define KT_TRAFFIC_SNIFF_VERIFY_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_NO_OP)

//0xa0
#define KT_TRAFFIC_SNIFF_VERIFY_READ_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_READ)

//0xa1
#define KT_TRAFFIC_SNIFF_VERIFY_READ_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_READ)

//0xa2
#define KT_TRAFFIC_SNIFF_VERIFY_WRITE_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_WRITE)

//0xa3
#define KT_TRAFFIC_SNIFF_VERIFY_WRITE_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_WRITE)

//0xa4
#define KT_TRAFFIC_SNIFF_VERIFY_ZERO_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_ZERO)

//0xa5
#define KT_TRAFFIC_SNIFF_VERIFY_ZERO_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_ZERO)

//0xa6
#define KT_TRAFFIC_SNIFF_VERIFY_CRPT_CRC_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_CRPT_CRC)

//0xa7
#define KT_TRAFFIC_SNIFF_VERIFY_CRPT_CRC_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_CRPT_CRC)

//0xa8
#define KT_TRAFFIC_SNIFF_VERIFY_MOVE_START  \
        (KT_START | KT_SNIFF_VERIFY | KT_MOVE)

//0xa9
#define KT_TRAFFIC_SNIFF_VERIFY_MOVE_DONE  \
        (KT_DONE | KT_SNIFF_VERIFY | KT_MOVE)


//ZERO_ON_DMD
//0xbe
#define KT_TRAFFIC_ZERO_ON_DMD_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_NO_OP)

//0xbf
#define KT_TRAFFIC_ZERO_ON_DMD_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_NO_OP)

//0xb0
#define KT_TRAFFIC_ZERO_ON_DMD_READ_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_READ)

//0xb1
#define KT_TRAFFIC_ZERO_ON_DMD_READ_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_READ)

//0xb2
#define KT_TRAFFIC_ZERO_ON_DMD_WRITE_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_WRITE)

//0xb3
#define KT_TRAFFIC_ZERO_ON_DMD_WRITE_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_WRITE)

//0xb4
#define KT_TRAFFIC_ZERO_ON_DMD_ZERO_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_ZERO)

//0xb5
#define KT_TRAFFIC_ZERO_ON_DMD_ZERO_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_ZERO)

//0xb6
#define KT_TRAFFIC_ZERO_ON_DMD_CRPT_CRC_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_CRPT_CRC)

//0xb7
#define KT_TRAFFIC_ZERO_ON_DMD_CRPT_CRC_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_CRPT_CRC)

//0xb8
#define KT_TRAFFIC_ZERO_ON_DMD_MOVE_START  \
        (KT_START | KT_ZERO_ON_DMD | KT_MOVE)

//0xb9
#define KT_TRAFFIC_ZERO_ON_DMD_MOVE_DONE  \
        (KT_DONE | KT_ZERO_ON_DMD | KT_MOVE)

/* FLARE | START | MOVE
 * source:      0x0138
 * destination: 0x0148
 * KT_TRAFFIC_DML_FLARE_START (KT_TRAFFIC_DML_START(KT_FLARE))
 */

/* FLARE | DONE | MOVE
 * source:      0x0139
 * destination: 0x0149
 * KT_TRAFFIC_DML_FLARE_DONE (KT_TRAFFIC_DML_DONE(KT_FLARE))
 */

/* AGGREGATE | START | MOVE
 * source:      0x0238
 * destination: 0x0248
 * KT_TRAFFIC_DML_AGGREGATE_START (KT_TRAFFIC_DML_START(KT_AGGREGATE))
 */

/* AGGREGATE | DONE | MOVE
 * source:      0x0239
 * destination: 0x0249
 * KT_TRAFFIC_DML_AGGREGATE_DONE (KT_TRAFFIC_DML_DONE(KT_AGGREGATE))
 */

/* MIGRATION | START | MOVE
 * source:      0x0338
 * destination: 0x0348
 * KT_TRAFFIC_DML_MIGRATION_START (KT_TRAFFIC_DML_START(KT_MIGRATION))
 */

/* MIGRATION | DONE | MOVE
 * source:      0x0339
 * destination: 0x0349
 * KT_TRAFFIC_DML_MIGRATION_DONE (KT_TRAFFIC_DML_DONE(KT_MIGRATION))
 */

/* SNAP | START | MOVE
 * source:      0x0438
 * destination: 0x0448
 * KT_TRAFFIC_DML_SNAP_START (KT_TRAFFIC_DML_START(KT_SNAP))
 */

/* SNAP | DONE | MOVE
 * source:      0x0439
 * destination: 0x0449
 * KT_TRAFFIC_DML_SNAP_DONE (KT_TRAFFIC_DML_DONE(KT_SNAP))
 */

/* CLONE | START | MOVE
 * source:      0x0538
 * destination: 0x0548
 * KT_TRAFFIC_DML_CLONE_START (KT_TRAFFIC_DML_START(KT_CLONE))
 */

/* CLONE | DONE | MOVE
 * source:      0x0539
 * destination: 0x0549
 * KT_TRAFFIC_DML_CLONE_DONE (KT_TRAFFIC_DML_DONE(KT_CLONE))
 */

/*********************/
/* MVA Layer Tracing */
/*********************/

// 0x0612
#define KT_TRAFFIC_MVA_PRIMARY_WRITE_START \
    (KT_START | KT_PRIMARY | KT_WRITE | KT_MVA )

// 0x0613
#define KT_TRAFFIC_MVA_PRIMARY_WRITE_DONE \
    (KT_DONE | KT_PRIMARY | KT_WRITE | KT_MVA )

// 0x1622
#define KT_TRAFFIC_MVA_SECONDARY_1_WRITE_START \
    (KT_START | KT_SECONDARY | KT_WRITE | KT_MVA | KT_TRAFFIC_SESSION_ID(1))

// 0x1623
#define KT_TRAFFIC_MVA_SECONDARY_1_WRITE_DONE \
    (KT_DONE | KT_SECONDARY | KT_WRITE | KT_MVA | KT_TRAFFIC_SESSION_ID(1))

// 0x2622
#define KT_TRAFFIC_MVA_SECONDARY_2_WRITE_START \
    (KT_START | KT_SECONDARY | KT_WRITE | KT_MVA | KT_TRAFFIC_SESSION_ID(2))

// 0x2623
#define KT_TRAFFIC_MVA_SECONDARY_2_WRITE_DONE \
    (KT_DONE | KT_SECONDARY | KT_WRITE | KT_MVA | KT_TRAFFIC_SESSION_ID(2))

/*********************/
/* MVS Layer Tracing */
/*********************/
// 0x0712
#define KT_TRAFFIC_MVS_PRIMARY_WRITE_START \
    (KT_START | KT_PRIMARY | KT_WRITE | KT_MVS )

// 0x0713
#define KT_TRAFFIC_MVS_PRIMARY_WRITE_DONE \
    (KT_DONE | KT_PRIMARY | KT_WRITE | KT_MVS )

// 0x1722
#define KT_TRAFFIC_MVS_SECONDARY_1_WRITE_START \
    (KT_START | KT_SECONDARY | KT_WRITE | KT_MVS | KT_TRAFFIC_SESSION_ID(1))

// 0x1723
#define KT_TRAFFIC_MVS_SECONDARY_1_WRITE_DONE \
    (KT_DONE | KT_SECONDARY | KT_WRITE | KT_MVS | KT_TRAFFIC_SESSION_ID(1))

// 0x2722
#define KT_TRAFFIC_MVS_SECONDARY_2_WRITE_START \
    (KT_START | KT_SECONDARY | KT_WRITE | KT_MVS | KT_TRAFFIC_SESSION_ID(2))

// 0x2723
#define KT_TRAFFIC_MVS_SECONDARY_2_WRITE_DONE \
    (KT_DONE | KT_SECONDARY | KT_WRITE | KT_MVS | KT_TRAFFIC_SESSION_ID(2))

/*********************/
/* MLU Layer Tracing */
/*********************/

/* Macros for internal use
*/
#define KT_MLU_METADATA_ISSUE   KT_TRAFFIC_SESSION_ID(1)

/* Opcodes
*/

// 0x0a00
#define KT_TRAFFIC_MLU_READ_START \
    (KT_START | KT_READ | KT_MLU )

// 0x0a01
#define KT_TRAFFIC_MLU_READ_DONE \
    (KT_DONE | KT_READ | KT_MLU )

// 0x0a02
#define KT_TRAFFIC_MLU_WRITE_START \
    (KT_START | KT_WRITE | KT_MLU )

// 0x0a03
#define KT_TRAFFIC_MLU_WRITE_DONE \
    (KT_DONE | KT_WRITE | KT_MLU )

// 0x0a04
#define KT_TRAFFIC_MLU_ZERO_FILL_START \
    (KT_START | KT_ZERO | KT_MLU )

// 0x0a05
#define KT_TRAFFIC_MLU_ZERO_FILL_DONE \
    (KT_DONE | KT_ZERO | KT_MLU )

// 0x0a50
#define KT_TRAFFIC_MLU_METADATA_READ_START \
    (KT_START | KT_READ | KT_METADATA | KT_MLU )

// 0x0a51
#define KT_TRAFFIC_MLU_METADATA_READ_DONE \
    (KT_DONE | KT_READ | KT_METADATA | KT_MLU )

// 0x0a52
#define KT_TRAFFIC_MLU_METADATA_WRITE_START \
    (KT_START | KT_WRITE | KT_METADATA | KT_MLU )

// 0x0a53
#define KT_TRAFFIC_MLU_METADATA_WRITE_DONE \
    (KT_DONE | KT_WRITE | KT_METADATA | KT_MLU )

// 0x1a50
#define KT_TRAFFIC_MLU_METADATA_ISSUE_READ_START \
    (KT_START | KT_READ | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )

// 0x1a51
#define KT_TRAFFIC_MLU_METADATA_ISSUE_READ_DONE \
    (KT_DONE | KT_READ | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )

// 0x1a52
#define KT_TRAFFIC_MLU_METADATA_ISSUE_WRITE_START \
    (KT_START | KT_WRITE | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )

// 0x1a53
#define KT_TRAFFIC_MLU_METADATA_ISSUE_WRITE_DONE \
    (KT_DONE | KT_WRITE | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )

// 0x1a54
#define KT_TRAFFIC_MLU_METADATA_ISSUE_ZERO_FILL_START \
    (KT_START | KT_ZERO | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )

// 0x1a55
#define KT_TRAFFIC_MLU_METADATA_ISSUE_ZERO_FILL_DONE \
    (KT_DONE | KT_ZERO | KT_METADATA | KT_MLU | KT_MLU_METADATA_ISSUE )


// 0x0a60
#define KT_TRAFFIC_MLU_SEGMENT_READ_START \
    (KT_START | KT_READ | KT_SEGMENT | KT_MLU )

// 0x0a61
#define KT_TRAFFIC_MLU_SEGMENT_READ_DONE \
    (KT_DONE | KT_READ | KT_SEGMENT | KT_MLU )

// 0x0a62
#define KT_TRAFFIC_MLU_SEGMENT_WRITE_START \
    (KT_START | KT_WRITE | KT_SEGMENT | KT_MLU )

// 0x0a63
#define KT_TRAFFIC_MLU_SEGMENT_WRITE_DONE \
    (KT_DONE | KT_WRITE | KT_SEGMENT | KT_MLU )

// 0x0a7a
#define KT_TRAFFIC_MLU_CBFS_XCHANGE_START \
    (KT_START | KT_XCHANGE | KT_CBFS | KT_MLU )

// 0x0a7b
#define KT_TRAFFIC_MLU_CBFS_XCHANGE_DONE \
    (KT_DONE | KT_XCHANGE | KT_CBFS | KT_MLU )

//  0x0a70
#define KT_TRAFFIC_MLU_CBFS_MAP_READ_START \
    (KT_START | KT_READ | KT_CBFS | KT_MLU )

//  0x0a71
#define KT_TRAFFIC_MLU_CBFS_MAP_READ_DONE \
    (KT_DONE | KT_READ | KT_CBFS | KT_MLU )

//  0x0a72
#define KT_TRAFFIC_MLU_CBFS_MAP_WRITE_START \
    (KT_START | KT_WRITE | KT_CBFS | KT_MLU )

// 0x0a73
#define KT_TRAFFIC_MLU_CBFS_MAP_WRITE_DONE \
    (KT_DONE | KT_WRITE | KT_CBFS | KT_MLU )

// 0x0a7c
#define KT_TRAFFIC_MLU_CBFS_XCHANGE_COMMIT_START \
    (KT_START | KT_COMMIT | KT_CBFS | KT_MLU )

// 0x0a7d
#define KT_TRAFFIC_MLU_CBFS_XCHANGE_COMMIT_DONE \
    (KT_DONE | KT_COMMIT | KT_CBFS | KT_MLU )


//DIMS 188125 RBA tracing for the SNAP Copy Driver
//Please enable the SNAP_RBA_TRACE_ENABLE preprocessor
//directive in Snapcopy.h and compile the driver code
// to enable RBA tracing in SNAP Driver

//0x0400 + 0xNNNN0000
//Target write start
#define KT_TRAFFIC_SNAP_COFW_START(_lun) \
    (KT_START|KT_SNAP|KT_TRAFFIC_FLU_ID(_lun))

//Target write done
//0x0401 + 0xNNNN0000
#define KT_TRAFFIC_SNAP_COFW_DONE(_lun) \
    (KT_DONE|KT_SNAP|KT_TRAFFIC_FLU_ID(_lun))

//0x0412 + 0xN000 + 0xNNNN0000
//Called before call to the DM library
#define KT_TRAFFIC_SNAP_COPY_START(_session_id,_lun) \
    (KT_START| KT_PRIMARY| KT_WRITE|KT_SNAP|KT_TRAFFIC_SESSION_ID(_session_id)|KT_TRAFFIC_FLU_ID(_lun))

//Called after DM is complete	
//0x413 + 0xN000 + 0xNNNN0000
#define KT_TRAFFIC_SNAP_COPY_DONE(_session_id,_lun) \
    (KT_DONE|KT_PRIMARY|KT_WRITE|KT_SNAP|KT_TRAFFIC_SESSION_ID(_session_id)|KT_TRAFFIC_FLU_ID(_lun))

//Write to a Snap start
//0x422 + 0xNNNN0000
#define KT_TRAFFIC_SNAP_WRITE_NEW_START(_lun)\
    (KT_START|KT_SECONDARY|KT_WRITE|KT_SNAP|KT_TRAFFIC_FLU_ID(_lun))

//Write to a snap done
//0x423 + 0xNNNN0000
#define KT_TRAFFIC_SNAP_WRITE_NEW_DONE(_lun)\
    (KT_DONE|KT_SECONDARY|KT_WRITE|KT_SNAP|KT_TRAFFIC_FLU_ID(_lun))


// Aggregate
// 
// 0x00000200
#define KT_TRAFFIC_AGG_READ_START(_lun) \
    (KT_START|KT_READ|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000201
#define KT_TRAFFIC_AGG_READ_DONE(_lun) \
    (KT_DONE|KT_READ|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000202
#define KT_TRAFFIC_AGG_WRITE_START(_lun) \
    (KT_START|KT_WRITE|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000203
#define KT_TRAFFIC_AGG_WRITE_DONE(_lun) \
    (KT_DONE|KT_WRITE|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000204
#define KT_TRAFFIC_AGG_ZERO_FILL_START(_lun) \
    (KT_START|KT_ZERO|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000205
#define KT_TRAFFIC_AGG_ZERO_FILL_DONE(_lun) \
    (KT_DONE|KT_ZERO|KT_AGGREGATE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000260
#define KT_TRAFFIC_AGG_FLUN_READ_START(_lun) \
    (KT_START|KT_READ|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))
// 
// 0x00000261
#define KT_TRAFFIC_AGG_FLUN_READ_DONE(_lun) \
    (KT_DONE|KT_READ|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000262
#define KT_TRAFFIC_AGG_FLUN_WRITE_START(_lun) \
    (KT_START|KT_WRITE|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000263
#define KT_TRAFFIC_AGG_FLUN_WRITE_DONE(_lun) \
    (KT_DONE|KT_WRITE|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000264
#define KT_TRAFFIC_AGG_FLUN_ZERO_FILL_START(_lun) \
    (KT_START|KT_ZERO|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000265
#define KT_TRAFFIC_AGG_FLUN_ZERO_FILL_DONE(_lun) \
    (KT_DONE|KT_ZERO|KT_AGGREGATE|KT_SEGMENT|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000270
#define KT_TRAFFIC_AGG_EXP_READ_START(_lun) \
    (KT_START|KT_READ|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000271
#define KT_TRAFFIC_AGG_EXP_READ_DONE(_lun) \
    (KT_DONE|KT_READ|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000272
#define KT_TRAFFIC_AGG_EXP_WRITE_START(_lun) \
    (KT_START|KT_WRITE|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000273
#define KT_TRAFFIC_AGG_EXP_WRITE_DONE(_lun) \
    (KT_DONE|KT_WRITE|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000274
#define KT_TRAFFIC_AGG_EXP_ZERO_FILL_START(_lun) \
    (KT_START|KT_ZERO|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000275
#define KT_TRAFFIC_AGG_EXP_ZERO_FILL_DONE(_lun) \
    (KT_DONE|KT_ZERO|KT_AGGREGATE|KT_CBFS|KT_TRAFFIC_FLU_ID(_lun))


// Migration

// 0x00000300
#define KT_TRAFFIC_MIG_READ_START(_lun) \
    (KT_START|KT_READ|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000301
#define KT_TRAFFIC_MIG_READ_DONE(_lun) \
    (KT_DONE|KT_READ|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000302
#define KT_TRAFFIC_MIG_WRITE_START(_lun) \
    (KT_START|KT_WRITE|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000303
#define KT_TRAFFIC_MIG_WRITE_DONE(_lun) \
    (KT_DONE|KT_WRITE|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000304
#define KT_TRAFFIC_MIG_ZERO_FILL_START(_lun) \
    (KT_START|KT_ZERO|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000305
#define KT_TRAFFIC_MIG_ZERO_FILL_DONE(_lun) \
    (KT_DONE|KT_ZERO|KT_MIGRATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000330
#define KT_TRAFFIC_MIG_SYNC_READ_SRC_START(_lun) \
    (KT_START|KT_READ|KT_MIGRATION|KT_SOURCE|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000331
#define KT_TRAFFIC_MIG_SYNC_READ_SRC_DONE(_lun) \
    (KT_DONE|KT_READ|KT_MIGRATION|KT_SOURCE|KT_TRAFFIC_FLU_ID(_lun))
// 0x00000342
#define KT_TRAFFIC_MIG_SYNC_WRITE_DEST_START(_lun) \
    (KT_START|KT_WRITE|KT_MIGRATION|KT_DESTINATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000343
#define KT_TRAFFIC_MIG_SYNC_WRITE_DEST_DONE(_lun) \
    (KT_DONE|KT_WRITE|KT_MIGRATION|KT_DESTINATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000344
#define KT_TRAFFIC_MIG_SYNC_ZERO_FILL_DEST_START(_lun) \
    (KT_START|KT_ZERO|KT_MIGRATION|KT_DESTINATION|KT_TRAFFIC_FLU_ID(_lun))

// 0x00000345
#define KT_TRAFFIC_MIG_SYNC_ZERO_FILL_DEST_DONE(_lun) \
    (KT_DONE|KT_ZERO|KT_MIGRATION|KT_DESTINATION|KT_TRAFFIC_FLU_ID(_lun))
/****************/
/* CBFS Tracing */
/****************/

/* Macros for internal use
*/
#define KT_CBFS_SET_2   KT_TRAFFIC_SESSION_ID(3)
#define KT_CBFS_SET_3   KT_TRAFFIC_SESSION_ID(4)

#define KT_TRAFFIC_MFW_MFR_NONALLOC_EXTENTCACHELOOKUP_START \
    (KT_START | KT_EXTENT_CACHE_LOOKUP | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_MFR_NONALLOC_EXTENTCACHELOOKUP_END \
    (KT_DONE | KT_EXTENT_CACHE_LOOKUP | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_MFR_NONALLOC_IBCACHELOOKUP_START \
    (KT_START | KT_IB_CACHE_LOOKUP | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_MFR_NONALLOC_IBCACHELOOKUP_END \
    (KT_DONE | KT_IB_CACHE_LOOKUP | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_PREALLOC_INDIRECTBLOCK_START \
    (KT_START | KT_INDIRECT_BLOCK | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_PREALLOC_INDIRECTBLOCK_END \
    (KT_DONE | KT_INDIRECT_BLOCK | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_PREALLOC_DATABLOCK_START \
    (KT_START | KT_DATA_BLOCK | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_MFW_PREALLOC_DATABLOCK_END \
    (KT_DONE | KT_DATA_BLOCK | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_LOG_REPLAY_BUFFERWATCHTHROTTLE_DOWN \
    (KT_START | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_LOG_REPLAY_BUFFERWATCHTHROTTLE_UP \
    (KT_DONE | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_LOG_REPLAY_HEAPWATCHTHROTTLE_DOWN \
    (KT_START | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_LOG_REPLAY_HEAPWATCHTHROTTLE_UP \
    (KT_DONE | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL )

#define KT_TRAFFIC_CBFS_API_BUFFERWATCHTHROTTLE_DOWN \
    (KT_START | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_2 )

#define KT_TRAFFIC_CBFS_API_BUFFERWATCHTHROTTLE_UP \
    (KT_DONE | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_2 )

#define KT_TRAFFIC_CBFS_API_HEAPWATCHTHROTTLE_DOWN \
    (KT_START | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_2 )

#define KT_TRAFFIC_CBFS_API_HEAPWATCHTHROTTLE_UP \
    (KT_DONE | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_2 )

#define KT_TRAFFIC_UFSCORE_BUFFERWATCHTHROTTLE_DOWN \
    (KT_START | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_3 )

#define KT_TRAFFIC_UFSCORE_BUFFERWATCHTHROTTLE_UP \
    (KT_DONE | KT_BUFFER_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_3 )

#define KT_TRAFFIC_UFSCORE_HEAPWATCHTHROTTLE_DOWN \
    (KT_START | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_3 )

#define KT_TRAFFIC_UFSCORE_HEAPWATCHTHROTTLE_UP \
    (KT_DONE | KT_HEAP_WATCH_THROTTLE | KT_CBFS | KT_CBFS_INTERNAL | KT_CBFS_SET_3 )


/****************/
/* SAN Copy Tracing */
/****************/

#define KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) (_did << 22)
#define KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid) (_iodid << 17)
#define KT_TRAFFIC_CPM_DEST_ID(_destid) (_destid << 12)

#define KT_TRAFFIC_CPM_READ_START(_did, _iodid) \
    (KT_START | KT_READ | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_READ_DONE(_did, _iodid) \
    (KT_DONE | KT_READ | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_WRITE_START(_did, _iodid, _destid) \
    (KT_START | KT_WRITE | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid) | KT_TRAFFIC_CPM_DEST_ID(_destid))

#define KT_TRAFFIC_CPM_WRITE_DONE(_did, _iodid, _destid) \
    (KT_DONE | KT_WRITE | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid) | KT_TRAFFIC_CPM_DEST_ID(_destid))

#define KT_TRAFFIC_CPM_ZERO_FILL_START(_did, _iodid, _destid) \
    (KT_START | KT_ZERO | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid) | KT_TRAFFIC_CPM_DEST_ID(_destid))

#define KT_TRAFFIC_CPM_ZERO_FILL_DONE(_did, _iodid, _destid) \
    (KT_DONE | KT_ZERO | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid) | KT_TRAFFIC_CPM_DEST_ID(_destid))

#define KT_TRAFFIC_CPM_GET_MDR_START(_did, _iodid) \
    (KT_START | KT_GET_MDR | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_GET_MDR_DONE(_did, _iodid) \
    (KT_DONE | KT_GET_MDR | KT_CPM | KT_TRAFFIC_CPM_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_IO_DESCRIPTOR_ID(_iodid))


#define KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) (_did << 17)
#define KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid) (_iodid << 12)

#define KT_TRAFFIC_CPM_FAR_READ_START(_did, _iodid) \
    (KT_START | KT_READ | KT_PRIMARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_READ_DONE(_did, _iodid) \
    (KT_DONE | KT_READ | KT_PRIMARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_WRITE_START(_did, _iodid) \
    (KT_START | KT_WRITE | KT_SECONDARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_WRITE_DONE(_did, _iodid) \
    (KT_DONE | KT_WRITE | KT_SECONDARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_ZERO_FILL_START(_did, _iodid) \
    (KT_START | KT_ZERO | KT_SECONDARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_ZERO_FILL_DONE(_did, _iodid) \
    (KT_DONE | KT_ZERO | KT_SECONDARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_GET_MDR_START(_did, _iodid) \
    (KT_START | KT_GET_MDR | KT_PRIMARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))

#define KT_TRAFFIC_CPM_FAR_GET_MDR_DONE(_did, _iodid) \
    (KT_DONE | KT_GET_MDR | KT_PRIMARY | KT_CPM | KT_TRAFFIC_CPM_FAR_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_FAR_IO_DESCRIPTOR_ID(_iodid))


#define KT_TRAFFIC_CPM_COPY_OP_DESCRIPTOR_ID(_did) (_did << 20)
#define KT_TRAFFIC_CPM_COPY_OP_STREAM_ID(_sid) (((UINT_8)_sid) << 12)


#define KT_TRAFFIC_CPM_COPY_START(_did, _sid) \
    (KT_START | KT_SOURCE | KT_COPY | KT_CPM | KT_TRAFFIC_CPM_COPY_OP_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_COPY_OP_STREAM_ID(_sid))

#define KT_TRAFFIC_CPM_COPY_DONE(_did, _sid) \
    (KT_DONE  | KT_SOURCE | KT_COPY | KT_CPM | KT_TRAFFIC_CPM_COPY_OP_DESCRIPTOR_ID(_did) | \
    KT_TRAFFIC_CPM_COPY_OP_STREAM_ID(_sid))


/****************/
/* FEDisk Tracing */
/****************/

#define KT_TRAFFIC_FEDISK_DEVICE_ID(_fid) (_fid << 12)

#define KT_TRAFFIC_FEDISK_READ_START(_fid)  \
    (KT_START | KT_READ | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_READ_DONE(_fid)  \
    (KT_DONE | KT_READ | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_WRITE_START(_fid) \
    (KT_START | KT_WRITE | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_WRITE_DONE(_fid)  \
    (KT_DONE | KT_WRITE | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_ZERO_FILL_START(_fid)  \
    (KT_START | KT_ZERO | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_ZERO_FILL_DONE(_fid)  \
    (KT_DONE | KT_ZERO | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_CREATE_DEVICE_DONE(_fid) \
   (KT_DONE | KT_CRT_FE_DEV | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_PASSTHRU_START(_fid) \
   (KT_START | KT_PASSTHRU | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))

#define KT_TRAFFIC_FEDISK_PASSTHRU_DONE(_fid) \
   (KT_DONE | KT_PASSTHRU | KT_FED | KT_TRAFFIC_FEDISK_DEVICE_ID (_fid))


/*********************/
//DIMS 188132 Clones RBA tracing
/*********************/

//Traces For Over All Operations

// Write IOCTL Start - 0x532
#define KT_TRAFFIC_CLONES_OPERATION_START\
    (KT_START | KT_CLONE_OPERATION | KT_WRITE | KT_CLONE)

// Write IOCTL Done - 0x533
#define KT_TRAFFIC_CLONES_OPERATION_COMPLETE\
    (KT_DONE | KT_CLONE_OPERATION | KT_WRITE | KT_CLONE)

//Traces For Primary Operations

// Write to Source/Fractured Clone Start - 0x512
#define KT_TRAFFIC_CLONES_PRI_WRITE_START\
    (KT_START | KT_CLONE_PRI | KT_WRITE | KT_CLONE)

// Write to Source/Fractured Clone Complete - 0x513
#define KT_TRAFFIC_CLONES_PRI_WRITE_COMPLETE\
    (KT_DONE | KT_CLONE_PRI | KT_WRITE | KT_CLONE)

//Traces For Secondry Operations

// Mirroring Start - 0x522
#define KT_TRAFFIC_CLONES_SEC_WRITE_START\
    (KT_START | KT_CLONE_SEC | KT_WRITE | KT_CLONE)

// Mirroring Complete - 0x523
#define KT_TRAFFIC_CLONES_SEC_WRITE_COMPLETE\
    (KT_DONE | KT_CLONE_SEC | KT_WRITE | KT_CLONE)

//Traces For Zero Filling Operations

// Zero Fill Start - 0x5B2
#define KT_TRAFFIC_CLONES_ZERO_FILLING_START\
    (KT_START | KT_CLONE_ZERO | KT_WRITE | KT_CLONE)

// Zero Fill Done - 0x5B3
#define KT_TRAFFIC_CLONES_ZERO_FILLING_COMPLETE\
    (KT_DONE | KT_CLONE_ZERO | KT_WRITE | KT_CLONE)


//Traces For COD Operations

//COD Start - 0x548
#define KT_TRAFFIC_CLONES_COD_START\
    (KT_START | KT_CLONE_COD | KT_MOVE | KT_CLONE)

//COD Start - 0x549
#define KT_TRAFFIC_CLONES_COD_COMPLETE\
    (KT_DONE | KT_CLONE_COD | KT_MOVE | KT_CLONE)

//Traces For DM Operations

//DM Start - 0x588
#define KT_TRAFFIC_CLONES_DM_START\
    (KT_START | KT_CLONE_DM | KT_MOVE | KT_CLONE)

//DM Done - 0x589
#define KT_TRAFFIC_CLONES_DM_COMPLETE\
    (KT_DONE | KT_CLONE_DM | KT_MOVE | KT_CLONE)

//DM (Sync) Start - 0x598
#define KT_TRAFFIC_CLONES_SYNC_DM_START\
    (KT_START | KT_CLONE_SYNC_DM | KT_MOVE | KT_CLONE)

//DM (Sync)  Done - 0x599
#define KT_TRAFFIC_CLONES_SYNC_DM_COMPLETE\
    (KT_DONE | KT_CLONE_SYNC_DM | KT_MOVE | KT_CLONE)

//DM (Rev. Sync) Start - 0x5A8
#define KT_TRAFFIC_CLONES_REV_SYNC_DM_START\
    (KT_START | KT_CLONE_REV_SYNC_DM | KT_MOVE | KT_CLONE)

//DM Rev. (Sync)  Done - 0x5A9
#define KT_TRAFFIC_CLONES_REV_SYNC_DM_COMPLETE\
    (KT_DONE | KT_CLONE_REV_SYNC_DM | KT_MOVE | KT_CLONE)

//Traces For Synch-Rev. Synch  Operations

//Sync Start -0x562
#define KT_TRAFFIC_CLONES_SYNC_OPERATION_START\
    (KT_START | KT_CLONE_SYNC | KT_WRITE | KT_CLONE)

//Sync/Rev-Sync Done -0x563
#define KT_TRAFFIC_CLONES_SYNC_OPERATION_COMPLETE\
    (KT_DONE | KT_CLONE_SYNC | KT_WRITE | KT_CLONE)

//Rev-Sync Start -0x572
#define KT_TRAFFIC_CLONES_REV_SYNC_OPERATION_START\
    (KT_START | KT_CLONE_REV_SYNC | KT_WRITE | KT_CLONE)

//Sync/Rev-Sync Done -0x573
#define KT_TRAFFIC_CLONES_REV_SYNC_OPERATION_COMPLETE\
    (KT_DONE | KT_CLONE_REV_SYNC | KT_WRITE | KT_CLONE)


//Traces For Extent Change Notification Operations

// Write IOCTL Start - 0x5C2
#define KT_TRAFFIC_CLONES_ECN_START\
    (KT_START | KT_CLONE_ECN | KT_WRITE | KT_CLONE)

// Write IOCTL Done - 0x5C3
#define KT_TRAFFIC_CLONES_ECN_COMPLETE\
    (KT_DONE | KT_CLONE_ECN | KT_WRITE | KT_CLONE)

/***********************/
/* Compression Tracing */
/***********************/

//
// Regular host IO
// 

// Read Start 0x1a10
#define KT_TRAFFIC_COMPRESSION_READ_START(_flu)                         \
    (KT_START | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Read Done 0x1a11
#define KT_TRAFFIC_COMPRESSION_READ_DONE(_flu)                          \
    (KT_DONE | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Write Start 0x1a12
#define KT_TRAFFIC_COMPRESSION_WRITE_START(_flu)                        \
    (KT_START | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Write Done 0x1a13
#define KT_TRAFFIC_COMPRESSION_WRITE_DONE(_flu)                         \
    (KT_DONE | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Zero Start 0x1a14
#define KT_TRAFFIC_COMPRESSION_ZERO_START(_flu)                         \
    (KT_START | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Zero Done 0x1a15
#define KT_TRAFFIC_COMPRESSION_ZERO_DONE(_flu)                          \
    (KT_DONE | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Describe Extent Start 0x1a18
#define KT_TRAFFIC_COMPRESSION_DESC_EXT_START(_flu)                     \
    (KT_START | KT_GET_MDR | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Describe Extent Done 0x1a19
#define KT_TRAFFIC_COMPRESSION_DESC_EXT_DONE(_flu)                      \
    (KT_DONE | KT_GET_MDR | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mark Block Bad Start 0x1a16
#define KT_TRAFFIC_COMPRESSION_MBB_START(_flu)                          \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mark Block Bad Done 0x1a17
#define KT_TRAFFIC_COMPRESSION_MBB_DONE(_flu)                           \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))


//
// Internal Engine IO
// 

// Engine Read Start 0x2a20
#define KT_TRAFFIC_COMPRESSION_ENG_READ_START(_flu)                     \
    (KT_START | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Read Done 0x2a21
#define KT_TRAFFIC_COMPRESSION_ENG_READ_DONE(_flu)                      \
    (KT_DONE | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Write Start 0x2a22
#define KT_TRAFFIC_COMPRESSION_ENG_WRITE_START(_flu)                    \
    (KT_START | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Write Done 0x2a23
#define KT_TRAFFIC_COMPRESSION_ENG_WRITE_DONE(_flu)                     \
    (KT_DONE | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Zero Start 0x2a24
#define KT_TRAFFIC_COMPRESSION_ENG_ZERO_START(_flu)                     \
    (KT_START | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Zero Done 0x2a25
#define KT_TRAFFIC_COMPRESSION_ENG_ZERO_DONE(_flu)                      \
    (KT_DONE | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Describe Extent Start 0x2a28
#define KT_TRAFFIC_COMPRESSION_ENG_DESC_EXT_START(_flu)                 \
    (KT_START | KT_GET_MDR | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Describe Extent Done 0x2a29
#define KT_TRAFFIC_COMPRESSION_ENG_DESC_EXT_DONE(_flu)                  \
    (KT_DONE | KT_GET_MDR | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Mark Block Bad Start 0x2a26
#define KT_TRAFFIC_COMPRESSION_ENG_MBB_START(_flu)                      \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Mark Block Bad Done 0x2a27
#define KT_TRAFFIC_COMPRESSION_ENG_MBB_DONE(_flu)                       \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))


//
// Engine Operation Bracket events
//

// Engine Compress Chunk Start 0x2a80
#define KT_TRAFFIC_COMPRESSION_ENG_COMPRESS_CHUNK_START(_flu)           \
    (KT_START | KT_READ | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Compress Chunk Done 0x2a81
#define KT_TRAFFIC_COMPRESSION_ENG_COMPRESS_CHUNK_DONE(_flu)            \
    (KT_DONE | KT_READ | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Decompress Chunk Start 0x2a82
#define KT_TRAFFIC_COMPRESSION_ENG_DECOMPRESS_CHUNK_START(_flu)         \
    (KT_START | KT_WRITE | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Decompress Chunk Done 0x2a83
#define KT_TRAFFIC_COMPRESSION_ENG_DECOMPRESS_CHUNK_DONE(_flu)          \
    (KT_DONE | KT_WRITE | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Get Compressed Chunks Start 0x2a84
#define KT_TRAFFIC_COMPRESSION_ENG_GET_COMPRESSED_CHUNKS_START(_flu)    \
    (KT_START | KT_ZERO | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Get Compressed Chunks Done 0x2a85
#define KT_TRAFFIC_COMPRESSION_ENG_GET_COMPRESSED_CHUNKS_DONE(_flu)     \
    (KT_DONE | KT_ZERO | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Get Uncompressed Chunks Start 0x2a86
#define KT_TRAFFIC_COMPRESSION_ENG_GET_UNCOMPRESSED_CHUNKS_START(_flu)  \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))

// Engine Get Uncompressed Chunks Done 0x2a87
#define KT_TRAFFIC_COMPRESSION_ENG_GET_UNCOMPRESSED_CHUNKS_DONE(_flu)   \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_BG_VERIFY | KT_TRAFFIC_SESSION_ID(2) | KT_TRAFFIC_FLU_ID(_flu))


//
// Inline Compression IO
// 

// Mig Read Start 0x1a40
#define KT_TRAFFIC_COMPRESSION_MIG_READ_START(_flu)                     \
    (KT_START | KT_READ | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Read Done 0x1a41
#define KT_TRAFFIC_COMPRESSION_MIG_READ_DONE(_flu)                      \
    (KT_DONE | KT_READ | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Write Start 0x1a42
#define KT_TRAFFIC_COMPRESSION_MIG_WRITE_START(_flu)                    \
    (KT_START | KT_WRITE | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Write Done 0x1a43
#define KT_TRAFFIC_COMPRESSION_MIG_WRITE_DONE(_flu)                     \
    (KT_DONE | KT_WRITE | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Zero Start 0x1a44
#define KT_TRAFFIC_COMPRESSION_MIG_ZERO_START(_flu)                     \
    (KT_START | KT_ZERO | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Zero Done 0x1a45
#define KT_TRAFFIC_COMPRESSION_MIG_ZERO_DONE(_flu)                      \
    (KT_DONE | KT_ZERO | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Describe Extent Start 0x1a48
#define KT_TRAFFIC_COMPRESSION_MIG_DESC_EXT_START(_flu)                 \
    (KT_START | KT_GET_MDR | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Describe Extent Done 0x1a49
#define KT_TRAFFIC_COMPRESSION_MIG_DESC_EXT_DONE(_flu)                  \
    (KT_DONE | KT_GET_MDR | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Mark Block Bad Start 0x1a46
#define KT_TRAFFIC_COMPRESSION_MIG_MBB_START(_flu)                      \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))

// Mig Mark Block Bad Done 0x1a47
#define KT_TRAFFIC_COMPRESSION_MIG_MBB_DONE(_flu)                       \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_DESTINATION | KT_TRAFFIC_SESSION_ID(1) | KT_TRAFFIC_FLU_ID(_flu))


//
// Chunk State Cache (CSC) Events

// CSC Query Hit Start 0x3a10
#define KT_TRAFFIC_COMPRESSION_CSC_QUERY_HIT_START(_flu)                \
    (KT_START | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Query Hit Done 0x3a11
#define KT_TRAFFIC_COMPRESSION_CSC_QUERY_HIT_DONE(_flu)                 \
    (KT_DONE | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Query Miss Start 0x3a12
#define KT_TRAFFIC_COMPRESSION_CSC_QUERY_MISS_START(_flu)               \
    (KT_START | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Query Miss Done 0x3a13
#define KT_TRAFFIC_COMPRESSION_CSC_QUERY_MISS_DONE(_flu)                \
    (KT_DONE | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Uncompressed Start 0x3a20
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_UNCOMPRESSED_START(_flu)      \
    (KT_START | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Uncompressed Done 0x3a21
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_UNCOMPRESSED_DONE(_flu)       \
    (KT_DONE | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Compressed Start 0x3a22
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_COMPRESSED_START(_flu)        \
    (KT_START | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Compressed Done 0x3a23
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_COMPRESSED_DONE(_flu)         \
    (KT_DONE | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Hole Start 0x3a24
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_HOLE_START(_flu)              \
    (KT_START | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Submit Hole Done 0x3a25
#define KT_TRAFFIC_COMPRESSION_CSC_SUBMIT_HOLE_DONE(_flu)               \
    (KT_DONE | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Invalidate Start 0x3a30
#define KT_TRAFFIC_COMPRESSION_CSC_INVALIDATE_START(_flu)               \
    (KT_START | KT_READ | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))

// CSC Invalidate Done 0x3a31
#define KT_TRAFFIC_COMPRESSION_CSC_INVALIDATE_DONE(_flu)                \
    (KT_DONE | KT_READ | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(3) | KT_TRAFFIC_FLU_ID(_flu))


//
// Uncompressed Data Cache (UDC) Events
// 

// UDC Pin Write Long Term Start 0x4a10
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_WRITE_LONG_TERM_START(_flu)      \
    (KT_START | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Write Long Term Done 0x4a11
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_WRITE_LONG_TERM_DONE(_flu)       \
    (KT_DONE | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Write Short Term Start 0x4a12
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_WRITE_SHORT_TERM_START(_flu)     \
    (KT_START | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Write Short Term Done 0x4a13
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_WRITE_SHORT_TERM_DONE(_flu)      \
    (KT_DONE | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Read Long Term Start 0x4a14
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_READ_LONG_TERM_START(_flu)       \
    (KT_START | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Read Long Term Done 0x4a15
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_READ_LONG_TERM_DONE(_flu)        \
    (KT_DONE | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Read Short Term Start 0x4a16
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_READ_SHORT_TERM_START(_flu)      \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Pin Read Short Term Done 0x4a17
#define KT_TRAFFIC_COMPRESSION_UDC_PIN_READ_SHORT_TERM_DONE(_flu)       \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Commit Start 0x4a20
#define KT_TRAFFIC_COMPRESSION_UDC_COMMIT_START(_flu)                   \
    (KT_START | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Commit Done 0x4a21
#define KT_TRAFFIC_COMPRESSION_UDC_COMMIT_DONE(_flu)                    \
    (KT_DONE | KT_READ | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Cancel Start 0x4a22
#define KT_TRAFFIC_COMPRESSION_UDC_CANCEL_START(_flu)                   \
    (KT_START | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Cancel Done 0x4a23
#define KT_TRAFFIC_COMPRESSION_UDC_CANCEL_DONE(_flu)                    \
    (KT_DONE | KT_WRITE | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Invalidate Start 0x4a24
#define KT_TRAFFIC_COMPRESSION_UDC_INVALIDATE_START(_flu)               \
    (KT_START | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Invalidate Done 0x4a25
#define KT_TRAFFIC_COMPRESSION_UDC_INVALIDATE_DONE(_flu)                \
    (KT_DONE | KT_ZERO | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Release Start 0x4a26
#define KT_TRAFFIC_COMPRESSION_UDC_RELEASE_START(_flu)                  \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Release Done 0x4a27
#define KT_TRAFFIC_COMPRESSION_UDC_RELEASE_DONE(_flu)                   \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_SECONDARY | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Compressed True Start 0x4a30
#define KT_TRAFFIC_COMPRESSION_UDC_SET_COMPRESSED_TRUE_START(_flu)      \
    (KT_START | KT_READ | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Compressed True Done 0x4a31
#define KT_TRAFFIC_COMPRESSION_UDC_SET_COMPRESSED_TRUE_DONE(_flu)       \
    (KT_DONE | KT_READ | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Compressed False Start 0x4a32
#define KT_TRAFFIC_COMPRESSION_UDC_SET_COMPRESSED_FALSE_START(_flu)     \
    (KT_START | KT_WRITE | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Compressed False Done 0x4a33
#define KT_TRAFFIC_COMPRESSION_UDC_SET_COMPRESSED_FALSE_DONE(_flu)      \
    (KT_DONE | KT_WRITE | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Hole True Start 0x4a34
#define KT_TRAFFIC_COMPRESSION_UDC_SET_HOLE_TRUE_START(_flu)            \
    (KT_START | KT_ZERO | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Hole True Done 0x4a35
#define KT_TRAFFIC_COMPRESSION_UDC_SET_HOLE_TRUE_DONE(_flu)             \
    (KT_DONE | KT_ZERO | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Hole False Start 0x4a36
#define KT_TRAFFIC_COMPRESSION_UDC_SET_HOLE_FALSE_START(_flu)           \
    (KT_START | KT_CRPT_CRC | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))

// UDC Set Hole False Done 0x4a37
#define KT_TRAFFIC_COMPRESSION_UDC_SET_HOLE_FALSE_DONE(_flu)            \
    (KT_DONE | KT_CRPT_CRC | KT_MLU | KT_SOURCE | KT_TRAFFIC_SESSION_ID(4) | KT_TRAFFIC_FLU_ID(_flu))


//
// Compress / Decompress Routine Events
//

// Compress Chunk Start 0x5a10
#define KT_TRAFFIC_COMPRESSION_COMPRESS_CHUNK_START(_flu)               \
    (KT_START | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

// Compress Chunk Done 0x5a11
#define KT_TRAFFIC_COMPRESSION_COMPRESS_CHUNK_DONE(_flu)                \
    (KT_DONE | KT_READ | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

// Decompress Chunk Start 0x5a12
#define KT_TRAFFIC_COMPRESSION_DECOMPRESS_CHUNK_START(_flu)             \
    (KT_START | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

// Decompress Chunk Done 0x5a13
#define KT_TRAFFIC_COMPRESSION_DECOMPRESS_CHUNK_DONE(_flu)              \
    (KT_DONE | KT_WRITE | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

// Skip Decompress Chunk Start 0x5a14
#define KT_TRAFFIC_COMPRESSION_SKIP_DECOMPRESS_CHUNK_START(_flu)        \
    (KT_START | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

// Skip Decompress Chunk Done 0x5a15
#define KT_TRAFFIC_COMPRESSION_SKIP_DECOMPRESS_CHUNK_DONE(_flu)         \
    (KT_DONE | KT_ZERO | KT_MLU | KT_PRIMARY | KT_TRAFFIC_SESSION_ID(5) | KT_TRAFFIC_FLU_ID(_flu))

/***********************/
/* Flash Cache Tracing */
/***********************/

// 0x00000b00
#define KT_TRAFFIC_FCT_READ_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b01
#define KT_TRAFFIC_FCT_READ_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b02
#define KT_TRAFFIC_FCT_WRITE_START(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_START | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b03
#define KT_TRAFFIC_FCT_WRITE_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_DONE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b04
#define KT_TRAFFIC_FCT_ZERO_START(_lun) \
(KT_FCT | KT_TRAFFIC_ZERO_START | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b05
#define KT_TRAFFIC_FCT_ZERO_DONE(_lun) \
(KT_FCT | KT_TRAFFIC_ZERO_DONE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b06
#define KT_TRAFFIC_FCT_CRPT_CRC_START(_lun) \
(KT_FCT | KT_TRAFFIC_CRPT_CRC_START | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b07
#define KT_TRAFFIC_FCT_CRPT_CRC_DONE(_lun) \
(KT_FCT | KT_TRAFFIC_CRPT_CRC_DONE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b10
#define KT_TRAFFIC_FCT_IDLE_CLEAN_PIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b11
#define KT_TRAFFIC_FCT_IDLE_CLEAN_PIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b18
#define KT_TRAFFIC_FCT_IDLE_CLEAN_UNPIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_START | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b19
#define KT_TRAFFIC_FCT_IDLE_CLEAN_UNPIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_DONE | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b1a
#define KT_TRAFFIC_FCT_IDLE_CLEAN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_START | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b1b
#define KT_TRAFFIC_FCT_IDLE_CLEAN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_DONE | KT_FCT_IDLE_CLEAN | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b20
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_PIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b21
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_PIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b28
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_UNPIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_START | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b29
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_UNPIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_DONE | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b2a
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_START(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_START | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b2b
#define KT_TRAFFIC_FCT_CACHE_DEVICE_FLUSH_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_DONE | KT_FCT_CACHE_DEVICE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b30
#define KT_TRAFFIC_FCT_IDLE_FLUSH_PIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b31
#define KT_TRAFFIC_FCT_IDLE_FLUSH_PIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b38
#define KT_TRAFFIC_FCT_IDLE_FLUSH_UNPIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_START | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b39
#define KT_TRAFFIC_FCT_IDLE_FLUSH_UNPIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_DONE | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b3a
#define KT_TRAFFIC_FCT_IDLE_FLUSH_START(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_START | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b3b
#define KT_TRAFFIC_FCT_IDLE_FLUSH_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_DONE | KT_FCT_IDLE_FLUSH | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b40
#define KT_TRAFFIC_FCT_PROMOTE_PIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b41
#define KT_TRAFFIC_FCT_PROMOTE_PIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b48
#define KT_TRAFFIC_FCT_PROMOTE_UNPIN_START(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_START | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b49
#define KT_TRAFFIC_FCT_PROMOTE_UNPIN_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_MOVE_DONE | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b4a
#define KT_TRAFFIC_FCT_PROMOTE_START(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_START | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b4b
#define KT_TRAFFIC_FCT_PROMOTE_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_COPY_DONE | KT_FCT_PROMOTE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b52
#define KT_TRAFFIC_FCT_DIRUPDATE_START(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_START | KT_FCT_DIRUPDATE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b53
#define KT_TRAFFIC_FCT_DIRUPDATE_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_DONE | KT_FCT_DIRUPDATE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b62
#define KT_TRAFFIC_FCT_CDUPDATE_START(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_START | KT_FCT_CDUPDATE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b63
#define KT_TRAFFIC_FCT_CDUPDATE_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_DONE | KT_FCT_CDUPDATE | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b72
#define KT_TRAFFIC_FCT_SYNC_START(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_START | KT_FCT_SYNC | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b73
#define KT_TRAFFIC_FCT_SYNC_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_DONE | KT_FCT_SYNC | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b80
#define KT_TRAFFIC_FCT_READ_FLASH_START(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_START | KT_FCT_FLASH_REQUEST | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b81
#define KT_TRAFFIC_FCT_READ_FLASH_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_READ_DONE | KT_FCT_FLASH_REQUEST | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b82
#define KT_TRAFFIC_FCT_WRITE_FLASH_START(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_START | KT_FCT_FLASH_REQUEST | KT_TRAFFIC_FLU_ID(_lun))

// 0x00000b83
#define KT_TRAFFIC_FCT_WRITE_FLASH_DONE(_lun) \
    (KT_FCT | KT_TRAFFIC_WRITE_DONE | KT_FCT_FLASH_REQUEST | KT_TRAFFIC_FLU_ID(_lun))

/***********************/
/* Dedup Server(DDS) Tracing */
/***********************/

//0X00000a10
#define KT_TRAFFIC_DDS_DIGEST_REQUEST_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_DIGEST_REQUEST) 

//0X00000a11
#define KT_TRAFFIC_DDS_DIGEST_REQUEST_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_DIGEST_REQUEST) 

//0X00000a20
#define KT_TRAFFIC_DDS_DIGEST_IOD_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_DIGEST_IOD) 

//0X00000a21
#define KT_TRAFFIC_DDS_DIGEST_IOD_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_DIGEST_IOD) 

//0X00000a30
#define KT_TRAFFIC_DDS_DIGEST_CALCULATION_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_DIGEST_CALCULATION) 

//0X00000a31
#define KT_TRAFFIC_DDS_DIGEST_CALCULATION_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_DIGEST_CALCULATION) 

//0X00000a40
#define KT_TRAFFIC_DDS_DEDUP_REQUEST_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_DEDUP_REQUEST) 

//0X00000a41
#define KT_TRAFFIC_DDS_DEDUP_REQUEST_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_DEDUP_REQUEST) 

//0X00000a50
#define KT_TRAFFIC_DDS_READ_CHUNKS_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_READ_CHUNKS) 

//0X00000a51
#define KT_TRAFFIC_DDS_READ_CHUNKS_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_READ_CHUNKS) 

//0X00000a60
#define KT_TRAFFIC_DDS_READ_KEY_IOD_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_READ_KEY_IOD) 

//0X00000a61
#define KT_TRAFFIC_DDS_READ_KEY_IOD_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_READ_KEY_IOD) 

//0X00000a70
#define KT_TRAFFIC_DDS_BIT_COMPARE_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_BIT_COMPARE) 

//0X00000a71
#define KT_TRAFFIC_DDS_BIT_COMPARE_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_BIT_COMPARE) 

//0X00000a80
#define KT_TRAFFIC_DDS_DEDUP_CHUNK_START \
    (KT_MLU | KT_TRAFFIC_READ_START | KT_DDS_DEDUP_CHUNK) 

//0X00000a81
#define KT_TRAFFIC_DDS_DEDUP_CHUNK_DONE \
    (KT_MLU | KT_TRAFFIC_READ_DONE | KT_DDS_DEDUP_CHUNK) 

/*************************/
/* SPC Initiated Tracing */
/*************************/
#define KT_TRAFFIC_SPC_HOST_READ \
    (KT_SPC | KT_SPC_HOST_REQUEST | KT_READ)

#define KT_TRAFFIC_SPC_HOST_WRITE_THROUGH \
    (KT_SPC | KT_SPC_HOST_REQUEST | KT_WRITE)

#define KT_TRAFFIC_SPC_PREFETCH \
    (KT_SPC | KT_SPC_PREFETCH | KT_READ)

#define KT_TRAFFIC_SPC_BACKFILL \
    (KT_SPC | KT_SPC_BACKFILL | KT_READ)

#define KT_TRAFFIC_SPC_FLUSH \
    (KT_SPC | KT_SPC_FLUSH | KT_WRITE)

/***************/
/* TDX Tracing */
/***************/

// 0x0000
#define KT_TRAFFIC_TDX_READ_START               ( KT_START | KT_READ )

// 0x0001
#define KT_TRAFFIC_TDX_READ_DONE                ( KT_DONE |  KT_READ )

// 0x0002
#define KT_TRAFFIC_TDX_WRITE_START              ( KT_START | KT_WRITE )

// 0x0003
#define KT_TRAFFIC_TDX_WRITE_DONE               ( KT_DONE |  KT_WRITE )

// 0x000a
#define KT_TRAFFIC_TDX_COPY_START               ( KT_START | KT_COPY )

// 0x000b
#define KT_TRAFFIC_TDX_COPY_DONE                ( KT_DONE | KT_COPY )

// 0x0008
#define KT_TRAFFIC_TDX_MOVE_START               ( KT_START | KT_MOVE )

// 0x0009
#define KT_TRAFFIC_TDX_MOVE_DONE                ( KT_DONE | KT_MOVE )

// 0x0010
#define KT_TRAFFIC_TDX_HOST_READ_START          ( KT_START | KT_READ | KT_TDX_HOST_REQUEST )

// 0x0011
#define KT_TRAFFIC_TDX_HOST_READ_DONE           ( KT_DONE | KT_READ | KT_TDX_HOST_REQUEST ) 

// 0x0012
#define KT_TRAFFIC_TDX_HOST_WRITE_START         ( KT_START | KT_WRITE | KT_TDX_HOST_REQUEST )

// 0x0013
#define KT_TRAFFIC_TDX_HOST_WRITE_DONE          ( KT_DONE | KT_WRITE | KT_TDX_HOST_REQUEST )


#ifdef __cplusplus
};
#endif

#endif // defined __KTRACE_STRUCT_H__
