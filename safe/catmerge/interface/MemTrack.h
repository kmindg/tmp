//++
//
//  Module:
//
//      Memtrack.h
//
//  Description:
//
//      Functions for tracking pool usage in a driver and detection
//      some kinds of buffer over- and under-runs and enforcing memory limits.
//
//  Notes:
//
//      By Mark Russinovich
//      http://www.sysinternals.com
//
//  History:
//
//      14-Mar-00   PTM;    Updated file for enforcing memory limits for NPP allocations, 
//                          also reformatted file.
//
//--

#ifndef MEMTRACK_H
#define MEMTRACK_H

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C++.
 */
extern "C" {
#endif

PVOID
MemAllocatePoolWithTag( IN EMCPAL_POOL_TYPE                            PoolType,
                        IN SIZE_T                               NumberOfBytes,
                        IN ULONG                                Tag );


PVOID
MemAllocatePool( IN EMCPAL_POOL_TYPE                                   PoolType,
                 IN SIZE_T                                      NumberOfBytes );


VOID
MemCheckForCorruption( PVOID                                    pBuffer );


VOID
MemFreePool( PVOID                                              pBuffer );


VOID
MemTrackPrintStats( PCHAR                                       pText);


EMCPAL_STATUS
MemMaxAllowed( SIZE_T                                            MaxMemoryAllowed );


EMCPAL_STATUS
MemMaxUnused( SIZE_T *                                           pNumberOfBytes );


EMCPAL_STATUS
MemIncrementCurrentUsage( SIZE_T                                 NumberOfBytes );


EMCPAL_STATUS
MemDecrementCurrentUsage( SIZE_T                                 NumberOfBytes );


//
//  The following are clients of MemTrack and the parameters they use
//  to initialize MemTrack.
//
//      MPS:        '1spM', 0xa0, 0xa1, 0xf0
//      RMD:        '1dmR', 0xc0, 0xc1, 0xf2
//      DLS:        'MslD', 0xd0, 0xd1, 0xf3
//      DlsDriver:  'DslD', 0xe0, 0xe1, 0xf4
//      SnapCopy:   'rDcS', 0xb0, 0xb1, 0xf1
//

VOID
MemTrackInit( IN ULONG                                          Tag,
              IN UCHAR                                          Allocate,
              IN UCHAR                                          AllocateWithTag,
              IN UCHAR                                          Free );

VOID
MemTrackUnInit(void);

#ifdef __cplusplus
};
#endif

#endif // MEMTRACK_H

