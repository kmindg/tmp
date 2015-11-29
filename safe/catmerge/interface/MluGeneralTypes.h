//  MLU General Type Definitions
//  
//  This header defines types that are common to the MLU driver and user-level
//  modules (this includes MLU libraries and clients of those libraries).
//  It is useful in the situation where MLU header files that reside in 
//  catmerge\include need types that would otherwise be defined in 
//  catmerge\layered\MLU\include\mluTypes.h.  Those types can be added to this
//  file so that modules don't need to add the path to mluTypes.h to the include
//  path.
//  
//  Copyright (c) 2010-2012, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.
// 

#ifndef  _MLU_GENERAL_TYPES_H_
#define  _MLU_GENERAL_TYPES_H_


//++
//  Name:
//      MLU_FCC_HANDLE
// 
//  Description:
//      This structure defines handle that FCC clients use to communicate
//      with the target Fixture.
// 
//--
typedef ULONG64 MLU_FCC_HANDLE, *PMLU_FCC_HANDLE;


//++
//  Name:
//      MLU_FCC_CHANNEL_NAME_LENGTH
//
//  Description:
//      Maximum length for an FCC channel name.
//
//--
#define MLU_FCC_CHANNEL_NAME_LENGTH     64


//++
//  Name:
//      MLU_FCC_CHANNEL_NAME
//
//  Description:
//      A char string that contains the name of an FCC channel.
//
//--
typedef CHAR    MLU_FCC_CHANNEL_NAME[ MLU_FCC_CHANNEL_NAME_LENGTH ];


//++
// Name:
//      MLU_CCN_FILTERS
//
// Description:
//      This enum defines the Configuration Change Notification (CCN) filters.
//      A client can specify a set of these filters with a registration request
//      and be notified when one or more filters in that set have been updated.
//      The user-level interface is provided by the MluSupport library.
//      Function definitions are included in MluSupport.h.
//
// Filter definitions:
//
// MLU_CCN_FILTER_POOL_ANY_CHANGE
//      Notification occurs when a pool object DeltaToken value is updated.
// MLU_CCN_FILTER_FSL_ANY_CHANGE
//      Notification occurs when the DeltaToken value of a VU object that
//      supports an FSL is updated.
// MLU_CCN_FILTER_TEST_EXCEPTION
//      Notification occurs when an object that has this filter enabled is
//      updated, or this filter is updated by MLU_CCN_TEST_130.  This is a
//      unit test filter to aid in testing the CCN framework.
// MLU_CCN_FILTER_DOMAIN_ANY_CHANGE
//      Notification occurs when a Domain change occurs
// 
// The filter values correspond to the bitnumber in filter bitmaps, and
// index number in filter DeltaToken arrays.  Note that these are not the
// bit values themselves.  The MLU_CCN_FILTER_BIT macro can be used to 
// convert from a bit number to the bit value.
//
// When filters are added or removed, the MLUSL_CCN_REVISION value in 
// MluSupport.h must be updated.  Also, the MluSLCCNGetFilterName() function in
// MLUSupportUser.cpp should be updated so that the filter name array matches
// the definitions below.
//
// NOTE: Filter sets are represented as bitmaps throughout the code.  The
// bitmap is stored in a ULONG64 (MLU_CCN_FILTER_BITMAP), so there can never
// be more than 64 filters defined.
//--
typedef enum _MLU_CCN_FILTERS
{
    MLU_CCN_FILTER_POOL_ANY_CHANGE,       // 0
    MLU_CCN_FILTER_FSL_ANY_CHANGE,        // 1
    MLU_CCN_FILTER_DOMAIN_ANY_CHANGE,     // 2
    // Insert new filters here
    MLU_CCN_FILTER_TEST_EXCEPTION,        // Used with test exception MLU_CCN_TEST_130
    MLU_CCN_FILTER_INVALID_INDEX
} MLU_CCN_FILTERS;

#define MLU_NUM_CCN_FILTERS               MLU_CCN_FILTER_INVALID_INDEX

#define MLU_CCN_FILTER_BIT(X)             (ULONG32)(1 << ((ULONG32)(X)))
#define MLU_CCN_VALID_FILTERS_BITMASK     (MLU_CCN_FILTER_BIT(MLU_CCN_FILTER_INVALID_INDEX) - 1)


//++
//  Name:
//      MLU_DELTA_TOKEN
//
//  Description:
//      A unique delta token
//
//--
typedef ULONG32                                 MLU_DELTA_TOKEN, *PMLU_DELTA_TOKEN;


//++
//  Name:
//      MLU_CCN_FILTER_BITMAP
//
//  Description:
//      Bitmap used to describe a set of CCN filters.  The MLU_CCN_FILTER_BIT
//      macro can be used to translate from a filter number to a bit in a 
//      bitmap.
//
//--
typedef ULONG64                                 MLU_CCN_FILTER_BITMAP, *PMLU_CCN_FILTER_BITMAP;


//++
//  Name:
//      MLU_CCN_ID
//
//  Description:
//      Value used to identify one or more CCN registration requests.  It
//      provides the user-level client a way to cancel outstanding registration
//      requests that were started using a particular ID value.
//
//--
typedef ULONG64                                 MLU_CCN_ID, *PMLU_CCN_ID;
#define MLU_CCN_INVALID_ID                      0

//=============================================================================
//  Name:
//      MLU_THIN_LU_NICE_NAME_LENGTH
//
//  Description:
//      Maximum length for NiceName of a LU.
//
//=============================================================================
#define MLU_THIN_LU_NICE_NAME_LENGTH            256

//=============================================================================
//  Name:
//      MLU_THIN_LU_NICE_NAME
//
//  Description:
//      A wide char string that contains the NiceName for a LU.
//
//=============================================================================
typedef WCHAR                                   MLU_THIN_LU_NICE_NAME[ MLU_THIN_LU_NICE_NAME_LENGTH ];

//=============================================================================
//
//  Name:
//      MLU_MAX_VVOL_UUID_STRING_LENGTH
//
//  Description:
//      Maximum length for a UUID.
//
//=============================================================================
#define MLU_MAX_VVOL_UUID_STRING_LENGTH        45

//=============================================================================
//
//  Name:
//      MLU_MAX_VVOL_UUID_STRING
//
//  Description:
//      A UTF8-encoded string that contains the UUID if one of the following forms:
//      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//      xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
//      rfc4122.xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
//      naa.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//      eui.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//
//=============================================================================
typedef CHAR                                    MLU_MAX_VVOL_UUID_STRING[ MLU_MAX_VVOL_UUID_STRING_LENGTH ];

//=============================================================================
//
//  Name:
//      MLU_VVOL_MAX_BIND_COUNT
//
//  Description:
//      Defines the max number of VVols in a single bind request
//
//=============================================================================
#define MLU_VVOL_MAX_BIND_COUNT             64


//++
//  Name:
//      MLU_OBJECT_ID_EXTERNAL
//
//  Description:
//      The oid that is exported outside of the ObjMgr.
//
//--
typedef ULONG64                                 MLU_OBJECT_ID_EXTERNAL, *PMLU_OBJECT_ID_EXTERNAL;

#endif  // _MLU_GENERAL_TYPES_H_
