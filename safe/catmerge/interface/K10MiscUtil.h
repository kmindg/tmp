//***************************************************************************
// Copyright (C) Data General Corporation 1989-2000
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

#ifndef _K10_MISC_UTIL_H_
#define _K10_MISC_UTIL_H_
//++
// File Name:
//      K10MiscUtil.h
//
// Contents:
//      prototypes for K10 Miscellaneous Utilities
//
// Exported:
//
// Revision History:
//      6/4/1999    MWagner   Created
//      6/4/2010    SYackey   Added K10MiscUtilMemoryWarehousePALInterlockedRequisitionPallet()
//      2/10/2011   AGottstein Added K10_MISC_UTIL_MONOSTABLE_TIMER support, other PAL pallet routines
//
//--

// Include Files
//--

#include "k10ntddk.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_String.h"
#include "EmcPAL_Inline.h"

#include "K10MiscUtilMessages.h"
#include "ListsOfIRPs.h"
#include "EmcPAL_List.h"

CSX_CDECLS_BEGIN


//++
// End Includes
//--

//++
// Forward Declarations
//--
struct _K10_MISC_UTIL_LH_ANCHOR;

//++
// Macro:
//      K10_TICKS_PER_XXX
//
// Description:
//      Simple macros to convert NT 100 nanosecond ticks to
//      microseconds, milliseconds, seconds, and minutes.
//
// Revision History:
//      3/16/2000   MWagner    Created.
//
//--

#define K10_TICKS_PER_MICROSECOND                                    (10)
#define K10_TICKS_PER_MILLISECOND    (1000 * (K10_TICKS_PER_MICROSECOND))
#define K10_TICKS_PER_SECOND         (1000 * (K10_TICKS_PER_MILLISECOND))
#define K10_TICKS_PER_MINUTE         ( 60  *      (K10_TICKS_PER_SECOND))


// .End K10_TICKS_PER_XXX


//++
// Macro:
//
//      K10_MISC_UTIL_WALK_LIST
//
// Description:
//      NTDDK.H supplies all sorts of list macros, but not one to walk a list, so
//      we will just supply our own.
//
//
// Arguments:
//      PLIST_ENTRY Current
//      PLIST_ENTRY Head
//
// Return Value
//      None.
//
// Revision History:
//      5/9/00   MWagner    I stole this from Paul McGrath, but I am sure that there
//                          are lots of other definitions floating around...
//
#define K10_MISC_UTIL_WALK_LIST( _Current_, _ListHead_ )               \
    for ( ( _Current_ )  = ( _ListHead_ ) -> Flink;                    \
          ( _Current_ ) != ( _ListHead_ );                             \
          ( _Current_ )  = ( _Current_ ) -> Flink )

// .End K10_MISC_UTIL_WALK_LIST

//++
// Macro:
//      K10_MISC_UTIL_ROUND_UP
//
// Description:
//      Rounds a quantity up to a granularity
//
// Arguments:
//      _Quantity_
//      _Granularity_
//
// Return Value
//      _Quantity_ rounded up to a _Granularity_ boundary
//
// Revision History:
//      11/23/09  MWagner    Created.
//
//--
#define K10_MISC_UTIL_ROUND_UP(_Quantity_, _Granularity_)                       \
            ((((_Quantity_) + ((_Granularity_) - 1)) / (_Granularity_)) * (_Granularity_))
//++
// Macro:
//      K10_MISC_UTIL_ROUND_DOWN
//
// Description:
//      Rounds a quantity down to a granularity
//
// Arguments:
//      _Quantity_
//      _Granularity_
//
// Return Value
//      _Quantity_ rounded down to a _Granularity_ boundary
//
// Revision History:
//      11/23/09  MWagner    Created.
//
//--
#define K10_MISC_UTIL_ROUND_DOWN(_Quantity_, _Granularity_)                    \
            (((_Quantity_) / (_Granularity_)) * (_Granularity_))

//++
// Macro:
//      K10_MISC_UTIL_GET_TRACE_FLAG
//
// Description:
//      Reads "TraceFlag" from a Driver's Parameters Area
//
// Arguments:
//
// Return Value
//      STATUS_SUCCESS:  the Trace Flag was read
//      Others       :   bubbled up from K10MiscUtilRegistryClerkReadDWORD()
//
// Revision History:
//      6/20/00   MWagner    Created.
//
//--
#define K10_MISC_UTIL_GET_TRACE_FLAG( _PRegistryPath_, _PValue_)    (\
            K10MiscUtilRegistryClerkReadDWORD(                       \
                _PRegistryPath_,                                     \
                 "TraceFlag",                                \
                _PValue_)                                            \
                                                                     )
// .End K10_MISC_UTIL_GET_TRACE_FLAG

//++
// Function:
//                  K10CreateDirectoryObject
//
// Description:     This routine creates a directory in the K10 device
//                  directory hierarchy.
//
// IRQL:            IRQL == PASSIVE_LEVEL
//
// Parameters:      pDirectoryPath - A pointer to the character string of the
//                      directory to create.
//                  pHandle - A pointer to a handle that will be filled in
//                      with a directory handle.
//
// Return Values:   STATUS_SUCCESS if the directory is created;
//                  an error code from a routine.
//
// History:         26-Apr-98       ALT     Created
//                  04-Jun-99       MWagner Stolen from ATaylor and moved
//                                          to K10MiscUtil
//--


EMCPAL_STATUS
K10CreateDirectoryObject( IN PCHAR                      pDirectoryPath,
                          IN PHANDLE                    pHandle );

//++
// Function:
//   K10CloseDirectory
//
// Description:     This routine close a device directory object.
//
// Parameters:      Handle - Handle to a device directory object.
//
// Return Values:   STATUS_SUCCESS if the directory is closed.
//
// History:         13-Aug-2012       renr2     Created
//
//--

EMCPAL_STATUS
K10CloseDirectory( IN HANDLE                    Handle );

//++
// Function:
//      K10MiscUtilOpenParameters
//
// Description:
//      K10MiscUtilOpenParameters() opens the Driver's "Parameters" Area
//
// Arguments:
//      pRegistryPath    the NT supplied registry path
//      pHandle          the Parameters handle, if it was opened
//
// Return Value:
//      STATUS_SUCCESS:  the  "Parameters" Area was opened
//      Others:          return from one of the RtlXXX() or ZwXxx() functions
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilOpenParameters(
                         IN OUT PHANDLE         pHandle,
                         IN csx_pchar_t         pRegistryPath,
                         IN ACCESS_MASK         accessMask
                         );


//++
// Function:
//      K10MiscUtilCloseParameters
//
// Description:
//      K10MiscUtilCloseParameters() closes the Driver's "Parameters" Area
//
// Arguments:
//      hParameters  handle from K10MiscUtilOpenParameters()
//
// Return Value:
//      STATUS_SUCCESS:   the "Parameters" Area was Close
//      Others:           error from ZwClose();
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilCloseParameters(
                          HANDLE  hParameters
                          );


//++
// Function:
//      K10MiscUtilFlushParameters
//
// Description:
//      K10MiscUtilFlushParametersKey() flushes the "Parameters" Area
//
// Arguments:
//      hParameters  handle from K10MiscUtilOpenParameters()
//
// Return Value:
//      STATUS_SUCCESS:   the "Parameters" Area was flushed
//      Others:           error from ZwFlush();
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilFlushParameters(
                          HANDLE  hParameters
                          );

//++
// Function:
//      K10MiscUtilRegistryClerkReadDWORD()
//
// Description:
//      K10MiscUtilRegistryClerkReadDWORD()  reads a DWORD from the Registry
//
// Arguments:
//      pRegistryPath    the NT supplied registry path
//      wstrValueName    Value Name
//      pValue           Value
//
// Return Value:
//      STATUS_SUCCESS:  the  Value was read and returned
//      Others:          return from one of the ZwXxx() functions
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilRegistryClerkReadDWORD(
                                 csx_pchar_t           pRegistryPath,
                                 csx_pchar_t           strValueName,
                                 PULONG                pValue
                                 );


//++
// Function:
//      K10MiscUtilRegistryClerkWriteDWORD
//
// Description:
//      K10MiscUtilRegistryClerkWriteDWORD()  writes a DWORD to the Registry
//
// Arguments:
//      pRegistryPath    the NT supplied registry path
//      wstrValueName      the PWSTR Value Name
//      ulValue          the Value to be written
//
// Return Value:
//      STATUS_SUCCESS:  the  DWORD was read
//      Others:          return from one of the ZwXxx() functions
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilRegistryClerkWriteDWORD(
                                  csx_pchar_t           pRegistryPath,
                                  csx_pchar_t           strValueName,
                                  ULONG                 ulValue
                                  );

//++
// Function:
//      K10MiscUtilRegistryClerkReadString()
//
// Description:
//      Reads a string from the Registry.
//
// Arguments:
//      pRegistryPath    the NT supplied registry path
//      wstrValueName    Value Name
//      pValue           Value
//      BufferSize       Size of buffer pointed to by Value
//
// Return Value:
//      STATUS_SUCCESS:  the  Value was read and returned
//      Others:          return from one of the ZwXxx() functions
//
// Revision History:
//      3/17/09   JTaylor    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilRegistryClerkReadString(
                                 csx_pchar_t           pRegistryPath,
                                 csx_pchar_t           strValueName,
                                 csx_pchar_t          pValue,
                                 ULONG           BufferSize
                                 );

//++
// Function:
//
//      K10MiscUtilRegistryQueryFeatureLimit
//
// Description:
//
//      Retrieve the value of a given version of a feature limit from the Registry.
//
// Arguments:
//
//      ProdName:   The name of the product for which a feature limit
//                  parameter value is being requested.
//
//      TierName:   The name of the package tier for which the feature limit
//                  parameter value is being requested.  Use "Default" for packages
//                  that have no tiers.
//
//      pLevel:     On input, contains the Compatibility Level of the feature limit
//                  parameter whose value is being requested. On output, contains the
//                  highest-numbered level that was actually found in the table but
//                  which does not exceed the input level.
//
//      ParamName:  The name of the feature limit parameter whose value is being requested.
//
//      pParamVal:  Pointer to a caller-allocated buffer in which the parameter value
//                  retrieved from the Registry will be stored.
//
//      pLength:    On input, points to the size in bytes of the pParamVal buffer.
//                  On output, points to the number of bytes in that buffer that were
//                  actually used to store the requested value.
//
// Return Value:
//
//      STATUS_SUCCESS:  The value was retrieved successfully.
//
//      STATUS_OBJECT_NAME_NOT_FOUND:  The specified parameter was not found in the Registry.
//
//      STATUS_BUFFER_TOO_SMALL:  The caller-supplied buffer was not large enough to hold
//      the value of the specified parameter.
//
// Revision History:
//
//      02-Nov-04   Goudreau    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilRegistryQueryFeatureLimit(
    IN PCHAR  ProdName,
    IN PCHAR  TierName,
    IN OUT PULONG  pLevel,
    IN PCHAR  ParamName,
    OUT PVOID  pParamVal,
    IN OUT PULONG  pLength
    );

//--
//
//  Wide character version of K10MiscUtilRegistryQueryFeatureLimit().
//  See above description for details.
//
//--

EMCPAL_STATUS 
K10MiscUtilRegistryQueryFeatureLimitW(
    IN csx_pchar_t  ProdName,
    IN csx_pchar_t  TierName,
    IN OUT PULONG   pLevel,
    IN csx_pchar_t  ParamName,
    OUT PVOID       pParamVal,
    IN OUT PULONG   pLength 
    );


//++
// Function:
//      K10MiscUtilGetMaxNumImagesFromRegistry()
//
// Description:
//      K10MiscUtilGetMaxNumImagesFromRegistry()  reads a DWORD from the Registry
//      according to the RegistryPath specified
//
// Arguments:
//      pValue           Value
//
// Return Value:
//      STATUS_SUCCESS:  the  Value was read and returned
//      Others:          return from one of the ZwXxx() functions
//
// Revision History:
//      6/21/00   BXU    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilGetMaxNumImagesFromRegistry(
                                      PULONG          pValue
                                      );

//++
//     K10 Misc Util Bitmap
//
//     This implements a very simple Bitmap utility.
//
//--

//++
// Macro:
//      K10_MISC_UTIL_BITS_PER_UCHAR
//
// Description:
//      Oddly enough, there's no constant I can find that defines the size of a byte
//      in Bits.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      3/13/01   MWagner    Created.
//
//--
#define K10_MISC_UTIL_BITS_PER_UCHAR           0x00000008





// .End K10_MISC_UTIL_BITS_PER_UCHAR

//++
// Macro:
//      K10_MISC_UTIL_ROUND_TO_ULONGS
//
// Description:
//      Rounds a size in bytes up to the nearest ULONG boundary.
//      (Code stolen from MS ROUND_TO_PAGES()
//
// Arguments:
//      _SizeInBytes_
//
// Return Value
//      _SizeInBytes_ rounded up to ULONG boundary.
//
// Revision History:
//      04/10/01   MWagner    Created.
//
//--
#define K10_MISC_UTIL_ROUND_TO_ULONGS(_SizeInBytes_)                           \
            (((ULONG)(_SizeInBytes_) + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1))



//++
// Macro:
//      K10_MISC_UTIL_ROUND_TO_LARGE_INTEGER
//
// Description:
//      Rounds a size in bytes up to the nearest 64 bits boundary.
//      (Code stolen from MS ROUND_TO_PAGES()
//
// Arguments:
//      _SizeInBytes_
//
// Return Value
//      _SizeInBytes_ rounded up to LARGE_INTEGER boundary.
//
// Revision History:
//      04/10/01   MWagner    Created.
//
//--
#define K10_MISC_UTIL_ROUND_TO_LARGE_INTEGERS(_SizeInBytes_)                           \
            (((ULONG)(_SizeInBytes_) + sizeof(EMCPAL_LARGE_INTEGER) - 1) & ~(sizeof(EMCPAL_LARGE_INTEGER) - 1))


// .End K10_MISC_UTIL_ROUND_TO_LARGE_INTEGERS

//++
// Macro:
//      K10_MISC_UTIL_BITMAP_INVALID_BIT_INDEX
//
// Description:
//      Used to signal "no bit" in the FindNext functions. This limits the Bitmap
//      to 4.2 Gigabits, which is something we'll have to live with.
//
//
// Return Value
//      None.
//
// Revision History:
//      3/13/01   MWagner    Created.
//
//--
#define K10_MISC_UTIL_BITMAP_INVALID_BIT_INDEX        MAXULONG




// .End K10_MISC_UTIL_BITMAP_INVALID_BIT_INDEX

//++
// Type:
//      K10_MISC_UTIL_BITMAP
//
// Description:
//      simple Bitmap
//		The K10MiscUtil.proto nanopb definition ( generated file under 
//		mf_common/components/be_protocol_def/ ) may need an update 
//		whenever this struct is changed.
//
// Members:
//      NumberOfBits - number of Bits in the Bitmap
//      BitsSet      - the number of Bits set in the Bitmap
//      Bits         - an array of UCHARS than can be addresses by bit address
//
// Revision History:
//      3/13/01   MWagner    Created.
//
//--
typedef struct _K10_MISC_UTIL_BITMAP
{
    ULONG NumberOfBits;
    ULONG BitsSet;
    UCHAR Bits[1];
} K10_MISC_UTIL_BITMAP, *PK10_MISC_UTIL_BITMAP;
// .End K10_MISC_UTIL_BITMAP


// .End K10_MISC_UTIL_BITMAP_CLEAR


//++
// Bitmap Macro Interface
//
// The macro Bitmap interface does not do any range checking or tracing, and does not
// offer any compound or interlocked functions. It is intended to be fast and
// relatively lightweight.
//
// The functional interface is much richer, but also slower.
//
//--

// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_BITS_SET
//
// Description:
//      This macro returns the number of bits set in the Bitmap (if the
//      header was initialized correctly).
//      BYTE units.
//
// Arguments:
//      _Bitmap_
//
// Return Value:
//      XXX:    number of bits set in the Bitmap
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_BITS_SET( _Bitmap_) ((_Bitmap_)->BitsSet)





// .End K10_MISC_UTIL_BITMAP_BITS_SET


// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_SIZEOF_BITS
//
// Description:
//      This macro calculates the size of the Bits in the Bitmap, in
//      bytes units.
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Bits in the Bitmap in bytes
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_SIZEOF_BITS( _NumberOfBits_ )           \
                                                                     \
         ((( _NumberOfBits_ ) + (K10_MISC_UTIL_BITS_PER_UCHAR-1))    \
                                / (K10_MISC_UTIL_BITS_PER_UCHAR))    \
                                                                     \





// .End K10_MISC_UTIL_BITMAP_SIZEOF_BITS


// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_SIZEOF_BITMAP
//
// Description:
//      This macro calculates the size of a _Bitmap_ (including the header)
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Bitmap in bytes, including the header
//
// Revision History:
//      03/13/01   MWagner    Created.
//
//      08/09/02   MWager     old macro was always returning 4 bytes too many,
//                            since sizeof(K10_MISC_UTIL_BITMAP) was returning
//                            12, not nine. Changed to directly figure out the
//                            sizeof NumberOfBits and BitsSet
//--
#define K10_MISC_UTIL_BITMAP_SIZEOF_BITMAP( _NumberOfBits_)         (\
                                                                     \
         sizeof(ULONG)   /* NumberOfBits */            +             \
         sizeof(ULONG)  /* BitsSet      */             +             \
         K10_MISC_UTIL_BITMAP_SIZEOF_BITS( (_NumberOfBits_) )        \
                                                                     )



// .End K10_MISC_UTIL_SIZEOF_BITMAP

// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_8BYTES_ALIGNED_SIZEOF_BITMAP
//
// Description:
//      This macro calculates the size of a _Bitmap_ (including the header)
//      aligned to 64 bits boundaries.
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Bitmap in bytes, including the header
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_8BYTES_ALIGNED_SIZEOF_BITMAP( _NumberOfBits_ )                              (\
            K10_MISC_UTIL_ROUND_TO_LARGE_INTEGERS(K10_MISC_UTIL_BITMAP_SIZEOF_BITMAP(( _NumberOfBits_ )))  \
                                                                                                   )
// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_4BYTES_ALIGNED_SIZEOF_BITMAP
//
// Description:
//      This macro calculates the size of a _Bitmap_ (including the header)
//      aligned to ULONG boundaries.
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Bitmap in bytes, including the header
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_4BYTES_ALIGNED_SIZEOF_BITMAP( _NumberOfBits_ )                              (\
            K10_MISC_UTIL_ROUND_TO_ULONGS(K10_MISC_UTIL_BITMAP_SIZEOF_BITMAP(( _NumberOfBits_ )))  \
            )



// .End K10_MISC_UTIL_BITMAP_4BYTES_ALIGNED_SIZEOF_BITMAP


// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_INIT_HEADER
//
// Description:
//      This macro initializes the _Bitmap_ Header. This macro does not zero out the
//      Bits themselves.
//
// Arguments:
//      _BitMap_
//      _NumberOfBits_
//
// Return Value:
//      None.
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_INIT_HEADER( _BitMap_, _NumberOfBits_)  \
     do {                                                            \
         ( _BitMap_ )->NumberOfBits = ( _NumberOfBits_ );            \
         ( _BitMap_ )->BitsSet      = 0;                             \
     } while (0)                                                     \
// .End K10_MISC_UTIL_BITMAP_INIT_HEADER







// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_TEST
//
// Description:
//      This macro tests a Bit
//
// Arguments:
//      _BitMap_
//      _BitIndex_
//
// Return Value:
//      TRUE  : the Bit is set
//      FALSE : the Bit is not set
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_TEST( _Bitmap_, _BitIndex_ )                           (\
             ( _Bitmap_ )->Bits [ ( _BitIndex_ ) / K10_MISC_UTIL_BITS_PER_UCHAR ]    \
              & ( 1 << ( ( _BitIndex_ ) % K10_MISC_UTIL_BITS_PER_UCHAR ) )           \
                                                                                 )


// .End K10_MISC_UTIL_BITMAP_TEST




// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_SET
//
// Description:
//      This macro sets a Bit that is unset. It does no range checking.
//
// Arguments:
//      _BitMap_
//      _BitIndex_
//
// Return Value:
//      None.
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_SET( _Bitmap_, _BitIndex_ )                                   \
          do {                                                                             \
              if ( !(K10_MISC_UTIL_BITMAP_TEST( _Bitmap_, _BitIndex_ )))                   \
              {                                                                            \
                  ( _Bitmap_ )->Bits[ ( _BitIndex_ ) / K10_MISC_UTIL_BITS_PER_UCHAR ]      \
                      |= ( 1 << ( ( _BitIndex_ ) % K10_MISC_UTIL_BITS_PER_UCHAR ) );       \
                  ( _Bitmap_ )->BitsSet++;                                                 \
              }                                                                            \
          } while (0)                                                                      \






// .End K10_MISC_UTIL_BITMAP_CLEAR

// ++
// Macro:
//      K10_MISC_UTIL_BITMAP_CLEAR
//
// Description:
//      This macro clears a Bit that is set.
//
// Arguments:
//      _BitMap_
//      _BitIndex_
//
// Return Value:
//      None.
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BITMAP_CLEAR( _Bitmap_, _BitIndex_ )                               \
        do {                                                                             \
            if ( K10_MISC_UTIL_BITMAP_TEST( _Bitmap_, _BitIndex_ ))                      \
            {                                                                            \
                ( _Bitmap_ )->Bits[ ( _BitIndex_ ) / K10_MISC_UTIL_BITS_PER_UCHAR ]      \
                    &= ~( 1 << ( ( _BitIndex_ ) % K10_MISC_UTIL_BITS_PER_UCHAR ) );      \
                ( _Bitmap_ )->BitsSet--;                                                 \
            }                                                                            \
        } while (0)                                                                      \





// .End K10_MISC_UTIL_BITMAP_CLEAR


//++
// Bitmap Functions
//
// See k10miscutilbitmap.c for descriptions of these functions
//--
EMCPAL_STATUS
K10MiscUtilBitmapInitialize(
                           PK10_MISC_UTIL_BITMAP PBitmap,
                           ULONG                 NumberOfBits
                           );

EMCPAL_STATUS
K10MiscUtilBitmapTestBit(
                        PK10_MISC_UTIL_BITMAP PBitmap,
                        ULONG_PTR             Bit,
                        PBOOLEAN              PBitIsSet
                        );
EMCPAL_STATUS
K10MiscUtilBitmapSetBit(
                       PK10_MISC_UTIL_BITMAP PBitmap,
                       ULONG                 Bit
                       );
EMCPAL_STATUS
K10MiscUtilBitmapClearBit(
                         PK10_MISC_UTIL_BITMAP PBitmap,
                         ULONG                 Bit
                         );
EMCPAL_STATUS
K10MiscUtilBitmapFindNextClearBitIndex(
                                      PK10_MISC_UTIL_BITMAP PBitmap,
                                      ULONG                 StartBit,
                                      PULONG                PNextClearBit
                                      );
EMCPAL_STATUS
K10MiscUtilBitmapFindNextSetBitIndex(
                                    PK10_MISC_UTIL_BITMAP PBitmap,
                                    ULONG                 StartBit,
                                    PULONG                PNextSetBitIndex
                                    );

//++
// The "interlocked Bitmap functions
//--
EMCPAL_STATUS
K10MiscUtilBitmapInterlockedTestBit(
                                   PK10_MISC_UTIL_BITMAP PBitmap,
                                   ULONG                 Bit,
                                   PBOOLEAN              PBitIsSet,
                                   PEMCPAL_SPINLOCK           PLock
                                   );
EMCPAL_STATUS
K10MiscUtilBitmapInterlockedSetBit(
                                  PK10_MISC_UTIL_BITMAP PBitmap,
                                  ULONG                 Bit,
                                  PEMCPAL_SPINLOCK           PLock
                                  );

EMCPAL_STATUS
K10MiscUtilBitmapInterlockedClearBit(
                                    PK10_MISC_UTIL_BITMAP PBitmap,
                                    ULONG                 Bit,
                                    PEMCPAL_SPINLOCK           PLock
                                    );

EMCPAL_STATUS
K10MiscUtilBitmapInterlockedTestAndSetBit(
                                         PK10_MISC_UTIL_BITMAP PBitmap,
                                         ULONG                 Bit,
                                         PEMCPAL_SPINLOCK           PLock,
                                         PBOOLEAN              PBitWasAlreadySet
                                         );
EMCPAL_STATUS
K10MiscUtilBitmapInterlockedFindNextClearBitIndex(
                                                 PK10_MISC_UTIL_BITMAP PBitmap,
                                                 ULONG                 StartBit,
                                                 PULONG                PNextClearBit,
                                                 PEMCPAL_SPINLOCK           PLock
                                                 );
EMCPAL_STATUS
K10MiscUtilBitmapInterlockedFindNextSetBitIndex(
                                               PK10_MISC_UTIL_BITMAP PBitmap,
                                               ULONG                 StartBit,
                                               PULONG                PNextSetBitIndex,
                                               PEMCPAL_SPINLOCK           PLock
                                               );

EMCPAL_STATUS
K10MiscUtilBitmapInterlockedFindAndSetNextClearBit(
                                                  PK10_MISC_UTIL_BITMAP PBitmap,
                                                  ULONG                 StartBit,
                                                  PULONG                PNextClearBit,
                                                  PEMCPAL_SPINLOCK           PLock
                                                  );

UINT_32
K10MiscUtilGetMSBWithLimit(
    UINT_64     value,
    UINT_32     limit
    );

//++
// Misc Util Memory Sheriff
//    The Sheriff locks and unlocks PagedPool Memory
//--

//++
// Enum:
//      K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT_PHASE
//
// Members:
//
//      K10_MISC_UTIL_WARRANT_PHASE_INVALID
//      K10_MISC_UTIL_WARRANT_PHASE_INCHOATE       the Warrant has been Initialized
//      K10_MISC_UTIL_WARRANT_PHASE_ALLOCATED      the PagedPool Memory has been Allocated
//      K10_MISC_UTIL_WARRANT_PHASE_MDL_ALLOCATED  the MDL has been allocated
//      K10_MISC_UTIL_WARRANT_PHASE_LOCKED         the PagedPool Memory has been Locked
//      K10_MISC_UTIL_WARRANT_PHASE_MAPPED         the PagedPool Memory has been Mapped
//
// Revision History:
//      3/29/01   MWagner    Created.
//
//--
typedef enum _K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT_PHASE
{
    K10_MISC_UTIL_WARRANT_PHASE_INVALID  = -1,
    K10_MISC_UTIL_WARRANT_PHASE_INCHOATE,
    K10_MISC_UTIL_WARRANT_PHASE_ALLOCATED,
    K10_MISC_UTIL_WARRANT_PHASE_MDL_ALLOCATED,
    K10_MISC_UTIL_WARRANT_PHASE_LOCKED,
    K10_MISC_UTIL_WARRANT_PHASE_MAPPED,
    K10_MISC_UTIL_WARRANT_PHASE_MAX

} K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT_PHASE, *PK10_MISC_UTIL_MEMORY_SHERIFF_WARRANT_PHASE;
//.End

//++
// Type:
//      K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT
//
// Description:
//      An Opaque data structure used by the Sheriff
//
// Members:
//      PMdl                  MDL for probing and locking Paged Pool allocation
//      PagedPoolAllocation   the memory allocated from Paged Pool
//      MappedAddress         the address of the Mapped Memory
//      UlNumberOfBytes       size of allocation
//
// Revision History:
//      3/29/01   MWagner    Created.
//
//--
typedef struct _K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT
{
    PEMCPAL_MDL                                 PMdl;
    // PVOID                                      PagedPoolAllocation;
    EMCPAL_IOMEMORY_OBJECT                      PagedPoolAllocation;
    PVOID                                       MappedAddress;
    ULONG                                       UlNumberOfBytes;
    K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT_PHASE Phase;
} K10_MISC_UTIL_MEMORY_SHERIFF_WARRANT, *PK10_MISC_UTIL_MEMORY_SHERIFF_WARRANT;
//.End

EMCPAL_STATUS
K10MiscUtilMemorySheriffAllocateAndLockPagedMemory(
                                                  PK10_MISC_UTIL_MEMORY_SHERIFF_WARRANT  PWarrant,
                                                  ULONG                                  UlNumberOfBytes
                                                  );
PVOID
K10MiscUtilMemorySheriffGetAddressOfLockedPagedMemory(
                                                     PK10_MISC_UTIL_MEMORY_SHERIFF_WARRANT PWarrant
                                                     );
VOID
K10MiscUtilMemorySheriffUnlockAndFreePagedMemory(
                                                PK10_MISC_UTIL_MEMORY_SHERIFF_WARRANT PWarrant
                                                );


//++
// Misc Util Memory Warehouse
//   This utility implements a "memory warehouse" of fixed size "pallets". The
//   name "pool" and "buffer" were avoided since a memory "pool" is already a
//   dissimilar NT construct, and does not imply fixed size allocations.
//--

//++
// Type:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE
//
// Description:
//      A Misc Util Warehouse is a managed collection of fixed size Pallets.
//      The Warehouse does no allocation or de-allocation, it only manages the
//      Pallets. The functions are called Requisition and Return to avoid any confusion
//      with NT or Memtrack Allocation calls. A client requisitions a Pallet when needed,
//      and returns it when done.
//
//      At initialization, the first word of every Pallet is stamped with the address
//      of that Pallet. When a Pallet is returned, it's also stamped with its own address.
//      When a Pallet is Requisitioned, the first word is inspected. If that is not equal
//      to the address of the Pallet, the Memory Warehouse BugChecks. That BugCheck indicated
//      that a Pallet has been written to while it is still in the Warehouse. In addition, if
//      a developer finds that the first word of a structure has been overwritten with
//      the address of that structure, then the structure has been released.
//
// Members:
//     PBitmap            bitmap to manage allocation
//     UlSizeOfPallet
//     UlNextSearchStart  don't start searching at the beginning of the Warehouse
//                        for each allocation- start at the bit after the last
//                        successful allocation, or at the last freed Pallet
//     UlHighWaterMark    largest number of Pallets in use
//     Pallets            client allocated memory to be managed
//
// Revision History:
//      3/27/01   MWagner    Created.
//
//--
typedef struct _K10_MISC_UTIL_MEMORY_WAREHOUSE
{
    PK10_MISC_UTIL_BITMAP    PBitmap;
    ULONG                    UlSizeOfPallet;
    ULONG                    UlNextSearchStart;
    ULONG                    UlHighWaterMark;
    PCHAR                    Pallets;
} K10_MISC_UTIL_MEMORY_WAREHOUSE, *PK10_MISC_UTIL_MEMORY_WAREHOUSE;
//.End

// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS
//
// Description:
//      Returns the Number of Pallets in use in the Warehouse
//
// Arguments:
//      _PMemoryWarehouse_
//
// Return Value:
//      Number of Pallets in use in the Warehouse
//
// Revision History:
//      04/19/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_NUMBER_OF_PALLETS( _PMemoryWarehouse_ )         (\
             ( _PMemoryWarehouse_ )->PBitmap->NumberOfBits                             )




// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS

//++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_UNALLOCATED_PALLET_STAMP
//
// Description:
//      A "magic" value used to verify that a Pallet has been released.
//      I picked 11221963 (the date of John Kennedy's assassination for
//      no particular reason
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_UNALLOCATED_PALLET_STAMP  0x11221963




// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_IN_USE
//
// Description:
//      Returns the Number of Pallets in use in the Warehouse
//
// Arguments:
//      _PMemoryWarehouse_
//
// Return Value:
//      Number of Pallets in use in the Warehouse
//
// Revision History:
//      04/19/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_IN_USE( _PMemoryWarehouse_ )         (\
             K10_MISC_UTIL_BITMAP_BITS_SET( (_PMemoryWarehouse_)->PBitmap )          )




// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_IN_USE

// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_FREE
//
// Description:
//      Returns the Number of Pallets NOT in use in the Warehouse
//
// Arguments:
//      _PMemoryWarehouse_
//
// Return Value:
//      Number of Pallets in use NOT in the Warehouse
//
// Revision History:
//      5/9/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_FREE( _PMemoryWarehouse_ )               (\
             (K10_MISC_UTIL_MEMORY_WAREHOUSE_NUMBER_OF_PALLETS( _PMemoryWarehouse_ ) -   \
                K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_IN_USE( (_PMemoryWarehouse_ )))   \
                                                                                         )




// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLETS_FREE


// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_IN_PURVIEW
//
// Description:
//      Returns TRUE if the Pallet is managed by this Warehouse, FALSE Otherwise
//
// Arguments:
//      _PMemoryWarehouse_
//      _Pallet_
//
// Return Value:
//      TRUE  the Pallet is managed by this Warehouse
//      FALSE the Pallet is not managed by thus Warehouse
//
// Revision History:
//      05/07/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_IN_PURVIEW( _PMemoryWarehouse_ , _Pallet_ )      (\
        ((((ULONG_PTR) (_Pallet_)) >= ((ULONG_PTR) ((_PMemoryWarehouse_)->Pallets))) &&                 \
         (((ULONG_PTR) (_Pallet_)) < ((ULONG_PTR)((_PMemoryWarehouse_)->Pallets)) +                     \
                   (K10_MISC_UTIL_MEMORY_WAREHOUSE_NUMBER_OF_PALLETS((_PMemoryWarehouse_)) *    \
                                 ( _PMemoryWarehouse_ )->UlSizeOfPallet)))                      \
                                                                                                )



// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_IN_PURVIEW

// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_TO_BIT
//
// Description:
//      Returns the Bit in the Bitmap associated with this Pallet
//
// Arguments:
//      _PMemoryWarehouse_
//      _Pallet_
//
// Return Value:
//      The Bit associated with a particular Pallet
//      K10_MISC_UTIL_BITMAP_INVALID_BIT_INDEX the Pallet is not managed by this Warehouse
//
// Revision History:
//      05/08/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_TO_BIT( _PMemoryWarehouse_ , _Pallet_ )        (\
    (K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_IN_PURVIEW(( _PMemoryWarehouse_ ), ( _Pallet_ ))?  \
    ((PtrToUlong((void *)(((ULONG_PTR)( _Pallet_ ) - (ULONG_PTR)((_PMemoryWarehouse_)->Pallets)) /                         \
                                 (( _PMemoryWarehouse_ )->UlSizeOfPallet))))):                   \
                                 K10_MISC_UTIL_BITMAP_INVALID_BIT_INDEX)                      \
                                                                                              )




// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_PALLET_TO_BIT


// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_BIT_TO_PALLET
//
// Description:
//      This macro returns the Pallet associated with a particular Bit. It returns
//      NULL if the Bit it out of Range
//
// Arguments:
//      _Bit_
//      _PMemoryWarehouse_
//
// Return Value:
//      None
//
// Revision History:
//      05/08/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_BIT_TO_PALLET( _Bit_ , _PMemoryWarehouse_ )              (\
                                                                                                 \
          ((_Bit_) < K10_MISC_UTIL_MEMORY_WAREHOUSE_NUMBER_OF_PALLETS( _PMemoryWarehouse_ )?     \
          (PUCHAR)(((ULONG)(PMemoryWarehouse->Pallets)) +                                        \
                   ((_Bit_) * (PMemoryWarehouse->UlSizeOfPallet))): NULL)                        \                                                                         \
                                                                                                 )


// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_WALK_PALLETS

// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_HIGH_WATER_MARK
//
// Description:
//      Returns the "High Water Mark" of the Memory Warehouse
//
// Arguments:
//      _PMemoryWarehouse_
//
// Return Value:
//      "High Water Mark" of the Memory Warehouse
//
// Revision History:
//      04/19/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_HIGH_WATER_MARK( _PMemoryWarehouse_ )   \
        ( _PMemoryWarehouse_->UlHighWaterMark )           )




// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_HIGH_WATER_MARK

// ++
// Macro:
//      K10_MISC_UTIL_MEMORY_WAREHOUSE_WALK_PALLETS
//
// Description:
//      This iterator walks through all Pallets in a Warehouse.
//
// Arguments:
//      _PCurrent_
//      _PMemoryWarehouse_
//
// Return Value:
//      None
//
// Revision History:
//      05/08/01   MWagner    Created.
//--
#define K10_MISC_UTIL_MEMORY_WAREHOUSE_WALK_PALLETS( _PCurrent_ , _PMemoryWarehouse_ )                          \
    for ( _PCurrent_ = ( _PMemoryWarehouse_ )->Pallets;                                                         \
         ((ULONG) ( _PCurrent_ )) < (((ULONG) (( _PMemoryWarehouse_ )->Pallets)) +                               \
                                   ((K10_MISC_UTIL_MEMORY_WAREHOUSE_NUMBER_OF_PALLETS( _PMemoryWarehouse_ )  *  \
                                    ( _PMemoryWarehouse_ )->UlSizeOfPallet)));                                     \
         ( _PCurrent_ ) = (PUCHAR)(((ULONG)(_PCurrent_)) + (( _PMemoryWarehouse_ )->UlSizeOfPallet))              \
        )


// .End K10_MISC_UTIL_MEMORY_WAREHOUSE_WALK_PALLETS


//++
// Memory Warehouse Functions
//
// See K10MiscUtilMemoryWarehouse.c for descriptions of these functions
//--
ULONG
K10MiscUtilMemoryWarehouseCalculateMaxPallets(
                                             ULONG SzAllocation,
                                             ULONG SzPallet
                                             );
EMCPAL_STATUS
K10MiscUtilMemoryWarehouseInitialize(
                                    PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                    PK10_MISC_UTIL_BITMAP           PBitmap,
                                    ULONG                           UlSizeOfPallet,
                                    ULONG                           UlNumberOfPallets,
                                    PVOID                           Pallets
                                    );

EMCPAL_STATUS
K10MiscUtilMemoryWarehouseOrganize(
                                  IN  PK10_MISC_UTIL_MEMORY_WAREHOUSE         pMemoryWarehouse,
                                  IN  ULONG                                   SzTotalSpace,
                                  IN  ULONG                                   SzPallet
                                  );
EMCPAL_STATUS
K10MiscUtilMemoryWarehouseRequisitionPallet(
                                           PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                           PVOID*                          PPallet
                                           );
EMCPAL_STATUS
K10MiscUtilMemoryWarehouseReturnPallet(
                                      PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                      PVOID                           Pallet
                                      );
PUCHAR
K10MiscUtilMemoryWarehouseNextRequisitionedPallet(
                                                 PK10_MISC_UTIL_MEMORY_WAREHOUSE PMemoryWarehouse,
                                                 PUCHAR                          PCurrentPallet
                                                 );
EMCPAL_STATUS
K10MiscUtilMemoryWarehouseInterlockedRequisitionPallet(
                                                      PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                                      PVOID*                          PPallet,
                                                      PEMCPAL_SPINLOCK                     PLock
                                                      );
EMCPAL_STATUS
K10MiscUtilMemoryWarehousePALInterlockedRequisitionPallet(
                                                         PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                                         PVOID*                          PPallet,
                                                         PEMCPAL_SPINLOCK                PLock
                                                         );
EMCPAL_STATUS
K10MiscUtilMemoryWarehouseInterlockedReturnPallet(
                                                 PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                                 PVOID                           Pallet,
                                                 PEMCPAL_SPINLOCK                     PLock
                                                 );

EMCPAL_STATUS
K10MiscUtilMemoryWarehousePALInterlockedReturnPallet(
                                                 PK10_MISC_UTIL_MEMORY_WAREHOUSE PWarehouse,
                                                 PVOID                           Pallet,
                                                 PEMCPAL_SPINLOCK                PLock
                                                 );
PUCHAR
K10MiscUtilMemoryWarehouseInterlockedNextRequisitionedPallet(
                                                            PK10_MISC_UTIL_MEMORY_WAREHOUSE PMemoryWarehouse,
                                                            PUCHAR                          PCurrentPallet,
                                                            PEMCPAL_SPINLOCK                     PLock
                                                            );

PUCHAR
K10MiscUtilMemoryWarehousePALInterlockedNextRequisitionedPallet(
                                                            PK10_MISC_UTIL_MEMORY_WAREHOUSE PMemoryWarehouse,
                                                            PUCHAR                          PCurrentPallet,
                                                            PEMCPAL_SPINLOCK                PLock
                                                            );
//++
// Macro:
//
//      K10_MISC_UTIL_GET_ENTRY_IRQL
//
// Description:
//      This function allocates two IRQLS, one for entry and one for exit. It initializes
//      the Entry IRQL.
//
//      The semi-colon is left off of the second declaration intentionally.
//
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      07/24/01     MWagner  Created.
//
//--
#ifdef DEBUG
    #define K10_MISC_UTIL_GET_ENTRY_IRQL()                                (\
     EMCPAL_KIRQL             _irqlK10MiscUtilEntryIrql_  = EmcpalGetCurrentLevel();   \
     EMCPAL_KIRQL             _irqlK10MiscUtilExitIrql_   = EMCPAL_LEVEL_HIGH            \
                                                                           )

#else
    #define K10_MISC_UTIL_GET_ENTRY_IRQL()
#endif





//++
// .End K10_MISC_UTIL_GET_ENTRY_IRQL
//--


//++
// Macro:
//
//      K10_MISC_UTIL_CHECK_EXIT_IRQL_IS_EQUAL_TO_ENTRY_IRQL
//
// Description:
//      This macro verifies that the exit IRQL is equal to the Entry IRQL and
//      BugChecks otherwise.
//
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      07/24/01     MWagner  Created.
//
//--
#ifdef DEBUG
    #define K10_MISC_UTIL_CHECK_EXIT_IRQL_IS_EQUAL_TO_ENTRY_IRQL()            (\
     _irqlK10MiscUtilExitIrql_  = EmcpalGetCurrentLevel();                          \
     if (_irqlK10MiscUtilEntryIrql_ !=  _irqlK10MiscUtilExitIrql_)             \
     {                                                                         \
         KLogBugCheckEx(K10_MISC_UTIL_ERROR_EXIT_IRQL_NOT_EQUAL_TO_ENTRY_IRQL, \
                        (ULONG) _irqlK10MiscUtilEntryIrql_,                    \
                        (ULONG) _irqlK10MiscUtilExitIrql_,                     \
                        (ULONG) __LINE__,                                      \
                        (ULONG) 0);                                            \
     }                                                                         \
                                                                               )

#else
    #define K10_MISC_UTIL_CHECK_EXIT_IRQL_IS_EQUAL_TO_ENTRY_IRQL()
#endif


//++
// .End K10_MISC_UTIL_CHECK_EXIT_IRQL_IS_EQUAL_TO_ENTRY_IRQL
//--

//++
// Macro:
//
//      K10_MISC_UTIL_STACK_LOCATIONS_EX_PARAMETER_NAME
//
// Description:
//      The name of the Registry Parameter that contains the
//  computed stack locations size, taking into account
//  driver mutual exclusivity.
//
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      2004-06-09 Victor Kan   Created.
//
//--
#define  K10_MISC_UTIL_STACK_LOCATIONS_EX_PARAMETER_NAME                                                     \
                  "StackLocationsEx"

//++
// Macro:
//
//      K10_MISC_UTIL_STACK_LOCATIONS_SUBKEY_PATH
//
// Description:
//      The path to the Registry Parameter that contains the
//  computed stack locations size, taking into account
//  driver mutual exclusivity.
//
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      2004-06-09 Victor Kan   Created.
//
//--
#define  K10_MISC_UTIL_STACK_LOCATIONS_SUBKEY_PATH                                                     \
                  "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\ScsiTargConfig\\"



//++
// Macro:
//
//      K10_MISC_UTIL_INVALID_STACK_LOCATIONS
//
// Description:
//      An invalid number of stack locations. If a client ignore the return status from
//      K10MiscUtilGetIrpStackSizeFromRegistry(), they will probably blow up when they try
//      to allocate an Irp with this many stack locations.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      10/04/01     MWagner  Created.
//
//--
#define  K10_MISC_UTIL_INVALID_STACK_LOCATIONS                      0xFFFFFFFF




//++
// Macro:
//
//      K10_MISC_UTIL_MAXIMUM_SUBKEY_NAME_LENGTH
//
// Description:
//      The maximum length of a subkey enumerated in K10MiscUtilGetIrpStackSizeFromRegistry()
//
// Revision History:
//      10/04/01     MWagner  Created.
//
//--
#define  K10_MISC_UTIL_MAXIMUM_SUBKEY_NAME_LENGTH                      64



//++
// Macro:
//
//      K10_MISC_UTIL_IRP_STACK_SIZE_INFORMATION_SIZE
//
// Description:
//      The size of the KEY_VALUE_FULL_INFORMATION packet in K10MiscUtilGetIrpStackSizeFromRegistry()
//
// Arguments:
//      None.
//
// Revision History:
//      10/04/01     MWagner  Created.
//
//--
#define  K10_MISC_UTIL_IRP_STACK_SIZE_INFORMATION_SIZE                                  (\
             sizeof(KEY_VALUE_FULL_INFORMATION) +                                        \
             ((K10_MISC_UTIL_MAXIMUM_SUBKEY_NAME_LENGTH - 1) * sizeof(WCHAR))            \
                                                                                         )



//++
// Function:
//      K10MiscUtilGetIrpStackSizeFromRegistry()
//
// Description:
//     Derives the number of IRP stack locations needed for IRPs that are
//     sent down the diskside "driver stack".
//
//     This is done by adding together the subkey values within the
//     registry at HKLM/System\CCS\Services\ScsiTargConfig\StackLocations.
//
// Arguments:
//     PIrpStackSize     the number of stack locations needed by an IRP on this Array
//
// Return Value:
//      STATUS_SUCCESS:
//      Others        : STATUS_INSUFFICIENT_RESOURCES, or NTSTATUS returned by ZwOpenKey,
//                      ZwEnumerateValueKey or ZwClose.
//
//                      If an error is returned, the PIrpStackSize will be set to
//                      K10_MISC_UTIL_INVALID_STACK_LOCATIONS. Presumably, any driver that ignores
//                      the return status and attempts to allocate an IRP with 4.2 billion stack
//                      location will discover the error of its ways quickly.
//
// Revision History:
//      10/10/01    MWagner  copied from host\Tcd\Driver\registry.c
//
//--

EMCPAL_STATUS
K10MiscUtilGetIrpStackSizeFromRegistry(
                                      PULONG PIrpStackSize
                                      );


//++
//
//  K10 MISC UTIL BLOCK BITMAP
//
//  The K10 Misct Util Block Bitmap is intended to be used for Bitmaps that are written
//  to and read from disk. FLARE IO is only guranteed to be atomic for 512 Byte blocks,
//  so Bitmaps that span larger Disk Areas may be corrupted by a failure during a FLARE
//  Write. The Block Bitmaps are broken into 512 Byte Sections, each consistent unto
//  itself.
//
//  The additional overhead is the use must store the Number Of Bits, if the Range Checking
//  functions are used- this Bitmap does *not* store the total number of bits.
//
//
//--


//++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_SIZE
//
// Description:
//      Size of the "Block" used
//
// Revision History:
//      05/07/02   MWagner    Created.
//--

#define K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_SIZE           512



// .End K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_SIZE

//++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK
//
// Description:
//      The number of Bits in a Block
//      Header = 8 Bytes
//      Bits   = 504 Bytes
//      504 * 8 = 4032
//
// Revision History:
//      05/07/02   MWagner    Created.
//--

#define K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK         4032


//++
// Type:
//      K10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP
//
// Description:
//      This is the internal Bitmap used by the K10_MISC_UTITL_BLOCK_BITMAP.
//      It's only reason for existence it that it is a fixed size, so we
//      can do pointer [and array] arithmetic, and not just increment by
//      sizeof(K10_MISC_UTIL_BITMAP), which is is just the size of the header
//
//      The point is
//           sizeof(K10_MISC_UTIL_BITMAP) = 9
//           sizeofK10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP = 512
//
// Members
//      NumberOfBits    4032 - here for compatility with K10_MISC_UTIL_BITMAP
//      BitsSet         Count of Bits Set
//      Bits            The bits themselves
//
// Revision History
//      5/9/02  MWagner created.
//
//--
typedef struct _K10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP
{
    ULONG NumberOfBits;
    ULONG BitsSet;
    UCHAR Bits[K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK  /  K10_MISC_UTIL_BITS_PER_UCHAR];

} K10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP, *PK10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP;
//++
// End Type K10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP
//--

//++
// Type:
//      K10_MISC_UTIL_BLOCK_BITMAP
//
// Description:
//      A Bitmap made up of a number of 512 Byte internal bitmaps
//
// Members:
//
// Revision History:
//      3/13/01   MWagner    Created.
//
//--
typedef struct _K10_MISC_UTIL_BLOCK_BITMAP
{

    K10_MISC_UTIL_BLOCK_BITMAP_INTERNAL_MAP    InternalMaps[1];

} K10_MISC_UTIL_BLOCK_BITMAP, *PK10_MISC_UTIL_BLOCK_BITMAP;
// .End K10_MISC_UTIL_BITMAP


// .End K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK


//++
// Block Bitmaps Macro Interface
//
// The macro Block Bitmap interface does not do any range checking or tracing, and does not
// offer any compound or interlocked functions. It is intended to be fast and
// relatively lightweight.
//
// The macros use "/ K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK" to find the
//  appropriate Block, and "% K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK"
//  to find the bit in that Block.
//
//--


// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS
//
// Description:
//      This macro calculates the size of a Block Bitmap, in Blocks
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Block Bitmap in Blocks
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS( _NumberOfBits_ )(\
        ((_NumberOfBits_) +                                                  \
             (K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK - 1))          \
        / K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK                    \
                                                                             )


// .End K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS

//++
// Macro:
//     K10_MISC_UTIL_BLOCK_BITMAP_BITS_PER_REGION
//
// Description:
//     How many bits are there in a Bitmap in an Region?
//
// Revision History
//      5/22/03  MWagner created.
//
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_BITS_PER_REGION( _RegionSize_ )            (\
        K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK  *                     \
           (( _RegionSize_ ) / K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_SIZE)          \
                                                                               )
// End Macro K10_MISC_UTIL_BLOCK_BITMAP_BITS_PER_REGION

// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP
//
// Description:
//      This macro calculates the size of a Block Bitmap, in Bytes
//
// Arguments:
//      _NumberOfBits_
//
// Return Value:
//      XXX:    the size of the Block Bitmap in Bytes
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP( _NumberOfBits_ )             (\
           K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS( _NumberOfBits_ ) \
           * K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_SIZE                              \
                                                                                )


// .End K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS

// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX
//
// Description:
//      Initializes all of the Block Bitmap Headers
//
// Arguments:
//      _BitIndex_
//
// Return Value:
//      Index of Block containing the Bit
//
// Revision History:
//      05/07/02   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX( _BitIndex_ )         \
             (( _BitIndex_ ) / K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK)



// .End K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX


// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX
//
// Description:
//      Initializes all of the Block Bitmap Headers
//
// Arguments:
//      _BitIndex_
//
// Return Value:
//      Index of Bit within Block containing the Bit
//
// Revision History:
//      05/07/02   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX( _BitIndex_ )         \
             (( _BitIndex_ ) % K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK)



// .End K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX


// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_INIT_HEADER
//
// Description:
//      Initializes all of the Block Bitmap Headers
//
// Arguments:
//      _BlockBitMap_
//      _BitIndex_
//
// Return Value:
//      None
//
// Revision History:
//      05/07/02   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_INIT_HEADER( _Bitmap_, _NumberOfBits_ )                                      \
             do                                                                                                 \
             {                                                                                                  \
                 ULONG _remainderBits_ = 0;                                                                     \
                 ULONG _blockIndex_ = 0;                                                                        \
                                                                                                                \
                 if (_NumberOfBits_)                                                                            \
                 {                                                                                              \
                     for (_blockIndex_ = 0;                                                                     \
                          _blockIndex_ < K10_MISC_UTIL_BLOCK_BITMAP_SIZEOF_BITMAP_IN_BLOCKS(_NumberOfBits_ ) - 1;   \
                          _blockIndex_++)                                                                       \
                     {                                                                                          \
                         K10_MISC_UTIL_BITMAP_INIT_HEADER(                                                      \
                             &(((PK10_MISC_UTIL_BLOCK_BITMAP)( _Bitmap_ ))->InternalMaps[_blockIndex_]),        \
                             K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK);                                  \
                     }                                                                                          \
                     _remainderBits_ = ( _NumberOfBits_ ) %                                                     \
                                        K10_MISC_UTIL_BLOCK_BITMAP_BLOCK_BITS_PER_BLOCK;                        \
                     if ( _remainderBits_ )                                                                     \
                     {                                                                                          \
                         K10_MISC_UTIL_BITMAP_INIT_HEADER(                                                      \
                             &(((PK10_MISC_UTIL_BLOCK_BITMAP)( _Bitmap_ ))->InternalMaps[_blockIndex_]),        \
                             _remainderBits_);                                                                  \
                     }                                                                                          \
                 }                                                                                              \
                                                                                                                \
             } while (0)



// .End K10_MISC_UTIL_BLOCK_INIT_HEADER


// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_TEST
//
// Description:
//      This macro tests a Bit
//
// Arguments:
//      _BlockBitMap_
//      _BitIndex_
//
// Return Value:
//      TRUE  : the Bit is set
//      FALSE : the Bit is not set
//
// Revision History:
//      05/07/02   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_TEST( _Bitmap_, _BitIndex_ )                                                                      \
    K10_MISC_UTIL_BITMAP_TEST(                                                                                                       \
       &(((PK10_MISC_UTIL_BLOCK_BITMAP)( _Bitmap_ ))->InternalMaps[K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX(_BitIndex_)]),   \
       K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX(_BitIndex_))                                                                   \



// .End K10_MISC_UTIL_BLOCK_BITMAP_TEST



// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_SET
//
// Description:
//      This macro sets a Bit that is unset. It does no range checking.
//
// Arguments:
//      _BitMap_
//      _BitIndex_
//
// Return Value:
//      None.
//
// Revision History:
//      03/13/01   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_SET( _Bitmap_, _BitIndex_ )                      \
          do {                                                                      \
             K10_MISC_UTIL_BITMAP_SET(                                              \
                 &(((PK10_MISC_UTIL_BLOCK_BITMAP)( _Bitmap_ ))->InternalMaps[K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX(_BitIndex_)]),   \
                 K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX(_BitIndex_));        \
          } while (0)                                                                      \



// .End K10_MISC_UTIL_BLOCK_BITMAP_SET

// ++
// Macro:
//      K10_MISC_UTIL_BLOCK_BITMAP_CLEAR
//
// Description:
//      This macro clears a Bit that is set.
//
// Arguments:
//      _BitMap_
//      _BitIndex_
//
// Return Value:
//      None.
//
// Revision History:
//      05/07/02   MWagner    Created.
//--
#define K10_MISC_UTIL_BLOCK_BITMAP_CLEAR( _Bitmap_, _BitIndex_ )                    \
        do {                                                                        \
             K10_MISC_UTIL_BITMAP_CLEAR(                                            \
                 &(((PK10_MISC_UTIL_BLOCK_BITMAP)( _Bitmap_ ))->InternalMaps[K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BLOCK_INDEX(_BitIndex_)]),   \
                 K10_MISC_UTIL_BLOCK_BITMAP_CALCULATE_BIT_INDEX(_BitIndex_));       \
        } while (0)



// .End K10_MISC_UTIL_BLOCK_BITMAP_CLEAR


//++
// Block Bitmap Functions
//
// See k10miscutilbitmap.c for descriptions of these functions
//--
EMCPAL_STATUS
K10MiscUtilBlockBitmapInitialize(
                                PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                ULONG                 NumberOfBits
                                );

EMCPAL_STATUS
K10MiscUtilBlockBitmapTestBit(
                             PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                             ULONG                 Bit,
                             ULONG                 NumberOfBits,
                             PBOOLEAN              PBitIsSet
                             );
EMCPAL_STATUS
K10MiscUtilBlockBitmapSetBit(
                            PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                            ULONG                 Bit,
                            ULONG                 NumberOfBits
                            );
EMCPAL_STATUS
K10MiscUtilBlockBitmapClearBit(
                              PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                              ULONG                 Bit,
                              ULONG                 NumberOfBits
                              );
EMCPAL_STATUS
K10MiscUtilBlockBitmapFindNextClearBitIndex(
                                           PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                           ULONG                 StartBit,
                                           ULONG                 NumberOfBits,
                                           PULONG                PNextClearBit
                                           );
EMCPAL_STATUS
K10MiscUtilBlockBitmapFindNextSetBitIndex(
                                         PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                         ULONG                 StartBit,
                                         ULONG                 NumberOfBits,
                                         PULONG                PNextSetBitIndex
                                         );

//++
// The "interlocked Bitmap functions
//--
EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedTestBit(
                                        PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                        ULONG                 Bit,
                                        ULONG                 NumberOfBits,
                                        PBOOLEAN              PBitIsSet,
                                        PEMCPAL_SPINLOCK           PLock
                                        );
EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedSetBit(
                                       PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                       ULONG                 Bit,
                                       ULONG                 NumberOfBits,
                                       PEMCPAL_SPINLOCK           PLock
                                       );

EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedClearBit(
                                         PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                         ULONG                 Bit,
                                         ULONG                 NumberOfBits,
                                         PEMCPAL_SPINLOCK           PLock
                                         );

EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedTestAndSetBit(
                                              PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                              ULONG                 Bit,
                                              ULONG                 NumberOfBits,
                                              PEMCPAL_SPINLOCK           PLock,
                                              PBOOLEAN              PBitWasAlreadySet
                                              );
EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedFindNextClearBitIndex(
                                                      PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                                      ULONG                 StartBit,
                                                      ULONG                 NumberOfBits,
                                                      PULONG                PNextClearBit,
                                                      PEMCPAL_SPINLOCK           PLock
                                                      );
EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedFindNextSetBitIndex(
                                                    PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                                    ULONG                 StartBit,
                                                    ULONG                 NumberOfBits,
                                                    PULONG                PNextSetBitIndex,
                                                    PEMCPAL_SPINLOCK           PLock
                                                    );

EMCPAL_STATUS
K10MiscUtilBlockBitmapInterlockedFindAndSetNextClearBit(
                                                       PK10_MISC_UTIL_BLOCK_BITMAP PBitmap,
                                                       ULONG                 StartBit,
                                                       ULONG                 NumberOfBits,
                                                       PULONG                PNextClearBit,
                                                       PEMCPAL_SPINLOCK           PLock
                                                       );




//
// The following defines the prototype for the hash routines that the
// client must provide at linear hash table initialization time.  It is
// expected that the client use the same underlying logic to hash.  They
// are just used at different times depending on the information present
// at the time.
//
// The client should be aware that the hash by key routine will be executed
// much more frequently than the hash by entry routine.  Hopefully this can
// be used to garner better performance.
//

typedef
ULONG
(*K10_MISC_UTIL_LH_HASH_KEY_FUNC)(
                                 IN struct _K10_MISC_UTIL_LH_ANCHOR *pHashTable,
                                 IN PVOID                            key,
                                 IN ULONG                            hashLevel
                                 );

typedef
ULONG
(*K10_MISC_UTIL_LH_HASH_ENTRY_FUNC)(
                                   IN struct _K10_MISC_UTIL_LH_ANCHOR *pHashTable,
                                   IN PVOID                            pEntry,
                                   IN ULONG                            hashLevel
                                   );

//
// The following defines the prototype for the comparison routine that the
// client must provide at linear hash table initialization time.
//

typedef
BOOLEAN
(*K10_MISC_UTIL_LH_EQUALS_FUNC)(
                               IN PVOID                            key,
                               IN PVOID                            pEntry
                               );

//
// The following defines the prototype for the allocation routine that the
// client must provide at linear hash table initialization time.
//

typedef
PVOID
(*K10_MISC_UTIL_LH_ALLOC_FUNC)(
                              IN ULONG                            numberOfBytes
                              );

//
// The following defines the prototype for the free routine that the
// client must provide at linear hash table initialization time.
//

typedef
VOID
(*K10_MISC_UTIL_LH_FREE_FUNC)(
                             IN PVOID                            buffer,
                             IN ULONG                            numberOfBytes
                             );

//
// The following defines the initial starting value for the number of chains.
//

#define K10_MISC_UTIL_LH_INITIAL_NUM_CHAINS         16


//
// The following defines the maximum average chain length.
//

#define K10_MISC_UTIL_LH_MAX_AVG_CHAIN_LENGTH       32


//++
//
//  Name:
//
//      K10_MISC_UTIL_LH_ANCHOR
//
//  Description:
//
//      This structure defines the binding data structure for a linear hash
//      table.  The memory for this routine must be supplied by the caller.
//
//--

typedef struct _K10_MISC_UTIL_LH_ANCHOR
{
    //
    // The array of chains.
    //

    PSINGLE_LIST_ENTRY                  Chains;

    //
    // The global lock for all chains
    //

    EMCPAL_SPINLOCK                          GlobalLock;

    //
    // The current number of chains.
    //

    ULONG                               NumberOfChains;

    //
    // The offset of the hash table link in the hashed data structure.
    // Typically clients use "FIELD_OFFSET( <TYPE>, <FIELD> )" to quantify
    // this value.
    //

    ULONG                               LinkOffset;

    //
    // The initial number of chains.
    //

    ULONG                               InitialNumberOfChains;

    //
    // The number of chain pointers allocated.
    //

    ULONG                               MaxNumberOfChains;

    //
    // The current number of entries in the table.
    //

    ULONG                               NumberOfEntries;

    //
    // The next chain to be split.
    //

    ULONG                               NextToSplit;

    //
    // The number of time MaxChains has grown.
    //

    ULONG                               HashLevel;

    //
    // The maximum average chain length.
    //

    ULONG                               MaxAvgChainLength;

    //
    // The caller supplied hashing routine for hashing a key.
    //

    K10_MISC_UTIL_LH_HASH_KEY_FUNC      hashKeyFunction;

    //
    // The caller supplied hashing routine for hashing an entry.
    //

    K10_MISC_UTIL_LH_HASH_ENTRY_FUNC    hashEntryFunction;

    //
    // The caller supplied comparison routine.
    //

    K10_MISC_UTIL_LH_EQUALS_FUNC        equalsFunction;

    //
    // The caller supplied memory allocation routine.
    //

    K10_MISC_UTIL_LH_ALLOC_FUNC         allocFunction;

    //
    // The caller supplied memory free routine.
    //

    K10_MISC_UTIL_LH_FREE_FUNC          freeFunction;

} K10_MISC_UTIL_LH_ANCHOR, *PK10_MISC_UTIL_LH_ANCHOR;


//
// Exported routines for the linear hash table module.
//

#define K10MiscUtilLinearHashInitialize(_pHashTable_, \
                                        _LinkOffset_, \
                                        _hashKeyFunction_, \
                                        _hashEntryFunction_, \
                                        _equalsFunction_, \
                                        _allocFunction_, \
                                        _freeFunction_ ) \
    K10MiscUtilLinearHashInitializeEx((_pHashTable_), \
                                      (_LinkOffset_), \
                                      (_hashKeyFunction_), \
                                      (_hashEntryFunction_), \
                                      (_equalsFunction_), \
                                      (_allocFunction_), \
                                      (_freeFunction_), \
                                      K10_MISC_UTIL_LH_INITIAL_NUM_CHAINS, \
                                      K10_MISC_UTIL_LH_MAX_AVG_CHAIN_LENGTH )

EMCPAL_STATUS
K10MiscUtilLinearHashInitializeEx(
                               IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                               IN ULONG                            LinkOffset,
                               IN K10_MISC_UTIL_LH_HASH_KEY_FUNC   hashKeyFunction,
                               IN K10_MISC_UTIL_LH_HASH_ENTRY_FUNC hashEntryFunction,
                               IN K10_MISC_UTIL_LH_EQUALS_FUNC     equalsFunction,
                               IN K10_MISC_UTIL_LH_ALLOC_FUNC      allocFunction,
                               IN K10_MISC_UTIL_LH_FREE_FUNC       freeFunction,
                               IN ULONG                            initialNumChains,
                               IN ULONG                            avgChainLength
                               );

VOID
K10MiscUtilLinearHashFree(
                         IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable
                         );

EMCPAL_STATUS
K10MiscUtilLinearHashInsertNoLock(
                           IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                           IN PVOID                            pEntry,
                           IN PVOID                            key
                           );

EMCPAL_STATUS
K10MiscUtilLinearHashInsert(
                           IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                           IN PVOID                            pEntry,
                           IN PVOID                            key
                           );

VOID
K10MiscUtilLinearHashRemoveNoLock(
                           IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                           IN PVOID                            pEntry,
                           IN PVOID                            key
                           );

VOID
K10MiscUtilLinearHashRemove(
                           IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                           IN PVOID                            pEntry,
                           IN PVOID                            key
                           );

PVOID
K10MiscUtilLinearHashRetrieveNoLock(
                             IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                             IN PVOID                            key
                             );

PVOID
K10MiscUtilLinearHashRetrieve(
                             IN PK10_MISC_UTIL_LH_ANCHOR         pHashTable,
                             IN PVOID                            key
                             );



//++
// Macro:
//
//      K10_MISC_UTIL_REGISTRY_LOCATION_OF_MAX_NUM_IMAGES_ON_SYSTEM
//
// Description:
//      The location of the Key MaxNumImages in Registry
//
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      06/20/02     BXU  Created.
//
//--
#define  K10_MISC_UTIL_REGISTRY_LOCATION_OF_MAX_NUM_IMAGES_ON_SYSTEM                                   \
                  "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\K10DriverConfig"

//------------------------------------------------------------------------------
// K10 Misc Util IRP Pool
//------------------------------------------------------------------------------
//
// This utility manages a list of IRPS, to avoid allocating and freeing IRPs. It uses
// two quotas - the Minimum IRP Quota, and the Maxiumum IRP Quota as follows:
//
//    Minimum IRP Quota - when an IRP is freed, if the Minimum IRP quota has
//    not been met, then the IRP is placed on the IRP list. If the minimum quota
//    has been met, the IRP is freed.
//
//    Maximum IRP Quota - when an attempt is made to allocate an IRP, and there
//    are none on the IRP list, if the Maximum IRP Quota has been met, then
//    return NULL. Else, allocate a new IRP.
//
//    These two quotas can be combined to get different behaviors
//
//    *    Min = MAXULONG Max = MAXULONG - allocate as many IRPs as you
//    need, and never free them
//
//    *    Min = n Max = MAXULONG - simulates an NT Lookaside List, allocate
//    as many IRPs as you need, but keep at least n of them around for
//    re-use.
//
//    *    Min = n, Max = n - use exactly n IRPS, and never release them
//
//    Note that the Pool will call IoInitaizeIrp() or IoReuseIrp() when an IRP is re-used.
//------------------------------------------------------------------------------

//++
// K10_MISC_UTIL_IRP_POOL
//
// The fundamental pool type
//--
typedef struct _K10_MISC_UTIL_IRP_POOL
{
    EMCPAL_SPINLOCK                           Lock;
    ListOfIRPs                                IrpList;
    CCHAR                                     StackSize;
    ULONG                                     MinIrpQuota;
    ULONG                                     MaxIrpQuota;
    ULONG                                     IrpsAllocated;
    ULONG                                     IrpsInUse;
} K10_MISC_UTIL_IRP_POOL, *PK10_MISC_UTIL_IRP_POOL;


//++
// K10MiscUtilIrpPoolFill()
//
// Initialize the pool
//
// This function will return STATUS_INSUFFICIENT_RESOURCES if it cannot
// allocate the specified Initial Number of IRPS
//--
EMCPAL_STATUS
K10MiscUtilIrpPoolFill(PK10_MISC_UTIL_IRP_POOL PPool,
                       IN CCHAR                StackSize,
                       IN ULONG                MinIrpQuota,
                       IN ULONG                MaxIrpQuota,
                       IN ULONG                InitialNumberOfIrps);



//++
// K10MiscUtilIrpPoolDrain()
//
// Free all Irps
//
// This function will return K10_MISC_WARNING_IRP_POOL_DRAIN_IRPS_IN_USE
// if there are IRPs in use when it is called.
//--
EMCPAL_STATUS
K10MiscUtilIrpPoolDrain(PK10_MISC_UTIL_IRP_POOL PPool);



//++
// K10MiscUtilIrpPoolAllocate(PK10_MISC_UTIL_IRP_POOL PPool)
//
// If there is an IRP on the Irp List, initialize it , and return it.
//
// If there is not an IRP on the Irp List, and the Max Quota
// has not been exceeded, then allocate new IRP, else return NULL.
//
//--
PEMCPAL_IRP
K10MiscUtilIrpPoolAllocate(PK10_MISC_UTIL_IRP_POOL PPool);


#define K10_MISC_UTIL_REGISTRY_DRIVER_CONFIG                                                            \
                "\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\K10DriverConfig"

// K10MiscUtilIrpPoolFree()
//
// If there are already Minimum IRP Quota IRPs allocated, then
// free the IRP, Else stick it on the IRP list.
//--

VOID
K10MiscUtilIrpPoolFree(PK10_MISC_UTIL_IRP_POOL PPool, PEMCPAL_IRP PIrp);

ULONG32
K10MiscUtilConvertNtTimeToCTime( 
    IN EMCPAL_LARGE_INTEGER NtTime);


//
//  Defines a KSPIN_LOCK that is eight-byte aligned in order to avoid the IERR
//  panics
//
typedef CSX_ALIGN_N(8) EMCPAL_SPINLOCK K10_MISC_UTIL_ALIGNED_EMCPAL_SPINLOCK, *PK10_MISC_UTIL_ALIGNED_EMCPAL_SPINLOCK;



//++
// Range Lock Data Structures and Functions
//--

#define INLINE static __inline

#define K10_MISC_UTIL_RANGE_LOCK_HASH_TABLE_SIZE    16


typedef enum 
{
    RL_SUCCESS,

    RL_UNAVAILABLE,

    RL_LOCK_NOT_FOUND,

}  rlStatus; // Range Lock Status
  
struct _K10_MISC_UTIL_RANGE_LOCK_REQUEST;

typedef void ( *K10_MISC_UTIL_RANGE_LOCK_REQUEST_CALLBACK ) (
    struct _K10_MISC_UTIL_RANGE_LOCK_REQUEST    *request,
    void                                        *context );

typedef struct _K10_MISC_UTIL_RANGE_LOCK_REQUEST
{
    ULONGLONG                                       lba_start;      
                                                        // Starting address for the lock

    ULONGLONG                                       lba_end;
                                                        // Ending address for the lock

    K10_MISC_UTIL_RANGE_LOCK_REQUEST_CALLBACK       callback_routine;

    void                                            *context;

    struct _K10_MISC_UTIL_RANGE_LOCK_REQUEST        *next_lock_request;

    EMCPAL_LARGE_INTEGER                                   ticks;

    EMCPAL_LIST_ENTRY                               link;

    ULONG                                           hash_index;   
                                                        // Index to which list this request resides

    BOOLEAN                                         lock_region_overlapped;
                                                        // Indicates if another lock request overlapped this
                                                        // lock region and was rejected

} K10_MISC_UTIL_RANGE_LOCK_REQUEST, *PK10_MISC_UTIL_RANGE_LOCK_REQUEST;




typedef struct _K10_MISC_UTIL_RANGE_LOCK
{

    ULONGLONG                       hash_divisor;    // Current 'Bucket' Size

    ULONGLONG                       hash_bias;       // Skew for effecient alignment handling

    ULONG                           lock_count;        // Number of locks outstanding

    ULONG                           rehash_count;      // Number of times we have rehashed

    EMCPAL_SPINLOCK                      locking;      // Spin lock for when we are acquiring a lba lock 

    EMCPAL_LIST_ENTRY               hash_table[ K10_MISC_UTIL_RANGE_LOCK_HASH_TABLE_SIZE ];   // The Hash Table


} K10_MISC_UTIL_RANGE_LOCK, *PK10_MISC_UTIL_RANGE_LOCK;


//
// Name: K10MiscUtilRangeLockRequestInitialize
//
// Description: 
//    Initialize lock request to default value
//
// Arguments: 
//     PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to initialize
//
// Returns:
//     None

INLINE void K10MiscUtilRangeLockRequestInitialize(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request)

{

    request -> lba_start = 0;

    request -> lba_end = 0;

    request -> callback_routine = NULL;

    request -> context = NULL;

    request -> next_lock_request = NULL;

    request -> ticks.QuadPart = 0;

    EmcpalInitializeListHead( &request -> link );

    request -> hash_index = -1;   
                                             
    request -> lock_region_overlapped = FALSE;

    return;
}

//
// Name: K10MiscUtilRangeLockRequestUninitialize
//
// Description: 
//    Unitialize lock request to default value
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to uninitialize when it 
//                                                is finished
//
// Returns:
//     None

INLINE void K10MiscUtilRangeLockRequestUninitialize(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request)
{
    request -> hash_index = -1;   
    return;
}


//
// Name: K10MiscUtilRangeLockRequestSetStartLBA
//
// Description: 
//    Set start LBA of lock request
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to uninitialize when it 
//                                                is finished
//    ULONGLONG lba - Set start LBA of lock request         
//
// Returns:
//    None

INLINE void K10MiscUtilRangeLockRequestSetStartLBA(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request,
    ULONGLONG                               lba)
{
    request -> lba_start = lba;
}


// Name: K10MiscUtilRangeLockRequestSetEndLBA
//
// Description: 
//    Set end LBA of lock request
//
// Arguments: 
//     PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to uninitialize when it 
//                                                 is finished
//     ULONGLONG lba - Set end LBA of lock request         
//
// Returns:
//     None
INLINE void K10MiscUtilRangeLockRequestSetEndLBA(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request,
    ULONGLONG                               lba)
{
    request -> lba_end = lba;
    return;
}


//
// Name: K10MiscUtilRangeLockRequestGetStartLBA
//
// Description: 
//    Get start LBA of lock request
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to uninitialize when it 
//                                                is finished
//
// Returns:
//    ULONGLONG - returned Start LBA    

INLINE ULONGLONG K10MiscUtilRangeLockRequestGetStartLBA(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request) 
{
    return request -> lba_start;
}

//
// Name: K10MiscUtilRangeLockRequestGetEndLBA
//
// Description: 
//    Get end LBA of lock request
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to uninitialize when it 
//                                                is finished
//
// Returns:
//    ULONGLONG - returned End LBA    

INLINE ULONGLONG K10MiscUtilRangeLockRequestGetEndLBA( 
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request) 
{
    return request -> lba_end;
}


//
// Name: K10MiscUtilRangeLockRequestGetTimeStamp
//
// Description: 
//    Get end LBA of lock request
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request whose time stamp to be retrieved
//
// Returns:
//    LARGE_INTEGER - The time stamp of this lock request

INLINE EMCPAL_LARGE_INTEGER K10MiscUtilRangeLockRequestGetTimeStamp(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request )
{
    return request -> ticks;
}

//
// Name: K10MiscUtilRangeLockRequestWasLockregionOverlapped
//
// Description: 
//    Returns TRUE if another lock attempted to access the region
//    this lock is in while this lock was held
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST request - The lock request to be checked
//
// Returns:
//    BOOLEAN -  TRUE if the lock was overlapped by another request
//               FALSE if no request has asked for a lock in this
//                     region since the lock was granted.
INLINE BOOLEAN K10MiscUtilRangeLockRequestWasLockregionOverlapped(
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request )
{
    return request -> lock_region_overlapped;
}


//
// Name: K10MiscUtilRangeLockInitialize
//
// Description: 
//    Initialize LBA Lock Object
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - lock to initialize
//
// Returns:
//    none
//  
void K10MiscUtilRangeLockInitialize(
    PK10_MISC_UTIL_RANGE_LOCK               lock);

//
// Name: K10MiscUtilRangeLockUninitialize
//
// Description: 
//    Unitialize LBA Lock Object
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - lock to uninitialize
//
// Returns:
//    none
//
void K10MiscUtilRangeLockUninitialize(
    PK10_MISC_UTIL_RANGE_LOCK               lock);


//
// Name: K10MiscUtilRangeLockAcquireLock
//
// Description: 
//    Attempts to get a lock for the lba region provided in request.
//    Will return an error and NOT grant the lock if there is an overlap.
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - LBA lock object manages all the lock requests
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request - The lock request containing the lba range that is
//                                                  to be locked.
//
// Returns:
//    RL_SUCCESS - There was no overlap and the lock is granted.
//    RL_UNAVAILABLE - There was an overlap with another lock
//                               request, lock is denied.
//

rlStatus K10MiscUtilRangeLockAcquireLock(
    PK10_MISC_UTIL_RANGE_LOCK               lock,
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request);


//
// Name: K10MiscUtilRangeLockAcquireLockWithCallback
//
// Description: 
//    Attempts to get a lock for the lba region provided in request.
//    Will return an error and NOT grant the lock if there is an overlap.
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - LBA lock object manages all the lock requests
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request - The lock request containing the lba range that is
//                                                  to be locked.
//    K10_MISC_UTIL_RANGE_LOCK_REQUEST_CALLBACK callback - callback function
//    void *context - context of callback function
//
// Returns:
//    RL_SUCCESS - There was no overlap and the lock is granted.
//    RL_UNAVAILABLE - There was an overlap with another lock
//                              request, lock is denied.
//

rlStatus K10MiscUtilRangeLockAcquireLockWithCallback(
    PK10_MISC_UTIL_RANGE_LOCK                   lock,
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST           request,
    K10_MISC_UTIL_RANGE_LOCK_REQUEST_CALLBACK   callback,
    void                                        *context);


//
// Name: K10MiscUtilRangeLockAppendLock
//
// Description: 
//    Attempts to take an existing lock and increase its lock region
//    The newStartLba and endLba must designate a region that is already
//    held by the lock request provided.
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - LBA lock object manages all the lock requests
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request - The lock request that is already locked and
//                     we want to increase its lock region on.
//    ULONGLONG new_start_lba - The new start lba of the lock region
//    ULONGLONG new_end_lba - The new end la of the lock region
// 
// Returns:
//    RL_SUCCESS - There was no overlap and the new lock region is granted.
//    RL_UNAVAILABLE - The new region overlapped another lock, this failed,
//                              or the new lock region did not include the old lock
//                              region.
//                              The original lock is still lock however.
//    RL_LOCK_NOT_FOUND - The lock request given was not actually holding a lock.

rlStatus K10MiscUtilRangeLockAppendLock(
    PK10_MISC_UTIL_RANGE_LOCK               lock,
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request,
    ULONGLONG                               new_start_lba,
    ULONGLONG                               new_end_lba);

//
// Name: K10MiscUtilRangeLockReleaseLock
//
// Description: 
//    Releases a lock that has been previously granted with acquireLock()
//
// Arguments: 
//    PK10_MISC_UTIL_RANGE_LOCK lock - LBA lock object manages all the lock requests
//    PK10_MISC_UTIL_RANGE_LOCK_REQUEST   request - The lock request that is to be released
//
// Returns:
//    RL_SUCCESS - Lock was released
//    RL_LOCK_NOT_FOUND - The lock request was not found in the hash table
//
rlStatus K10MiscUtilRangeLockReleaseLock(
    PK10_MISC_UTIL_RANGE_LOCK               lock,
    PK10_MISC_UTIL_RANGE_LOCK_REQUEST       request);

//++
//  K10 Monostable Timer
//
//  Provides EMCPAL_MONOSTABLE_TIMER functionality, and supports the Boolean return value to
//  the Cancel function ( used to determine whether the timer expiry callback is already running ).
//  This functionality is implemented by an internal flag protected by a spinlock.


typedef struct _K10_MISC_UTIL_MONOSTABLE_TIMER
{
	EMCPAL_MONOSTABLE_TIMER    Timer;
	EMCPAL_TIMER_CALLBACK      UserCallback;
        EMCPAL_TIMER_CONTEXT       UserContext;

	EMCPAL_SPINLOCK            Lock;
	BOOLEAN                    isActive;

} K10_MISC_UTIL_MONOSTABLE_TIMER, *PK10_MISC_UTIL_MONOSTABLE_TIMER;

EMCPAL_STATUS
K10MiscUtilMonostableTimerCreate (IN  PEMCPAL_CLIENT                        pPalClient,
                                  IN  PK10_MISC_UTIL_MONOSTABLE_TIMER       pTimer,
                                  IN  const TEXT                            *pName,
                                  IN  EMCPAL_TIMER_CALLBACK                 pCallback,
                                  IN  EMCPAL_TIMER_CONTEXT                  pContext);
VOID
K10MiscUtilMonostableTimerDestroy(IN  PK10_MISC_UTIL_MONOSTABLE_TIMER       pTimer);

EMCPAL_STATUS
K10MiscUtilMonostableTimerStart  (IN  PK10_MISC_UTIL_MONOSTABLE_TIMER       pTimer,
                                  IN  EMCPAL_TIMEOUT_MSECS                  timeout,
                                  OUT PBOOLEAN                              pbIsTimerActive);
EMCPAL_STATUS
K10MiscUtilMonostableTimerCancel (IN  PK10_MISC_UTIL_MONOSTABLE_TIMER       pTimer,
                                  OUT PBOOLEAN                              pbIsTimerActive);


CSX_CDECLS_END


//      K10MiscUtilRegistryClerkReadBinary()
//
// Description:
//      Reads a binary value from the Registry.
//
// Arguments:
//      pRegistryPath    the NT supplied registry path
//      Offset           Offset into data to read
//      wstrValueName    Value Name
//      pValue           Value
//      BufferSize       Size of buffer pointed to by Value
//
// Return Value:
//      STATUS_SUCCESS:  the  Value was read and returned
//      Others:          return from one of the ZwXxx() functions
//
// Revision History:
//      4/21/10   JTaylor    Created.
//
//--
EMCPAL_STATUS
K10MiscUtilRegistryClerkReadBinary(
    csx_pchar_t pRegistryPath,
    csx_pchar_t strValueName,
    ULONG Offset,
    PUCHAR pValue,
    ULONG BufferSize
    );

//++
//
//  Function:
//
//      K10MiscUtilRegistryClerkWriteBinary
//
//  Description:
//
//      K10MiscUtilRegistryClerkWriteBinary() writes a binary value to the Registry
//
//  Arguments:
//
//      pRegistryPath    the NT supplied registry path
//      wstrValueName    Value Name
//      pValue           Value
//      BufferSize       Size of buffer pointed to by Value
//
//  Return Value:
//
//      STATUS_SUCCESS:  the  DWORD was read 
//      Others:          return from one of the ZwXxx() functions
//
//  Revision History:
//
//      4/21/10   JTaylor    Created.
//
//--

EMCPAL_STATUS
K10MiscUtilRegistryClerkWriteBinary(
    csx_pchar_t pRegistryPath,
    csx_pchar_t strValueName,
    PUCHAR pValue,
    ULONG BufferSize
);



/////////////////////////////////////////////////////
//    Interfaces exported from K10MiscUtilIncident.c
/////////////////////////////////////////////////////


//
//  A structure used to track a class of identical incidents and to
//  decide which particular instances of the incident are worthy of
//  further attention.
//
//  The contents of this structure should be treated as opaque by clients of K10MiscUtil.
//
typedef struct _K10_MISC_UTIL_INCIDENT_NOTEWORTHINESS
{
    //
    //  The number of incidents since the object was initialized
    //  or reset which will automatically be deemed noteworthy.  
    //
    ULONG64 NumInitialNoteworthyIncidents;

    //
    //  Additional incidents between <NumInitialNoteworthyIncidents>
    //  and this incident will be deemed non-noteworthy. 
    //
    ULONG64 InitialModValue;

    //
    //  The factor by which the mod interval grows each time it is
    //  reached. The incident at which an interval is reached
    //  is deemed noteworthy, and all subsequent incidents 
    //  until the next such interval are deemed non-noteworthy.
    //
    ULONG64 ModIntervalGrowth;

    //
    //  The maximum value that the mod interval may reach.
    //
    ULONG64 MaxModInterval;

    //
    //  The number of incidents that have been reported. 
    //
    ULONG64 LastRecordedIncident;

    //
    //  The instance nunmber of the next noteworthy incident. 
    //
    ULONG64 NextNoteworthyIncident;

    //
    //  The mod value currently in effect. 
    //
    ULONG64 CurrentModValue;

} K10_MISC_UTIL_INCIDENT_NOTEWORTHINESS, *PK10_MISC_UTIL_INCIDENT_NOTEWORTHINESS;


///
///  K10MiscUtilInitializeIncidentNoteworthiness
///
///  Initializes a noteworthiness object.
/// 
///  @param[in] pNoteworthiness
///      Client provides storage (opaque to client)
///  @param[in] NumInitialNoteworthyIncidents
///      The number of incidents since the object was initialized
///           or reset which will automatically be deemed noteworthy.  
///  @param[in]InitialModValue
///      Additional incidents between <NumInitialNoteworthyIncidents> and
///           this incident will be deemed non-noteworthy. 
///  @param[in] ModIntervalGrowth
///      The factor by which the mod interval grows each time it is
///           reached. The incident at which an interval is reached
///           is deemed noteworthy, and all subsequent incidents 
///           until the next such interval are deemed non-noteworthy.
///  @param[in] MaxModInterval
///      The maximum value that the mod interval may reach.
///
///  @retval void
///
void
K10MiscUtilInitializeIncidentNoteworthiness (
    IN OUT PK10_MISC_UTIL_INCIDENT_NOTEWORTHINESS pNoteworthiness,
    IN ULONG64                  NumInitialNoteworthyIncidents,
    IN ULONG64                  InitialModValue,
    IN ULONG64                  ModIntervalGrowth,
    IN ULONG64                  MaxModInterval
);

///
///  K10MiscUtilResetIncidentNoteworthiness
///
///  Resets a noteworthiness object's incident counter to zero.
/// 
///  @param[in out] pNoteworthiness
///      Address of a previously-initialized noteworthiness object.
///
///  @retval void
///
void
K10MiscUtilResetIncidentNoteworthiness (
    IN OUT PK10_MISC_UTIL_INCIDENT_NOTEWORTHINESS pNoteworthiness
);


///
///  K10MiscUtilRecordIncident
///
///    Records a new incident in a noteworthiness object and returns
///    information about that incident.
/// 
///  @param[in] pNoteworthiness
///      Address of a previously-initialized noteworthiness object.
///  @param[out] pIsNoteworthy
///      Address of a flag that will be set to TRUE if this particular
///           incident is noteworthy, else FALSE.
///  @param[out] pGapUntilNextNoteworthyIncident
///      Will be filled in with the number of additional non-noteworthy incidents
///           that must be recorded until one is deemed noteworthy.  
///
///  @retval ULONG64
///           The instance number (since the last time the object
///           was reset) of the incident that was recorded.
///
ULONG64
K10MiscUtilRecordIncident (
    IN OUT PK10_MISC_UTIL_INCIDENT_NOTEWORTHINESS pNoteworthiness,
    OUT PBOOLEAN                  pIsNoteworthy,
    OUT PULONG64                  pGapUntilNextNoteworthyIncident
);



#endif  // _K10_MISC_UTIL_H_

// End K10MiscUtil.h
