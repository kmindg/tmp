// fileheader
//
//   File:          MluVVolSupport.h
//
//   Description:   Contains all the MLU Support Library functions for VVol.
//
//
//  Copyright (c) 2015, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.
//
//
#ifndef MLUSUPPORTVVOL_H
#define MLUSUPPORTVVOL_H

#pragma once

#include "csx_ext.h"
#include "MluGeneralTypes.h"
#include "EmcPAL.h"

#if defined(__cplusplus)
extern "C"
{
#endif


#if defined(UMODE_ENV)

#define RETURN_STATUS       CSX_MOD_EXPORT HRESULT

#else

#define RETURN_STATUS       CSX_MOD_EXPORT EMCPAL_STATUS

#endif

// We're doing it only to ensure identical structure alignment between 32-bit and 64-bit executables,
//  so that ioctls sent by 32-bit user space programs to the 64-bit MLU driver will work correctly.
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */

#include "ScsiTargGlobals.h"
#include "ScsiTargLogicalUnits.h"

//=============================================================================
//
//  Name:
//      MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE
//
//  Description:
//      This enum defines the different types of VVol referencing in MLU_VVOL_INSERT_VU_REQUEST
//      and MLU_VVOL_REMOVE_VU_REQUEST
//
//=============================================================================
typedef enum _MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE
{
    //
    //  VVol is referenced by VMware UUID in binary format
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE_WWN,

    //
    //  VVol is referenced by VMware UUID in string
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE_WWN_STRING,

    //
    //  VVol is referenced by OID
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE_OID,

} MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE;

//=============================================================================
//
//  Name:
//      MLU_VVOL_PREPARE_VU_VVOL_REFERENCE
//
//  Description:
//      Union is used to identify single host address/name in a bind vvol call
//
//=============================================================================
typedef union _MLU_VVOL_PREPARE_VU_VVOL_REFERENCE
{
    //
    //  Binary form of VMware UUID
    //
    K10_WWID                  Uuid;

    //
    // VMware UUID in string form
    //
    MLU_MAX_VVOL_UUID_STRING  UuidString;

    //
    //  MLU VVol OID
    //
    MLU_OBJECT_ID_EXTERNAL    Oid;

} MLU_VVOL_PREPARE_VU_VVOL_REFERENCE;

//=============================================================================
//
//  Name:
//      MLU_VVOL_REMOVE_VU_REQUEST
//
//  Description:
//      This struct defines the data for a single release VU vvol call
//
//=============================================================================

typedef struct _MLU_VVOL_REMOVE_VU_REQUEST
{
    //
    // VVol identifier type
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE VVolReferenceType;

    //
    // VVol identifier
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE      VVol;

} MLU_VVOL_REMOVE_VU_REQUEST, *PMLU_VVOL_REMOVE_VU_REQUEST, * const PCMLU_VVOL_REMOVE_VU_REQUEST;

//=============================================================================
//
//  Name:
//      MLU_VVOL_REMOVE_VU_RESULT
//
//  Description:
//      This struct defines the single result of a prepare VU vvol call
//
//=============================================================================
typedef struct _MLU_VVOL_REMOVE_VU_RESULT
{
    //
    //  Result of bind operation
    //
    ULONG32                 Error;

    //
    //  OID of VVol that was processed
    //
    MLU_OBJECT_ID_EXTERNAL  VVolOid;

    //
    //  OID of resulting VU
    //
    MLU_OBJECT_ID_EXTERNAL  VuOid;

    //
    //  In case of update profile, Destination VU oid
    //
    MLU_OBJECT_ID_EXTERNAL   DestVuOid;

    //
    // Device parameters
    //
    XLU_PARAM               VuParams;

} MLU_VVOL_REMOVE_VU_RESULT, *PMLU_VVOL_REMOVE_VU_RESULT, * const PCMLU_VVOL_REMOVE_VU_RESULT;

//++
//
//  Name:
//      MLU_VVOL_REMOVE_VU_IN_BUF
//
//  Description:
//      Input buffer for MLU_VVOL_REMOVE_VU ioctl.
//
//--
typedef struct _MLU_VVOL_REMOVE_VU_IN_BUF
{
    //
    // RevisionID is used to ensure this structure's layout is supported.
    //
    ULONG32                         RevisionID;

    //
    // True for synchronous call
    //
    BOOLEAN                         Synchronous;

    //
    // Client ID of the caller
    //
    UCHAR                           ClientID;

    //
    // Number of members in VVol
    //
    ULONG32                         VVolCount;

    //
    // List of VVols to released
    //
    MLU_VVOL_REMOVE_VU_REQUEST      VVol[ MLU_VVOL_MAX_BIND_COUNT ];

} MLU_VVOL_REMOVE_VU_IN_BUF, *PMLU_VVOL_REMOVE_VU_IN_BUF, * const PCMLU_VVOL_REMOVE_VU_IN_BUF;

//++
//
//  Name:
//      MLU_VVOL_REMOVE_VU_OUT_BUF
//
//  Description:
//      Input buffer for MLU_VVOL_REMOVE_VU ioctl.
//
//--

typedef struct _MLU_VVOL_REMOVE_VU_OUT_BUF
{
    //
    // True if device map requires rebuild
    //
    BOOLEAN                    DeviceMapInvalidationNeeded;

    //
    // Number of results in Objects array
    //
    ULONG32                    NumberOfObjects;

    //
    // Objects - array of release VU results
    //
    MLU_VVOL_REMOVE_VU_RESULT  Objects[ MLU_VVOL_MAX_BIND_COUNT ];

} MLU_VVOL_REMOVE_VU_OUT_BUF, *PMLU_VVOL_REMOVE_VU_OUT_BUF, * const PCMLU_VVOL_REMOVE_VU_OUT_BUF;


//=============================================================================
//
//  Name:
//      MLU_VVOL_INSERT_VU_REQUEST
//
//  Description:
//      This struct defines the data for a single prepare VU vvol call
//
//=============================================================================

typedef struct _MLU_VVOL_INSERT_VU_REQUEST
{
    //
    // VVol identifier type
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE_TYPE VVolReferenceType;

    //
    // VVol identifier
    //
    MLU_VVOL_PREPARE_VU_VVOL_REFERENCE      VVol;

    //
    // ID of VVol storage group
    //
    K10_WWID                                StorageGroupID;

} MLU_VVOL_INSERT_VU_REQUEST, *PMLU_VVOL_INSERT_VU_REQUEST, * const PCMLU_VVOL_INSERT_VU_REQUEST;

//=============================================================================
//
//  Name:
//      MLU_VVOL_INSERT_VU_RESULT
//
//  Description:
//      This struct defines the single result of a prepare VU vvol call
//
//=============================================================================
typedef struct _MLU_VVOL_INSERT_VU_RESULT
{
    //
    //  Result of bind operation
    //
    ULONG32                 Error;

    //
    //  OID of VVol that was prepared
    //
    MLU_OBJECT_ID_EXTERNAL  VVolOid;

    //
    //  OID of resulting VU
    //
    MLU_OBJECT_ID_EXTERNAL  VuOid;

    //
    //  In case of update profile, Destination VU oid
    //
    MLU_OBJECT_ID_EXTERNAL     DestVuOid;

    //
    // Device parameters
    //
    XLU_PARAM               VuParams;

} MLU_VVOL_INSERT_VU_RESULT, *PMLU_VVOL_INSERT_VU_RESULT, * const PCMLU_VVOL_INSERT_VU_RESULT;


//++
//
//  Name:
//      MLU_VVOL_INSERT_VU_IN_BUF
//
//  Description:
//      Input buffer for MLU_VVOL_INSERT_VU ioctl.
//
//--
typedef struct _MLU_VVOL_INSERT_VU_IN_BUF
{
    //
    // RevisionID is used to ensure this structure's layout is supported.
    //
    ULONG32                         RevisionID;

    //
    // True for synchronous call
    //
    BOOLEAN                         Synchronous;

    //
    // Client ID of the caller
    //
    UCHAR                           ClientID;

    //
    // Number of members in VVol
    //
    ULONG32                         VVolCount;

    //
    // List of VVols to prepared
    //
    MLU_VVOL_INSERT_VU_REQUEST      VVol[ MLU_VVOL_MAX_BIND_COUNT ];

} MLU_VVOL_INSERT_VU_IN_BUF, *PMLU_VVOL_INSERT_VU_IN_BUF, * const PCMLU_VVOL_INSERT_VU_IN_BUF;

//++
//
//  Name:
//      MLU_VVOL_INSERT_VU_OUT_BUF
//
//  Description:
//      Output buffer for MLU_VVOL_INSERT_VU ioctl.
//
//--
typedef struct _MLU_VVOL_INSERT_VU_OUT_BUF
{
    //
    // True if device map requires rebuild
    //
    BOOLEAN                    DeviceMapInvalidationNeeded;

    //
    // Number of results in Objects array
    //
    ULONG32                    NumberOfObjects;

    //
    // Array of prepare VU results
    //
    MLU_VVOL_INSERT_VU_RESULT  Objects[ MLU_VVOL_MAX_BIND_COUNT ];

} MLU_VVOL_INSERT_VU_OUT_BUF, *PMLU_VVOL_INSERT_VU_OUT_BUF, * const PCMLU_VVOL_INSERT_VU_OUT_BUF;

//=============================================================================
//
//  Name:
//      MLU_VVOL_PREPARE_VU_CLIENT_HOSTSIDE
//
//  Description:
//      Client ID to be used by hostside driver when preparing VU
//
//=============================================================================

static const UCHAR MLU_VVOL_PREPARE_VU_CLIENT_HOSTSIDE  = 0x80;

//=============================================================================
//
//  Name:
//      MLU_VVOL_PREPARE_VU_CLIENT_MIGRATION
//
//  Description:
//      Client ID to be used by migration driver when preparing VU
//
//=============================================================================

static const UCHAR MLU_VVOL_PREPARE_VU_CLIENT_MIGRATION = 0x40;


//=============================================================================
//  Function:
//      MluSLInsertVVolVu
//
//  Description:
//      This function exposes VU and MLU-Bound part of autoinsert for set of VVols.
//
//  Arguments:
//      in - Describing parameters of operation and set of VVols that require
//           "exposure"
//      out - Describes result of each "exposure" per requested object
//
//  Return Value:
//      TODO
//
//=============================================================================
RETURN_STATUS
MluSLInsertVVolVu(
    IN  PMLU_VVOL_INSERT_VU_IN_BUF in,
    OUT PMLU_VVOL_INSERT_VU_OUT_BUF out
);

//=============================================================================
//  Function:
//      MluSLRemoveVVolVu
//
//  Description:
//      This functions insructs MLU that set of VUs (and MLU-Bound part of autoinsert)
//      (result of MluSLInsertVVolVu) are not needed anymore, optionally tearing
//      down IO stack
//
//  Arguments:
//      TDOD
//
//  Return Value:
//      TODO
//
//=============================================================================
RETURN_STATUS
MluSLRemoveVVolVu(
    IN  PMLU_VVOL_REMOVE_VU_IN_BUF in,
    OUT PMLU_VVOL_REMOVE_VU_OUT_BUF out
);


#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */

#if defined(__cplusplus)
}
#endif

#endif //MLUSUPPORTVVOL_H
