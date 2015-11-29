//**********************************************************************
//.++
//  COMPANY CONFIDENTIAL:
//		Copyright (C) EMC Corporation, 2002
//		All rights reserved.
//		Licensed material - Property of EMC Corporation.
//.--                                                                    
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//		deltamaputils.h
//
// DESCRIPTION:
//		Functions for manipulation the deltamap.
//		 
//		
//
// REVISION HISTORY:
//      14-Mar-02  spathak	Created
//.--                                                                    
//**********************************************************************

#ifndef _DELTAMAP_UTILS_H
#define _DELTAMAP_UTILS_H



//
//  The maximum size of  a deltamap, in bits (256K). The number is somewhat
//  arbitrary, but it has an impact (together with the extent size represented
//  by a bit of the bitmap) on how large a disk region that the bitmap can
//  cover. At the current max bitmap size (256K bits) and picking a "small"
//  extent size of 64 Kb (likely to be the smallest extent allowed), a bitmap
//  of maximum size would account for a disk region of 8 GB.
//
//  This limit applies to the bits in the actual bitmap. The optional header
//  and trailer sizes would be additional and there is really no limits as to
//  how large these optional memory chunks can be.
//

#define DELTA_MAP_SIZE_MAX      (ULONG) (1024 * 256)

//
//  This represents an invalid bit number in a bitmap.
//

#define DELTA_MAP_INVALID_MAP_BIT   (ULONG) 0xffffffff

//
// Just for the heck of it, define something so we don't have
// 32 and 31 everywhere
//
#define DELTAMAP_BITS_PER_ULONG        (sizeof(ULONG) * 8)

// 
// A macro which will check the current IRQL level and call the 
// appropriate API to acquire the lock.
//

#define ACQUIRE_DELTAMAP_LOCK(mOldIrql, mLockPtr)  if ( EMCPAL_THIS_LEVEL_IS_DISPATCH(mOldIrql))\
    {                                                     \
        EmcpalSpinlockLockSpecial(mLockPtr);           \
    }                                                     \
    else                                                  \
    {                                                     \
        EmcpalSpinlockLock(mLockPtr);          \
    }                                                      
                                                   

// 
// A macro which will check the current IRQL level and call the 
// appropriate API to release the lock.
//

#define RELEASE_DELTAMAP_LOCK(mOldIrql, mLockPtr)  if (EMCPAL_THIS_LEVEL_IS_DISPATCH(mOldIrql)) \
    {                                                      \
        EmcpalSpinlockUnlockSpecial(mLockPtr);             \
    }                                                      \
    else                                                   \
    {                                                      \
        EmcpalSpinlockUnlock(mLockPtr);             \
    }                                                      
                                                    

//++
// Macro:
//      SIZEOF_DELTA_MAP_BUFFER
//
// Description:
//      Returns the size, in bytes of a Bitmap's buffer
//
// Arguments:
//      _PDeltamap_    the  Deltamap (DELTA_MAP)
//
// Return Value
//       size in bytes of the Bitmap's buffer
//
// Revision History:
//      03/18/2002   spathak    Created.
//
//--

#define SIZEOF_DELTA_MAP_BUFFER( _Pdeltamap_ )                   (  \
           ( ( _PDeltamap_ )->BufferSizeInWords * sizeof(ULONG) )   \
                                                                 )


//
//  Type:
//      DMH_DELTA_MAP
//
//  Description:
//      This structure represents a delta map resource.
//
//  Members:
//      BufferSizeInWords:
//          The size of the buffer, in words. This includes the bits of the
//          bitmap, plus the optional header and trailer.
//      BitmapSizeInWords:
//          Size in words of the bitmap bits themselves -- excluded header and
//          trailers. Yes, it can be computed, but kept here for convenience --
//          after all, memory is cheap, right?
//      BitsInBitmap:
//          The total size, in bits, of the bitmap. This size excludes optional
//          header and trailers!!!
//      BitmapBufferPtr:
//          The chunk of memory containing the actual bits of the bitmap plus
//          the optional header and trailers. This whole buffer is dumped to
//          disk IF this bitmap is kept in a persistent database.
//      BitsPtr:
//          The actual bits of the bitmap. It is a pointer within the
//          BitmapBufferPtr chunk of memory. A bitmap must have bits in it and
//          hence this field is never NULL in a properly initialized bitmap.
//      HeaderPtr:
//          The optional bitmap header. Headers are optional and this field
//          is NULL if a bitmap has no header. The contents of headers are left
//          entirely to the discretion of the bitmap user: we do no management
//          or locking of its contents. The only thing we do is dump the header
//          along w/ the bits and possible trailer if the bitmap is being
//          written to disk.
//      TrailerPtr:
//          The optional bitmap trailer. Trailers are optional and this field
//          is NULL if a bitmap has none. The contents of trailers are left
//          entirely to the discretion of the bitmap user: we do no management
//          or locking of its contents. The only thing we do is dump the trailer
//          along w/ the bits and possible header if the bitmap is being
//          written to disk.
//      ChunkSize:
//          The chunk size to use.
//      LockPtr:
//          Pointer to a the Executive spin lock which shall wiht the delta map

typedef struct _DMH_DELTA_MAP
{
    ULONG       BufferSizeInWords;
    ULONG       BitmapSizeInWords;
    ULONG       BitsInBitmap;
    PULONG      BitmapBufferPtr;
    PULONG      BitsPtr;
    PULONG      HeaderPtr;
    PULONG      TrailerPtr;
    ULONG       ChunkSize;
    PEMCPAL_SPINLOCK LockPtr;    
} DMH_DELTA_MAP, *PDMH_DELTA_MAP;

// A pointer to a function 

typedef VOID (*DMH_INITIALIZE_FUNC_PTR)(PDMH_DELTA_MAP);

extern EMCPAL_STATUS
DMH_DeltaMapInitialize (
                       IN  PDMH_DELTA_MAP               DeltaMapPtr,
                       IN  ULONG                        BitmapSizeInBits,
                       IN  ULONG                        HeaderSizeInWords,
                       IN  ULONG                        TrailerSizeInWords,
                       IN  PVOID                        BufferPtr,
                       IN  ULONG                        BufferSizeInWords,
                       IN  ULONG                        ChunkSize,
                       IN  DMH_INITIALIZE_FUNC_PTR      FuncPtrToInitializeBitmap,
                       IN  PEMCPAL_SPINLOCK                  LockPtr
                       );

extern EMCPAL_STATUS
DMH_DeltaMapCreate (
                   IN  PDMH_DELTA_MAP           DeltaMapPtr,
                   IN  ULONG                    BitmapSizeInBits,
                   IN  ULONG                    HeaderSizeInWords,
                   IN  ULONG                    TrailerSizeInWords,
                   IN  ULONG                    ChunkSize,
                   IN  DMH_INITIALIZE_FUNC_PTR  FuncPtrToInitializeBitmap,
                   IN  PEMCPAL_SPINLOCK              LockPtr
                   );

extern VOID
DMH_DeltaMapDestroy (
                    IN  PDMH_DELTA_MAP  DeltaMapPtr
                    );

extern BOOLEAN
DMH_IsBitSet (
             IN  PDMH_DELTA_MAP  DeltaMapPtr,
             IN  ULONG           BitNumber
             );

extern BOOLEAN
DMH_Lock_IsBitSet (
                  IN  PDMH_DELTA_MAP   DeltaMapPtr,
                  IN  ULONG            BitNumber
                  );

extern ULONG
DMH_GetBitsSetCount (
                    IN  PDMH_DELTA_MAP       DeltaMapPtr
                    );

extern ULONG
DMH_Lock_GetBitsSetCount (
                         IN  PDMH_DELTA_MAP       DeltaMapPtr
                         );

extern VOID
DMH_ClearAll (
             IN  PDMH_DELTA_MAP   DeltaMapPtr
             );

extern VOID
DMH_Lock_ClearAll (
                  IN  PDMH_DELTA_MAP    DeltaMapPtr
                  );

extern VOID
DMH_SetAll (
           IN  PDMH_DELTA_MAP   DeltaMapPtr
           );

extern VOID
DMH_Lock_SetAll (
                IN  PDMH_DELTA_MAP       DeltaMapPtr
                );

extern EMCPAL_STATUS
DMH_SetBit (
           IN  PDMH_DELTA_MAP      DeltaMapPtr,
           IN  ULONG               BitNumber
           );

extern EMCPAL_STATUS
DMH_Lock_SetBit (
                IN  PDMH_DELTA_MAP      DeltaMapPtr,
                IN  ULONG               BitNumber
                );

extern EMCPAL_STATUS
DMH_ClearBit (
             IN  PDMH_DELTA_MAP   DeltaMapPtr,
             IN  ULONG            BitNumber
             );

extern EMCPAL_STATUS
DMH_Lock_ClearBit (
                  IN  PDMH_DELTA_MAP      DeltaMapPtr,
                  IN  ULONG               BitNumber
                  );

extern EMCPAL_STATUS
DMH_Lock_CopyDeltaMap (
                      IN  PDMH_DELTA_MAP             pDestination,
                      IN  PDMH_DELTA_MAP             pSource
                      );
extern ULONG
DMH_GetNextBitSet (
                  IN  PDMH_DELTA_MAP       DeltaMapPtr,
                  IN  ULONG                   StartBit
                  );

extern ULONG
DMH_Lock_GetNextBitSet (
                       IN  PDMH_DELTA_MAP       DeltaMapPtr,
                       IN  ULONG                   StartBit
                       );




#endif //  _DELTAMAP_UTILS_H
