//
//  File:
//
//      ldbg.h
//
//  Description:
//
//      This module contains the external interface for layered feature
//      provider DLLs for ldbg.
//
//
// +-- LDBG_DEVICE added to head by allocateLdbgDevice()
// |                                                             ^  TO TOP OF
// |  +----------------------------+    +---------------------+  |  DEVICE STACK
// |  | LDBG_ROOT                  |    | LDBG_DEVICE         |  |      ^
// |  +----------------------------+    +---------------------+  |      |
// +->|     NewDeviceListHead      |    |     pDeviceListHead | -+------+
//    +----------------------------+    +---------------------+  |
//    |     RootDeviceListHead     | -> |     RootDeviceList  | -+--> TO NEXT 
//    +----------------------------+    +---------------------+  |    FOUNDER
// +->|     PrunedDeviceListHead   |    |     DeviceList      | -+    DEVICE
// |  +----------------------------+    +---------------------+  |
// |                                                             |
// |                                                             v
// |                                                             TO NEXT DEVICE
// +-- LDBG_DEVICE duplicates moved to head as necessary.        IN STACK
//
//
//  History:
//
//     20-August-04   ALT;    Created
//

#ifndef __LDBG_H__
#define __LDBG_H__


#define KDEXT_64BIT
#include "dbgext_csx.h"

# ifdef __cplusplus
extern "C"
{
#include "k10defs.h"
}
#else
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "k10defs.h"
#endif

# ifdef __cplusplus
extern "C" {
# endif

#define OFFSET( type, field ) ( (DWORD)(&((type *)0)->field) )

//
// The following defines the name of the ldbg DLL.
//

#define LDBG_NAME   "ldbg.dll"

//
// The following defines the name of the devstack routine.
//
#define LDBG_DEVSTACK_ROUTINE   "devstack"

//
// The following defines the name of the features routine.
//
#define LDBG_FEATURES_ROUTINE   "features"

//
// The following instructs ldbg to hide the compile time banner. This is used
// if we are initialized by another debug extension (such as fdb.dll).
//

#define LDBG_SILENT_INIT    0xbabe

//
// The following help maintain uniform indentions.
//

#define LDB_SPACER_1ST_LEVEL "    "
#define LDB_SPACER_2ND_LEVEL "        "
#define LDB_SPACER_3RD_LEVEL "            "
#define LDB_SPACER_4TH_LEVEL "                "

//
// The following defines the buffer length required to convert a WWN into
// a string.
//

#define K10_FAR_UTIL_WWN_BUF_LEN                48


//
// The following defines a constant to use in the LDBG_DISPLAY_SUMMARY()
// macro to identify drivers that are non I/O initiator's (don't get quiesce
// call).
//

#define LDBG_DISPLAY_NOT_IO_INITIATOR   CSX_S64_MINUS_ONE

//
// The following defines the maximum value for a ULONG64.
//

//
// The following macro must be used by all clients when asked to display
// a device in the "Summary" mode.
//

#define LDBG_DISPLAY_SUMMARY( _module_name_, \
                              _device_name_, \
                              _open_count_, \
                              _owner_, \
                              _quiesced_, \
                              _dev_ext_ \
                            ) \
{ \
    csx_dbg_ext_print("%-6.6s ", (_module_name_) ); \
    csx_dbg_ext_print("%-45.45s ", (char*)(_device_name_) ); \
    if ( ((LONG)(_open_count_)) >= 0 && (_open_count_) < 100 ) \
        csx_dbg_ext_print("%-2lld    ", (long long)(_open_count_) ); \
    else \
        csx_dbg_ext_print("??    "); \
    csx_dbg_ext_print("%-4.4s  ", (_owner_) ? "Mine" : "Peer" ); \
    if ( ((_quiesced_) == 0) || ((_quiesced_) == 1) ) \
        csx_dbg_ext_print("%-1lld   ", (long long)(_quiesced_) ); \
    else \
        csx_dbg_ext_print("N/A "); \
    csx_dbg_ext_print("0x%-16.016llx", (unsigned long long)(_dev_ext_) ); \
    csx_dbg_ext_print("\n"); \
}

//
// The following defines the string to be used for the top and bottom of the
// device stack.  Clients should use these constants as appropriate when
// invoking allocateLdbgDevice().
//

#define LDBG_DEVICE_TOP_MARKER      "LDBG_DEVICE_STACK_TOP"

#define LDBG_DEVICE_BOTTOM_MARKER   "LDBG_DEVICE_STACK_BOTTOM"

//
// The following defines the reason for the ldbg device allocation.
//

#define LDBG_ALLOCATE_DEVICE_DEFAULT_REASON \
            "LDBG_ALLOCATE_DEVICE_BECAUSE_DEVICE"


#define ldbgGetFieldValue( _Addr_, _Type_, _Field_, _OutValue_ )  \
            if ( CSX_FAILURE(csx_dbg_ext_read_in_field( _Addr_, _Type_, _Field_, sizeof(_OutValue_), &_OutValue_ ) ) ) \
            { csx_dbg_ext_print("ldbgError: Could not get the value of %s in %s at %016llx!\n", _Field_, _Type_, (unsigned long long)_Addr_);}

ULONG
ldbgGetFieldOffset(
    IN  LPCSTR      Type,
    IN  LPCSTR      Field
    );

//
//  Name: LDBG_SEARCH_OBJECT_NUMBER_BASE
//
//  Description:
//      This enumeration defines the various bases supported to interpret
//      user input.
//
//  Members:
//      LdbgSearchObjectNumberBaseInvalid
//          The enumeration lower bounds marker.
//
//      LdbgSearchObjectNumberBase10
//          The user input should be interpreted as base 10.
//
//      LdbgSearchObjectNumberBase16
//          The user input should be interpreted as base 16.
//
//      LdbgSearchObjectNumberBaseMaximum
//          The enumeration upper bounds marker.
//
typedef enum _LDBG_SEARCH_OBJECT_NUMBER_BASE
{
    LdbgSearchObjectNumberBaseInvalid = -1,

    LdbgSearchObjectNumberBase10,

    LdbgSearchObjectNumberBase16,

    LdbgSearchObjectNumberBaseMaximum

} LDBG_SEARCH_OBJECT_NUMBER_BASE, *PLDBG_SEARCH_OBJECT_NUMBER_BASE;


//
// This routine is provided for the client to give the ldbg engine a string to
// search for.  Ldbg determines if there is a match and returns TRUE to the
// client.  If ldbg finds a match, the client must call allocateLdbgDevice().
// If ldbg does not find a match, it returns FALSE.
//
struct _LDBG_ROOT;

BOOLEAN
searchObjectName(
    IN struct _LDBG_ROOT *              pLdbgRoot,
    IN PCHAR                            pObjectName,
    IN PCHAR                            featureReason,
    OUT PCHAR                           reasonString,
    IN PCHAR                            ComponentString
    );

//
// This routine is provided for the client to give the ldbg engine a number to
// search for.  Ldbg determines if there is a match and returns TRUE to the
// client.  If ldbg finds a match, the client must call allocateLdbgDevice().
// If ldbg does not find a match, it returns FALSE.
//

BOOLEAN
searchObjectNumber(
    IN struct _LDBG_ROOT *              pLdbgRoot,
    IN DWORD                            ObjectNumber,
    IN LDBG_SEARCH_OBJECT_NUMBER_BASE   Base,
    IN PCHAR                            featureReason,
    OUT PCHAR                           reasonString,
    IN PCHAR                            ComponentString
    );

//
// This routine is provided for the client to give the ldbg engine an address
// to search for.  Ldbg determines if there is a match and returns TRUE to the
// client.  If ldbg finds a match, the client must call allocateLdbgDevice().
// If ldbg does not find a match, it returns FALSE.
//

BOOLEAN
searchObjectAddress(
    IN struct _LDBG_ROOT *              pLdbgRoot,
    IN ULONG64                          ObjectAddress,
    IN DWORD                            structureSize,
    IN PCHAR                            featureReason,
    OUT PCHAR                           reasonString,
    IN PCHAR                            ComponentString
    );

//
// This routine is provided for for ease of printing a formatted WWID in ldbg 
// layered driver code
//

VOID
ldbgFormatWWID(
    IN PK10_WWID            pWwid,
    OUT PCHAR               buf
    );

//
// This routine is provided for for ease of printing a formatted WWN in ldbg 
// layered driver code
//

VOID
ldbgFormatWWN(
    IN PK10_WWN             pWwn,
    OUT PCHAR               buf
    );


//
// This routine is provided for the client to allocate a ldbg device and
// perform the correct maintainence on the root device.
//
// The Reason field should be LDBG_ALLOCATE_DEVICE_DEFAULT_REASON unless the
// match was found due to something other than a device. 
//

VOID
allocateLdbgDevice(
    IN struct _LDBG_ROOT *              pLdbgRoot,
    IN K10_DVR_OBJECT_NAME              ExportedDevice,
    IN K10_DVR_OBJECT_NAME              ConsumedDevice,
    IN ULONG64                          Context,
    IN PCHAR                            Reason,
    IN PCHAR                            ComponentString
    );

//
// The following routine should be used to display a kernel timestamp to the
// user.  This function does not add any whitespace before or after the
// timestamp.
//

VOID
printTimestamp(
    ULONG64                            Time
    );


//
// The following defines the length of the object name string.
//

#define LDBG_OBJECT_NAME_LENGTH     512


//
//  Name: LDBG_DISPLAY_TYPE      
//
//  Description:
//      This enumeration defines the various display types that a client
//      must support.
//
//  Members:
//      LdbgDisplayInvalid
//          The enumeration lower bounds marker.
//
//      LdbgDisplaySummary
//          Clients are limited to 1 line of output.  Clients MUST use the
//          LDBG_DISPLAY_SUMMARY() macro to display the output.
//
//      LdbgDisplayObject
//          Clients are limited to output pertaining to objects only.
//          For example, mirrors, snaps, etc...
//
//      LdbgDisplayVerbose
//          Clients are not limited in verbosity.
//
//      LdbgDisplayMaximum
//          The enumeration upper bounds marker.
//
typedef enum _LDBG_DISPLAY_TYPE
{
    LdbgDisplayInvalid = -1,

    LdbgDisplaySummary,

    LdbgDisplayObject,

    LdbgDisplayVerbose,

    LdbgDisplayMaximum
} LDBG_DISPLAY_TYPE, *PLDBG_DISPLAY_TYPE;

//
//  Name: LDBG_DEVICE_LOOKUP_ERROR_CODES      
//
//  Description:
//      This enumeration defines the various return codes that a client
//      can return from the device lookup routine.
//
//  Members:
//      LdbgDeviceLookupErrorInvalid
//          The enumeration lower bounds marker.
//
//      LdbgDeviceLookupErrorDeviceNotFound
//          The device could not be located.
//
//      LdbgDeviceLookupErrorDeviceFound
//          The device was located and allocateLdbgDevice() was called.
//
//      LdbgDeviceLookupErrorMaximum
//          The enumeration upper bounds marker.
//
typedef enum _LDBG_DEVICE_LOOKUP_ERROR_CODES
{
    LdbgDeviceLookupErrorInvalid = -1,

    LdbgDeviceLookupErrorDeviceNotFound,

    LdbgDeviceLookupErrorDeviceFound,

    LdbgDeviceLookupErrorMaximum

} LDBG_DEVICE_LOOKUP_ERROR_CODES, *PLDBG_DEVICE_LOOKUP_ERROR_CODES;


//
//  Name: LDBG_DEVICE_LOOKUP_ROUTINE
//      
//
//  Description:
//      The following defines a lookup callback routine for a device.
//
//  Members:
//
//      pLdbgRoot
//          A pointer to an allocated ldbg root object.
//
//  Returns:
//      A member of the LDBG_DEVICE_LOOKUP_ERROR_CODES enumeration.
//
typedef
LDBG_DEVICE_LOOKUP_ERROR_CODES
(*PLDBG_DEVICE_LOOKUP_ROUTINE)(
    IN struct _LDBG_ROOT *pLdbgRoot
    );

//
//  Name: LDBG_DEVICE_DISPLAY_ROUTINE
//      
//
//  Description:
//          The following defines a display callback routine for a device.
//          Clients must abide by the display type.      
//
//  Arguments:
//
//      pLdbgDevice
//          A pointer to a ldbg device to use to display device
//          information.
//
//      DisplayType
//          The type of display that is requested.
//
//  Returns:
//      None.
//
struct _LDBG_DEVICE;

typedef
VOID
(*PLDBG_DEVICE_DISPLAY_ROUTINE)(
    IN struct _LDBG_DEVICE *pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

//
//  Name: LDBG_FEATURE_DISPLAY_ROUTINE
//      
//
//  Description:
//          The following defines a feature display routine for a client.
//
//  Arguments:
//
//      pLdbgRoot - A pointer to the root that has all the devices that
//          the feature supports.
//
//  Returns:
//      None.
//
typedef
VOID
(*PLDBG_FEATURE_DISPLAY_ROUTINE)(
    IN struct _LDBG_ROOT *pLdbgRoot
    );

//
//  Type:   LDBG_REGISTRATION_ENTRY
//
//  Description:
//      This data type definition is used to define debug client routines.
//
//  Members:
//      pDeviceLookupRoutine
//          A pointer to a client device lookup routine.
//
//      pDisplayRoutine
//          A pointer to a client device display routine.
//
//      pFeatureDisplayRoutine
//          A pointer to a client feature display routine.
//
typedef struct _LDBG_REGISTRATION_ENTRY
{
    PLDBG_DEVICE_LOOKUP_ROUTINE         pDeviceLookupRoutine;

    PLDBG_DEVICE_DISPLAY_ROUTINE        pDeviceDisplayRoutine;

    PLDBG_FEATURE_DISPLAY_ROUTINE       pFeatureDisplayRoutine;

} LDBG_REGISTRATION_ENTRY, *PLDBG_REGISTRATION_ENTRY;

//
//  Name: LDBG_DEVICE
//      
//
//  Description:
//      The following defines a generic device.  The client allocates
//      a device for ldbg to use by calling allocateLdbgDevice().  A client
//      is later presented with a device to display to the user.
//
//  Members:
//
//      pDeviceListHead
//          A private pointer to a private field used by ldbg to determine the
//          top of a given stack.
//
//      pRegistrationEntry
//          A private pointer to the registration entry that owns this device.
//
//      DeviceList
//          A private field used by ldbg to link devices together.
//
//      RootDeviceList
//          A private field used by ldbg to link similar devices together.
//
//      ExportedDevice
//          The name of the exported device.  The client may use this field
//          as needed.
//
//      ConsumedDevice
//          The name of the consumed device.  The client may use this field
//          as needed.
//
//      Context
//          A client "hint" field to prevent multiple list searches.  The
//          clients are strongly encouraged to use this field.
//
typedef struct _LDBG_DEVICE
{
    //
    // Ldbg private fields.
    //

    PEMCPAL_LIST_ENTRY                 pDeviceListHead;

    PLDBG_REGISTRATION_ENTRY    pRegistrationEntry;

    EMCPAL_LIST_ENTRY                  DeviceList;

    EMCPAL_LIST_ENTRY                  RootDeviceList;

    CHAR                        Reason[ LDBG_OBJECT_NAME_LENGTH ];

    //
    // Client usable fields.
    //

    K10_DVR_OBJECT_NAME         ExportedDevice;

    K10_DVR_OBJECT_NAME         ConsumedDevice;

    ULONG64                     Context;

} LDBG_DEVICE, *PLDBG_DEVICE;

//
//  Name: LDBG_ROOT
//      
//
//  Description:
//          The following defines a root device used by ldbg to communicate
//          with each client.  Ldbg passes this device in the lookup routine,
//          and the client allocates a new device as appropiate.  Ldbg will
//          pick off the devices and finish initializing them before moving the
//          device to the DeviceList.  Ldbg assumes responsibility for free'ing
//          any memory allocated by the client.
//
//          NOTE: ALL LDBG PROVIDERS SHOULD TREAT THIS STRCUTRE AS OPAQUE.
//
//  Members:
//
//      RootDeviceListHead
//          A private field used by ldbg to link devices together.
//
//      NewDeviceListHead
//          A private field used by ldbg to determine newly created devices.
//
//      PrunedDeviceListHead
//          A private list of devices that have been pruned from the root
//          before display to avoid duplicates.
//
//      Object
//          The private object name that the ldbg engine uses when
//          a client calls searchObjectName().
//
//      ObjectLower
//          A copy of the original object string converted to lower case to allow
//          case insensitive searches.
//
//      ObjectNumberDec
//          The object name the user entered interpreted as a decimal value.
//
//      ObjectNumberHex
//          The object name the user entered interpreted as a hexadecimal value.
//
//      NumberOfSearches
//          The number of times provider DLLs call back into LDBG to make
//          a comparison.
//
//      Debug
//          TRUE if we are in debug mode and need to print out tons of
//          information, FALSE otherwise.
//
//      DiscoverExactMatchesOnly
//          A boolean indicating that exact matches only should be returned
//          from providers.  This increases performance.
//
//      Reason
//          A string that indicates the reason why we are searching for this
//          device in the first place.  This is not the same as the per-device
//          reason for a match.
//
typedef struct _LDBG_ROOT
{
    EMCPAL_LIST_ENTRY                RootDeviceListHead;

    EMCPAL_LIST_ENTRY                NewDeviceListHead;

    EMCPAL_LIST_ENTRY                PrunedDeviceListHead;

    CHAR                      Object[ LDBG_OBJECT_NAME_LENGTH ];

    CHAR                      ObjectLower[ LDBG_OBJECT_NAME_LENGTH ];

    ULONG64                   ObjectNumberDec;

    ULONG64                   ObjectNumberHex;

    DWORD                     NumberOfSearches;

    BOOLEAN                   Debug;

    BOOLEAN                   DiscoverExactMatchesOnly;

    CHAR                      Reason[ LDBG_OBJECT_NAME_LENGTH ];

} LDBG_ROOT, *PLDBG_ROOT;

void
sprintK10DvrObjectName( 
    K10_DVR_OBJECT_NAME                         DvrObjectName,
    CHAR *                                      buffer
);

void
printK10DvrObjectName( 
    K10_DVR_OBJECT_NAME                         DvrObjectName
);

//
// Client function prototypes.
//

//
// Clones exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
clDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
clDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
clFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// CPM exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
cpmDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
cpmDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
cpmFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// Rollback exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
rbDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
rbDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
rbFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// RM exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
rmDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
rmDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
rmFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// Hostside exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
hsDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
hsDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
hsFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// SnapView exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
svDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
svDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
svFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// FAR exported routines that the ldbg engine calls to operation correctly.
//

extern
LDBG_DEVICE_LOOKUP_ERROR_CODES
farDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern
VOID
farDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern
VOID
farFeatureDisplay(
    IN PLDBG_ROOT           pLdbgRoot
    );

//
// AgDb exported routines that the ldbg engine calls to operation correctly.
//
extern VOID agDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );

extern LDBG_DEVICE_LOOKUP_ERROR_CODES agDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );

extern VOID agFeatureDisplay( IN PLDBG_ROOT pLdbgRoot );


//
// MigDb exported routines that the ldbg engine calls to operation correctly.
//
extern VOID migDeviceDisplay(
    IN PLDBG_DEVICE         pLdbgDevice,
    IN LDBG_DISPLAY_TYPE    DisplayType
    );
extern LDBG_DEVICE_LOOKUP_ERROR_CODES migDeviceLookup(
    IN PLDBG_ROOT           pLdbgRoot
    );
extern VOID migFeatureDisplay( IN PLDBG_ROOT pLdbgRoot );

// Used for base layers to know if they are supported or not.
// Basically because at this time, the user needs to load the base layered
// debug libraries.
extern BOOLEAN baseLayersSupported(void);

# ifdef __cplusplus
    };
# endif

#endif // __LDBG_H__
