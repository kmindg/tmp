#if !defined(KTCONS_LOCAL_H)
#define KTCONS_LOCAL_H

#include "csx_ext.h"

//typedef ULONG ULONG_PTR;

#if !defined(BOOL_H)
#include "bool.h"
#endif
#include "ktrace_IOCTL.h"

#define KDEXT_64BIT
#include <wdbgexts.h>
#include "compare.h"   // for MIN function

/*
typedef struct _TCP_SYM_DUMP_PARAM {
    SYM_DUMP_PARAM  tcpsymDumpParam;
    UCHAR                   tcpsName[64];
    UCHAR                   tcpfName[64];
    ULONG                   fOptions;
} TCP_SYM_DUMP_PARAM, *PTCP_SYM_DUMP_PARAM;
*/
#define TCP_SYM_NAME_LENGTH (64)
typedef struct _TCP_SYM_DUMP_PARAM {
  ULONG               size;          // size of this struct... from SYM_DUMP_PARAM
  ULONG               Options;       // Dump options ... from SYM_DUMP_PARAM
  ULONG64             addr;          // Address to take data for type... from SYM_DUMP_PARAM
  ULONG               nFields;       // # elements in Fields... from SYM_DUMP_PARAM
  UCHAR               tcpsName[TCP_SYM_NAME_LENGTH];
  UCHAR               tcpfName[TCP_SYM_NAME_LENGTH];
  ULONG                   fOptions;
} TCP_SYM_DUMP_PARAM, *PTCP_SYM_DUMP_PARAM;

//Define a custom exception for Drivers not loaded
#define STATUS_DRIVERS_NOT_LOADED       0xE0000001


/*
 * CONDITIONALS
 *   Very and somewhat constant definitions used by KtCons.
 */
#if !defined(KTCONS_DEF_TCP_PORT) /* default TCP/IP port number */
#define KTCONS_DEF_TCP_PORT 13456
#endif
#if !defined(KTCONS_DEF_DEBUG_MODE) /* default setting for debug mode */
#define KTCONS_DEF_DEBUG_MODE KT_DBG_ALWAYS
#endif
#if !defined(KTCONS_INIT_FILE) /* initialization file */
#define KTCONS_INIT_FILE "_ktcons.ini"
#endif
#if !defined(KTLOGGING_INIT_FILE) /* initialization file */
#define KTLOGGING_INIT_FILE "_ktlog.ini"
#endif

#if !defined(KTCONS_INITIAL_CMD) /* startup command */
#define KTCONS_INITIAL_CMD ".source " KTCONS_INIT_FILE "\r\n"
#endif
#if !defined(KTCONS_KEY_PATH)	/* path from HKEY_LOCAL_MACHINE to KtCons */
#define KTCONS_KEY_PATH	"SOFTWARE\\EMC\\K10\\ktcons"
#endif
#if !defined(KTCONS_KEY_DEBUG_MODE) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_DEBUG_MODE "dbgMode"
#endif
#if !defined(KTCONS_KEY_TCP_PORT) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_TCP_PORT "tcpPort"
#endif
#if !defined(KTCONS_KEY_KTLOG) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_KTLOG "KtLog"
#endif
#if !defined(KTCONS_KEY_DUMP_AT_SHUTDOWN) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_DUMP_AT_SHUTDOWN "KtDumpAtShutdown"
#endif
#if !defined(KTCONS_KEY_DUMP_FILE_COUNT) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_DUMP_FILE_COUNT "KtDumpFileCount"
#endif
#if !defined(KTCONS_KEY_DUMP_AT_SHUTDOWN) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_DUMP_AT_SHUTDOWN "DumpKtraceBufAtShutdown"
#endif
#if !defined(KTCONS_KEY_DUMP_FILE_COUNT) /* symbol in SOFTWARE\...\ktcons  */
#define KTCONS_KEY_DUMP_FILE_COUNT "DumpKtraceFileCount"
#endif
#if !defined(KTCONS_PROMPT_RO)	/* prompt string in read only mode */
#define KTCONS_PROMPT_RO "> "
#endif
#if !defined(KTCONS_PROMPT_RW)	/* prompt string in read write mode */
#define KTCONS_PROMPT_RW "# "
#endif
#if !defined(KTCONS_ROOT_KEY)	/* base for KTCONS_KEY_PATH */
#define KTCONS_ROOT_KEY HKEY_LOCAL_MACHINE
#endif

#if !defined(Kt_ASCII_COUNT)	/* ascii characters per line of DA */
#define Kt_ASCII_COUNT 32
#endif
#if !defined(Kt_BUFSIZE)
#define Kt_BUFSIZE 64
#endif
#if !defined(Kt_FILEPATHBUFSIZE)
#define Kt_FILEPATHBUFSIZE 128
#endif
#if !defined(Kt_BYTE_COUNT)	/* byte characters per line of DB */
#define Kt_BYTE_COUNT 16
#endif
#if !defined(Kt_CMDBUFSIZE)	/* keyboard command buffer size */
#define Kt_CMDBUFSIZE 256
#endif

#if !defined(Kt_DEFAULTLOG)	/* default name for log file */
#define Kt_DEFAULTLOG "KtCons.log"
#endif
#if !defined(Kt_DPRINT_BUFFER)	/* max chars for nt_dprintf() output */
#define Kt_DPRINT_BUFFER 2048
#endif
#if !defined(Kt_FILECACHESIZE)
#define Kt_FILECACHESIZE 256
#endif
#if !defined(Kt_INT_COUNT)	/* words per line of DD */
#define Kt_INT_COUNT 4
#endif
#if !defined(Kt_IOCTL_BUFFER)	/* buffer for reading ioctl data */
#define Kt_IOCTL_BUFFER 256
#endif
#if !defined(Kt_MODULECACHESIZE)
#define Kt_MODULECACHESIZE 100
#endif
#if !defined(Kt_SHORT_COUNT)	/* short words per line of DW */
#define Kt_SHORT_COUNT 8
#endif
#if !defined(Kt_SOURCE_BUFFER)	/* buffer for reading from .source files */
#define Kt_SOURCE_BUFFER 512
#endif
#if !defined(Kt_SRCSHARE)	/* depth of .source command nesting */
#define Kt_SRCSHARE 12
#endif
#if !defined(Kt_STDD_LINES)	/* lines to display for each D* command */
#define Kt_STDD_LINES 8
#endif
#if !defined(Kt_PRINTF_BUFFER)	/* max chars for KtPrintf() output */
#define Kt_PRINTF_BUFFER 2048
#endif
#if !defined(Kt_PROCESSCACHESIZE)
#define Kt_PROCESSCACHESIZE 100
#endif
#if !defined(Kt_UNICODE_COUNT)	/* unicode characters per line of DU ??? */
#define Kt_UNICODE_COUNT 8
#endif
#if !defined(Kt_WAIT_CONN)	/* number of milliseconds to wait for retry
				 * of connection on TCP/IP */
#define Kt_WAIT_CONN 3000
#endif
#if !defined(Kt_WAIT_KTRACE_CF) /* number of milliseconds to wait for retry
                                 * of CreateFile to access ktracedev.sys */
#define Kt_WAIT_KTRACE_CF 5000
#endif
#if !defined(Kt_WRITE_BUFFER)	/* max chars for memory write operations */
#define Kt_WRITE_BUFFER 2048
#endif

#if !defined(Kt_MAXNAMELEN) /* max chars for symbol or for other names */
#define Kt_MAXNAMELEN 128
#endif

#if !defined(DUMPS_FOLDER)
#define DUMPS_FOLDER  "c:\\dumps\\"
#endif

/*
 * charNames_T
 *
 */
enum charNames_T
{
  EOS = '\000',	  /* end of string sentinel */
  STAR = '*',     /* comment introducer */
  BANG = '!',	  /* extended command introducer */
  DOT  = '.',	  /* separates dllname from command name */
  QM   = '?',	  /* display value, .? & !? help */
  LF   = '\n',    /* end of line delimiter */
  HT   = '\t',    /* horizontal tab */
  VT   = '\v',    /* vertical tab */
  BS   = '\b',    /* back space */
  CR   = '\r',    /* carriage return */
  FF   = '\f',    /* form feed */
  BEL  = '\a',    /* bell */
};

/*
 * Kt_errorCode_T
 *
 * DESCRIPTION: This enum defines the error codes returned by
 *   Kt_GetSymbolValue().
 */
typedef enum Kt_errorCode_enum
{
  Kt_SUCCESS = 0,		/* success -- symbol value returned */
  Kt_NOSYMBOL,			/* no symbol name supplied */
  Kt_TOKTOOLONG,		/* path, file name, or type too long */
  Kt_ntSYMINIT,			/* SymInitialize() call failed */
  Kt_ntSYMLOAD,			/* SymLoadModule() call failed */
  Kt_ntENUMDEVDRVR,		/* EnumDeviceDrivers() call failed */
  Kt_SYMNOTFOUND,		/* SymGetSymFromName() returned failure */
  Kt_ntREGACCESS,		/* RegOpenKeyEx() or RegQueryValueEx() err */
  Kt_ntENUMPROC,		/* EnumProcesses() call failed */
  Kt_UnknownSymInfoOp,  /* Unknow Symbol Info OP Code */
} Kt_errorCode_T;

/*
 * Kt_fileCache_T
 *
 * DESCRIPTION: This structure defines the contents of a (driver) file
 *   cache entry.
 *
 *              +-----------------------------------+
 *    refCount: |          unsigned int             | 0 = unused; >0 = number of hits
 *              +-----------------------------------+
 * baseAddress: |          long int                 | address from image file
 *              +-----------------------------------+
 * loadAddress: |          void*                    | address in memory
 *              +-----------------------------------+
 *     flagDrv: |          bool                     | true if entry from _driver()
 *              +-----------------------------------+
 *    filePath: |          char[Kt_FILEPATHBUFSIZE] | path to file
 *              +-----------------------------------+
 *    fileName: |          char[Kt_BUFSIZE]         | name of file != ""
 *              +-----------------------------------+
 *    fileType: |          char[Kt_BUFSIZE]         | type of file (extension)
 *              +-----------------------------------+
 *
 * NOTE: DO NOT rely on any fields other than: baseAddress, LoadAddress,
 *       filePath, fileName, or FileType.  Others are subject to change
 *       or removal.
 */
typedef struct Kt_fileCache_struct
{
  unsigned int  refCount;
  ULONG64      baseAddress;
  ULONG64      loadAddress;
  bool              flagDrv;
  bool              wow64addressset;
  char              filePath[Kt_FILEPATHBUFSIZE];
  char              fileName[Kt_BUFSIZE];
  char              fileType[Kt_BUFSIZE];
} Kt_fileCache_T;

typedef struct reqData_struct
{
  KT_RWMEMORY_IOCTL req;
  char              data[Kt_WRITE_BUFFER];
} reqData_T;

typedef struct ArgList_struct
{
    CHAR argName[Kt_MAXNAMELEN];
    CHAR argValue[Kt_MAXNAMELEN];

}ARG_LIST;




/*
 * global symbols
 */
extern int             Kt_bufSize;
extern Kt_fileCache_T *Kt_fileCache;
extern int             Kt_fileCacheSize;
extern bool            Kt_optionalBang;
extern bool            Kt_retRepeat;
extern bool            Kt_source;
extern HANDLE          Kt_hSource;
extern bool            Kt_writeEnabled;
extern HANDLE          Kt_hLogFile;
extern bool            Kt_logToCon;
extern bool            Kt_logToFile;
extern char            RemoteHost[128];

#define Kt_BUFSIZE_SYM       "Kt_bufSize"
#define Kt_FILECACHE_SYM     "Kt_fileCache"
#define Kt_FILECACHESIZE_SYM "Kt_fileCacheSize"

extern Kt_fileCache_T fileCache[Kt_FILECACHESIZE]; /* file info cache */


/*
 * standard elements of array macro
 */
#if !defined(elementsof)
#define elementsof(m_array) (sizeof (m_array) / sizeof (m_array[0]))
#endif

/*
defined for call to SymGetTypeInfo for
	 TI_FINDCHILDREN parameter passed to it.
	 Need the extra variable ChildId to account for the extra space in TI_FINDCHILDREN_PARAMS  structure
	 
*/
 typedef struct _fc_params 
 {  
 ULONG Count;
 ULONG Start;
 ULONG ChildId[0x1000];
} fc_params;

typedef struct _ktdumpSym_Ret
{
   ULONG64 address;
   ULONG    length;
   ULONG   offset;
} KTDUMPSYM_RET;


/*
 * Macros
 */
/* These macros are used for a fixed length array destination string
 * to ensure proper termination.  DO NOT USE if the destination
 * is just a pointer.
 */
#define KT_FIXED_LENGTH_DEST_STRNCPY( m_to, m_from ) \
{   m_to[sizeof(m_to) -1] = 0; \
  strncpy(m_to, m_from, sizeof(m_to) -1); }

#define KT_FIXED_LENGTH_DEST_STRNCAT( m_to, m_from) \
{   size_t string_len = MIN((sizeof(m_to) - strlen(m_to) -1), strlen(m_from));\
    strncat( m_to, m_from, string_len); \
    m_to[sizeof(m_to) -1] = 0; }



/*
 * function prototypes
 */
void            _writeFile              (char *const dataPA);
bool            getSourceLine_init      (const char *const fileNamePA);
bool            KtBangCmd               (const char *const cmdPA,
			                 const char *const argPA);
bool            KtDotCmd                (const char *const cmdPA,
                                         const char *const argPA);
bool			KtCheckandOpenFile		(void);
bool            KtDotLogClose           (void);
void			Kt_EnterCriticalSection(void);
void			Kt_LeaveCriticalSection(void);
VOID            Ktdprintf               (PCSTR lpFormat, ...);
ULONG64       KtGetExpression         (PCSTR lpExpression);
Kt_fileCache_T* KtGetFileCacheEntry     (char *const fileNamePA,
				         char *const fileTypePA,
				         char *const filePathPA);
Kt_errorCode_T  KtGetSymbolValue        (const char *const identPA,
				         ULONG64 *valuePA);
void CleanUpBeforeReload(void);
Kt_errorCode_T  KtGetTypeSize        (const char *const identPA,
				         ULONG *valuePA);
Kt_errorCode_T  KtDumpSymbolInfo     (void *pInputBuf,
				         KTDUMPSYM_RET *valuePA);

VOID
KtGetSymbolType (UINT argCount, ARG_LIST argList[ ]);

bool            KtGetSymbolValue_drivers (void);
bool            KtReadMemory            (ULONG64 offset,
				         PVOID lpBuffer,
				         ULONG   cb,
				         PULONG  lpcbBytesRead);
bool            KtStdCmd                (const char *const cmdPA,
					 const char *const argPA);
bool            KtWriteMemory           (ULONG64 offset,
				         LPCVOID lpBuffer,
				         ULONG   cb,
			                 PULONG  lpcbBytesWritten);
void            printUsage              (void);

#endif /* !defined(KTCONS_LOCAL_H) */
