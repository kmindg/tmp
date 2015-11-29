#ifndef KTRACEINTF_H
#define KTRACEINTF_H

#include "EmcPAL_Ioctl.h"

/*
** KtraceIntf.h
**	This file contains information on the interface between ktrace
**	and ktrace console in the user space.
*/

//
// Various file names for Ktrace.sys
//
#define KTRACE_UMODE_NT_DEVICE_NAME   "\\\\.\\Ktrace"
#define KTRACE_NT_DEVICE_NAME        "\\Device\\Ktrace"
#define KTRACE_WIN32_DEVICE_NAME     "\\DosDevices\\Ktrace"

//
// IOCTL codes
//
#define IOCTL_KTRACE_GET_EXPRESSION		\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC88, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_READ_MEMORY		 \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC89, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_DUMPSTAMP			 \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8A, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_WRITE_MEMORY		 \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8B, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_READ_PHYSICAL               \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8C, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_WRITE_PHYSICAL              \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8D, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_READ_IO_SPACE               \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8E, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_WRITE_IO_SPACE              \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC8F, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_KUTRACEX                    \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC90, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_RINGS                       \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC91, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_FILE                        \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC92, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_GET_SYMBOL_INFO                        \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC93, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)


#define IOCTL_KTRACE_GET_TYPE_SIZE		\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC94, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_DUMP_SYM_INFO		\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC95, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_FIND_ADDR_WOW64		\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC96, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_KTRACE_KUTRACEX_DIRECT		\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0xC97, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)


//
// structures for IOCTL arguments and returned values
//
typedef union physical_addr_struct
{
  struct {
	unsigned int LowPart;
	unsigned int HiPart;
  }
  AddrParts;

  csx_u64_t HiAndLowParts;

} physicalAddr_T;

typedef enum kt_rwm_ioctl_enum
{
  Kt_rwmCLR  = 0,		/* not set */
  Kt_rwmVIRT = 1 << 0,		/* passing a virtual address */
  Kt_rwmPHYS = 1 << 1,		/* passing a physical address */
  Kt_rwmIO   = 1 << 2,		/* passing an io space address */
} Kt_RWM_T;

//
// IOCTL for read/write memory.
//
typedef struct _kt_rwmemory_ioctl
{
  Kt_RWM_T flags;		// access modifiers
  physicalAddr_T  addr;		// address to begin access
  unsigned long rwLen;		// Bytes of data to be read/written to.
  // for write operations the write data immediately follows this request
} KT_RWMEMORY_IOCTL, *PKT_RWMEMORY_IOCTL;

/*
 * IOCTL_KTRACE_RINGS
 */

/*
 * Kt_RINGS_T
 *
 * DESCRIPTION:  This enum defines the action codes used by the
 *   IOCTL_KTRACE_RINGS.
 *
 *   Kt_ringNAME -- return the rings' short names
 *   Kt_ringTEXT -- return the rings' longer text
 */
typedef enum kt_rings_ioctl_enum
{
  Kt_ringNAME = 0,
  Kt_ringTEXT = 1,
  Kt_ringMAX  = Kt_ringTEXT
} Kt_RINGS_T;

/*
 * Kt_ring_ioctl_T
 *
 * DESCRIPTION: This structure defines the layout of the arguments
 *   sent to IOCTL_KTRACE_RINGS.
 *
 *           +----+
 * .code     |    | Kt_RINGS_T
 *           +----+
 * .reserved |  0 | alignment
 *           +----+----+
 * .size     |         | number of bytes in buffer
 *           +----+----+
 *
 */
typedef struct kt_ring_ioctl_struct
{
  unsigned char       code;	/* Kt_RING_T */
  unsigned char       reserved;	/* padding for alignment */
  unsigned short int  size;	/* bytes in buffer */
} Kt_ring_ioctl_T;

/*
 * DESCRIPTION: This is the return buffer layout for IOCTL_KTRACE_RINGS
 *
 *    begin: +----+----+
 *    +------|offset_0 | offset values are from begin:
 *    |      +----+----+
 *    | +----|offset_1 |
 *    | |    +----+----+
 *    | |    :         :
 *    | |    +----+----+
 *    | | +--|offset_n |
 *    | | |  +----+----+
 *    | | |  |    0    | 0 offset terminates the list
 *    | | |  +----+----+----+----+ ... +----+
 *    +-)-)->|"name" or "text"          EOS |
 *      | |  +----+----+----+----+ ... +----+
 *      +-)->|"name" or "text"          EOS |
 *        |  +----+----+----+----+ ... +----+
 *        |  :                              :
 *        |  +----+----+----+----+ ... +----+
 *        +->|"name" or "text"          EOS |
 *           +----+----+----+----+ ... +----+
 *
 * ERRORS:
 *  If there is insufficient space in the buffer to hold the requested
 *  information xxx status is returned and the first short int in the 
 *  buffer is set to the required size.  If the buffer is not at least
 *  two bytes long, it is not updated.
 *
 */
#define KT_RING_IOCTL_OUT(m_name, m_offsets, m_bytes)	\
union m_name##_union					\
{							\
  Kt_ring_ioctl_T    in;				\
  unsigned short int offsets[m_offsets];		\
  char               strings[(m_bytes)+2*(m_offsets)];	\
} m_name

// CTS 
// 


typedef KT_RING_IOCTL_OUT(Kt_ring_ioctl_out_T, 1, 1);

/*
 * IOCTL_KTRACE_FILE
 */

/* Kt_FILE_T
 *
 * DESCRIPTION: This enum defines the codes used in IOCTL_KTRACE_FILE to
 *   request the sub functions of the IOCTL>
 */
typedef enum kt_file_ioctl_enum
{
  Kt_filePAUSE      = 0,  /* Kt_file_ring_T -- suspend writing to the file */
  Kt_fileSTART      = 1,  /* Kt_file_ring_T -- resume writing to the file */
  Kt_fileDEALLOCATE = 2,  /* Kt_file_ring_T -- deallocate the ring's buffers */
  Kt_fileALLOC      = 3,  /* Kt_file_size_T -- allocate the ring's buffers */
  Kt_fileFLAGS      = 4,  /* Kt_file_flag_T -- operate on the ring's flags */
  Kt_fileOPEN       = 5,  /* Kt_file_name_T -- open a file, truncating */
  Kt_printThrdPrio = 6, /* Kt_file_flag_T -- To set PrintThread Priority*/
  Kt_fileCLOSE      = 7,  /* Kt_file_ring_T -- close a file */
  Kt_fileWrapOff    = 9,  /* Kt_file_ring_T -- turn off wrapper */
  Kt_fileWrapOn     = 10, /* Kt_file_size_T -- turn on  wrapper */
  Kt_fileTRAFFICoff = 11, /* Kt_file_ring_T -- turn off traffic counts */
  Kt_fileTRAFFICon  = 12, /* Kt_file_ring_T -- turn on  traffic counts */
  Kt_fileTRAFFICis  = 13, /* Kt_file_ring_T -- return count on/off status */
  Kt_fileTFLAG      = 14, /* Kt_file_flag_T -- operate on ring's tflag */
  Kt_fileBuffSize     =15, /* Kt_file_flag_T -- Set the buffer size */
  Kt_fileTime      =16, /* Kt_file_flag_T -- Set time duration to collect trace */
  
} Kt_FILE_T;

/*
 * Kt_file_ring_T
 *
 * DESCRIPTION: This structure defines the arguments sent to IOCTL_KTRACE_FILE
 *   for those operations that only need to specify the ring id.
 *
 *       +----+
 * .code |    | Kt_FILE_T
 *       +----+
 * .ring |    | KTRACE_ring_id_T
 *       +----+
 *
 * 
 */
typedef struct kt_file_ioctl_ring_struct
{
  unsigned char code;		/* Kt_FILE_T (PAUSE,START,DEALLOC,CLOSE,FLUSH,
				 * MARKoff, TRAFFICoff, TRAFFICon, TRAFFICis) */
  unsigned char ring;		/* KTRACE_ring_id_T */
} Kt_file_ring_T;

/*
 * Kt_file_size_T
 *
 * DESCRIPTION: This structure defines the arguments sent to IOCTL_KTRACE_FILE
 *   for those operations that only need to specify the ring id and a memory
 *   size.
 *
 *       +----+
 * .code |    | Kt_FILE_T
 *       +----+
 * .ring |    | KTRACE_ring_id_T
 *       +----+----+
 *       |    0    | alignment
 *       +----+----+----+----+
 * .size |                   | number of slots to allocate / mark interval
 *       +----+----+----+----+
 *
 */
typedef struct kt_file_ioctl_size_struct
{
  unsigned char code;		/* Kt_FILE_T (ALLOC, MARKon) */
  unsigned char ring;		/* KTRACE_ring_id_T */
  short int     reserved;	/* alignment */
  unsigned int  size;		/* slot count */
} Kt_file_size_T;

/*
 * Kt_file_flag_T
 *
 * DESCRIPTION: This structure defines the arguments sent to IOCTL_KTRACE_FILE
 *   for those operations that manipulate the ring's flags.  By setting .set
 *   and .clear to 0 we can obtain the current value of the flag without
 *   altering it.
 *
 *           +----+
 * .code     |    | Kt_FILE_T
 *           +----+
 * .ring     |    | KTRACE_ring_id_T 
 *           +----+----+
 *           |    0    | alignment
 *           +----+----+
 * .set      |         | KTRC_flags_T -- bits to set in ring's flag
 *           +----+----+
 * .clear    |         | KTRC_flags_T -- bits to clear in ring's flag
 *           +----+----+
 *
 */
typedef struct kt_file_ioctl_flag_struct
{
  unsigned char code;		/* Kt_FILE_T (FLAG) */
  unsigned char ring;		/* KTRACE_ring_id_T */
  short int     reserved;	/* alignment */
  int     set;		/* bits to set in flag */
  int     clear;		/* bits to clear in flag */
} Kt_file_flag_T;

/*
 * DESCRIPTION: This is the return structure for the Kt_fileFLAG request:
 *
 *           +----+----+
 *           |         | original value of flag (before set/clear)
 *           +----+----+
 */


/*
 * Kt_file_name_T
 *
 * DESCRIPTION: This structure defines the arguments sent to IOCTL_KTRACE_FILE
 *   for those operations that need a file path/name.
 *
 *                   +----+
 * .code             |    | Kt_FILE_T
 *                   +----+
 * .ring             |    | KTRACE_ring_id_T
 *                   +----+----+
 * .size             |         | size of .name in bytes
 *                   +----+----+ ... +----+----+
 * .name             | "path/name"          EOS| 
 *                   +----+----+ ... +----+----+
 *
 */
typedef struct kt_file_ioctl_name_struct
{
  unsigned char      code;	/* Kt_FILE_T (OPEN, APPEND) */
  unsigned char      ring;	/* KTRACE_ring_id_T */
  unsigned short int size;	/* size of name in bytes */
  char               name[1];	/* file path/name */
} Kt_file_name_T;

#endif /* ndef KTRACEINTF_H */
