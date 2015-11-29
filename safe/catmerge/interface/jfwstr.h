#if !defined (JFWSTR_H)
#define JFWSTR_H
/*
 * CONDITIONALS
 *
 * SAVE_STR_BUF        -- if true, support allocating a single buffer
 *                        that can have its length reduced.
 * SAVE_STR_NO_DUP     -- if true, when passed an address in a pool
 *                        just return the address
 * SAVE_STR_LARGE      -- if true allocate a special buffer for large strings
 *                        if false, error on large strings
 * SAVE_STR_STATS      -- if true collect statistics
 * SAVE_STR_UNIQUE     -- if true return unique addresses only for
 *                        unique strings
 */
#if !defined(SAVE_STR_BUF)
#define SAVE_STR_BUF 0
#endif

#if !defined(SAVE_STR_POOL_SIZE)
#define SAVE_STR_POOL_SIZE 4096
#endif

#if !defined(SAVE_STR_NO_DUP)
#define SAVE_STR_NO_DUP 0
#endif

#if !defined(SAVE_STR_LARGE)
#define SAVE_STR_LARGE 0
#endif

#if !defined(SAVE_STR_UNIQUE)
#define SAVE_STR_UNIQUE 0
#endif

#if !defined(SAVE_STR_STATS)
#define SAVE_STR_STATS 1
#endif

#include "bool.h"
#include "cdecl.h"

typedef enum saveStrErrNum_enum
{
  saveStrErr_BadBufP,
  saveStrErr_BadBufSize,
  saveStrErr_NoMem,
  saveStrErr_NullIn,
  saveStrErr_NullOut,
  saveStrErr_TooBig,
  saveStrErr_TooSmall,
} saveStrErrNum_T;

/*
 * saveStrPool_T structure
 *
 *           +----------------+
 *   nextP   | saveStrPool_T* | next (previous pool) or NULL 
 *           +----------------+
 *   beginP  | char*          | first address of pool struct
 *           +----------------+
 *   endP    | char*          | Last address of pool structure
 *           +----------------+
 *   count   | int            | number of strings in this pool        [_STATS]
 *           +----------------+
 *   wasted  | int            | number of bytes wasted in closed pool [_STATS]
 *           +----------------+
 *
 *           +----------------+ <- saveStrGlobal.poolP
 *           |saveStrPool_T   | <- saveStrPool_T.nextP
 *           |                |
 *           +----------------+ <- saveStrPool_T.beginP
 *           |str#1           |
 *           +----------------+
 *           |str#2           |
 *           +----------------+
 *           :                :
 *           +----------------+[<- saveStrGlobal.bufP]
 *           |str#n           |
 *           +----------------+<- saveStrGlobal.freeP
 *           |                |
 *           |                |
 *           +----------------+ <- saveStrPool_T.endP
 */
typedef struct   saveStrPool_struct
{
  struct saveStrPool_struct *nextP;
  char           *beginP;
  char           *endP;
#if SAVE_STR_STATS
  int             count;
  int             wasted;
#endif
}
saveStrPool_T;

typedef char * (CDECL_ saveStrErrFunc_T (saveStrErrNum_T errNumA,
					 char *cPA,
					 size_t sizeA));

/*
 * saveStr_T structure
 *
 *          +-----------------+
 * poolP    |saveStrPool_T*   | active pool or NULL if no pools
 *          +-----------------+
 * freeP    |char*            | first free address in current pool
 *          +-----------------+
 * alloc    |size_t           | size to allocate to pool
 *          +-----------------+
 * errFunc  |saveStrErrFunc_T*| error reporting function
 *          +-----------------+
 * bufP     |char*            | address of active buffer              [_BUF]
 *          +-----------------+
 * bufSize  |size_t           | size of active buffer                 [_BUF]
 *          +-----------------+
 * poolCnt  |int              | number of pools allocated           [_STATS]
 *          +-----------------+
 * dupCnt   |int              | number of duplicate string avoided  [_STATS]
 *          +-----------------+
 */
typedef struct    saveStr_struct
{
  saveStrPool_T    *poolP;
  char             *freeP;
  size_t            alloc;
  saveStrErrFunc_T *errFunc;
#if SAVE_STR_BUF
  char             *bufP;
  size_t            bufSize;
#endif
#if SAVE_STR_STATS
  int               poolCnt;
  int               dupCnt;
#endif
}
saveStr_T;

#if !defined(stringify)
#define stringify(m_string) #m_string
#endif

/* *INDENT-OFF* */
#if defined(__cplusplus)
extern "C"
{
#endif
bool   CDECL_     cmpStr2abbrev       (const char *const strPA,
				       const char *const abbrevPA);
size_t CDECL_     dos2unix            (char *outStrPA,
				       const char *const inStrPA,
				       const size_t outLenA);
size_t CDECL_     dos2unix_1          (char *const strPA);
void   CDECL_     dotCtrl             (char *const outStrPA,
				       const char *const inStrPA,
				       const size_t inCntA);
void   CDECL_     guardStr            (char *const outStrPA,
				       const char *const inStrPA);
void   CDECL_     lowerStr            (char *const outStrPA,
				       const char *const inStrPA);
int    CDECL_     matchOnOff          (const char *const namePA);
short int CDECL_  parsePathNameType   (char* inStrPA, 
		                               char** pathPPA,
		                               char** namePPA,
		                               char** typePPA);
void   CDECL_     repChar             (char *const outStrPA,
				       const char  inCharA,
				       const size_t      countA);
char*  CDECL_     saveStr             (const char *const inPA);
char*  CDECL_     saveBuf             (const size_t bufSizeA);
char*  CDECL_     saveBufAdj          (const char *const inPA);
bool   CDECL_     saveStrInit         (const size_t sizeA);
void   CDECL_     saveStrDump         (void);
void   CDECL_     saveStrPoolDump     (const saveStrPool_T * const poolPA);
saveStrErrFunc_T* CDECL_  saveStrSetErrFunc   (saveStrErrFunc_T* errFuncPA);
char*  CDECL_     saveStrErrFunc_Fatal (const saveStrErrNum_T errNumA,
				       char *cPA,
				       size_t sizeA);
char*  CDECL_     saveStrErrFunc_NULL (const saveStrErrNum_T errNumA,
				       char *cPA,
				       size_t sizeA);
char*  CDECL_     saveStrErrFunc_Msg  (const saveStrErrNum_T errNumA,
				       char *cPA,
				       size_t sizeA);
char*  CDECL_     saveStrErrFunc_MsgDie (const saveStrErrNum_T errNumA,
				       char *cPA,
				       size_t sizeA);
bool   CDECL_     showCtrl            (      char *const outStrPA,
				       const char *const inStrPA,
				       const size_t      outLenA);
char*  CDECL_     skipStr             (const char *const strPA,
			               const char *const patternPA);
char*  CDECL_     skipWSP             (const char *const strPA);
char*  CDECL_     skip2WSP            (const char *const strPA);
#ifdef ALAMOSA_WINDOWS_ENV
char*  CDECL_     strtok_r            (char *      strPA,
				                       const char *      controlPA,
				                       char **     lastPA);
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - not needed off Windows */
void   CDECL_     trimWSP             (char *const strPA);
size_t CDECL_     unix2dos            (char *outStrPA,
				       const char *const inStrPA,
				       const size_t outLenA);
size_t CDECL_     unix2dos_1          (char *const strPA,
				       const size_t outLenA);
void   CDECL_     upperStr            (char *const outStrPA,
				       const char *const inStrPA);
char*  CDECL_     vstrcat             (char *destPA,
				       /* const char* src1PA,*/
				       ...
				       /*, NULL */);
char*  CDECL_     vstrncat            (char *destPA,
				       size_t countA,
				       /* const char* src1PA, */
				       ...
				       /*, NULL */);
char*  CDECL_     vstrcpycat          (char *destPA,
				       /* const char* src1PA,*/
				       ...
				       /*, NULL */);
char*  CDECL_     vstrncpycat         (char *destPA,
				       size_t countA,
				       /* const char* src1PA, */
				       ...
				       /*, NULL */);
void   CDECL_     zapEOL              (char *lineBufPA);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/* *INDENT-ON* */
#endif	/* !defined(JFWST_H) */	/* end of file jfwStr.h */
