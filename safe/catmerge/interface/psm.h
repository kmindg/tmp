//*****************************************************************************
// Copyright (C) EMC Corp. 1989-2004
// All rights reserved.
// Licensed material -- property of EMC Corp.
//****************************************************************************/

#ifndef _PSM_H_
#define _PSM_H_

//++
// File:
//      Psm.h
//
// Description:
//      This file contains all of the necessary information to utilize the
//      device control interface presented by the Persistent Storage
//      Manager.
//
//      NOTE: ntddk.h is a required include from kernel space.
//            wtypes.h is a required include from user space.
//
// Revision History:
//      01-Sep-98    PCooke      Created.
//
//      01-Oct-98    PCooke      Incorporation of code review comments.
//
//      01-Nov-01    CVaidya     Added R/W IOCTL support.
//
//      01-Dec-01    CVaidya     Added support for more data areas.
//
//      01-Apr-03    MAjmera     Added support for PSM list layout command.
//
//      01-Jul-03    MAjmera     Added support for GetCompatMode().
//
//      01-Jul-03    Sri         Added write-once data area support.
//
//--

#include "EmcPAL.h"
//#include <devioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

//++
// Includes
//--

//
// PSM Status Codes generated from the K10PersistenStoraheManagerMessages.mc
//
#include "K10PersistentStorageManagerMessages.h"
#include "EmcUTIL_Time.h"
//typedef csx_s64_t EMCUTIL_SYSTEM_TIME, *PEMCUTIL_SYSTEM_TIME;

//++
//.End Includes
//--

//typedef csx_s64_t EMCUTIL_SYSTEM_TIME, *PEMCUTIL_SYSTEM_TIME;

//++
// Constants
//--

//++
// Name:
//      PSM_NT_CLARIION_DIR
//
// Description:
//      The top-level directory to create the kernel-accessible PSM device
//      hierarchy.
//
//--
#define PSM_NT_CLARIION_DIR            "\\Device\\CLARiiON"
//.End PSM_NT_CLARIION_DIR

//++
// Name:
//      PSM_NT_CONTROL_DIR
//
// Description:
//      The control directory to create the kernel-accessible PSM device
//      hierarchy.
//
//--
#define PSM_NT_CONTROL_DIR             "\\Device\\CLARiiON\\Control"
//.End PSM_NT_CONTROL_DIR

//++
// Name:
//      PSM_NT_DEVICE_NAME
//
// Description:
//      The kernel-accessible PSM device name.
//
//--
#define PSM_NT_DEVICE_NAME  \
    "\\Device\\CLARiiON\\Control\\PersistentStorageManager"

// NOTE: The PSM_NT_DEVICE_NAME should eventually go away as the entire code
//       base is transformed to stop using wide character and Unicode strings
#define PSM_NT_DEVICE_NAME_CHAR "\\Device\\CLARiiON\\Control\\PersistentStorageManager"

//.End PSM_NT_DEVICE_NAME

//++
// Name:
//      PSM_DEVICE_NAME_WCHAR
//
// Description:
//      PSM device name for access from user-space (in Unicode).
//
//--
#define PSM_DEVICE_NAME_WCHAR         L"\\\\.\\CLARiiONpsm"
//.End PSM_DEVICE_NAME_WCHAR

//++
// Name:
//      PSM_DEVICE_NAME_CHAR
//
// Description:
//      PSM device name for access from user-space (in Ascii).
//
//--
#define     PSM_DEVICE_NAME_CHAR      "\\\\.\\CLARiiONpsm"
#define     PSM_DEVICE_NAME                 "CLARiiONpsm"
//.End PSM_DEVICE_NAME_CHAR

//++
// Name:
//      PSM_DEFAULT_CONTAINER
//
// Description:
//      Name of the default PSM container that almost all users will use.
//
//--
#define PSM_DEFAULT_CONTAINER         "psmDefaultContainer"
//.End PSM_DEFAULT_CONTAINER

//++
// Name:
//      PSM_API_VERSION_ONE
//
// Description:
//      The version of the IOCTL structures in legacy PSM.
//
//--
#define PSM_API_VERSION_ONE           0x0001
//.End PSM_API_VERSION_ONE

//++
// Name:
//      PSM_API_VERSION_TWO
//
// Description:
//      The version of the IOCTL structures in enhanced PSM.
//
//--
#define PSM_API_VERSION_TWO           0x0002
//.End PSM_API_VERSION_TWO

//++
// Name:
//      PSM_CURRENT_API_VERSION
//
// Description:
//      The current PSM IOCTL version structures.
//
//--
#define PSM_CURRENT_API_VERSION       PSM_API_VERSION_TWO
//.End PSM_CURRENT_API_VERSION

//++
// Name:
//      PSM_DATA_AREA_NAME_MAX
//
// Description:
//      The following defines the maximum length of data area name in
//      wide(not any more) characters.  This maximum length includes a wide character NULL,
//      so you really have one less character to play with.  This value
//      is arbitrary and has no real dependencies.
//
//--
#define PSM_DATA_AREA_NAME_MAX                    48
//.End PSM_DATA_AREA_NAME_MAX
 
//++
// Name:
//      PSM_CONTAINER_NAME_MAX
//
// Description:
//      The following defines the maximum length of container name in
//      wide(not any more) characters.  This maximum length includes a wide character NULL,
//      so you really have one less character to play with.  This value
//      is arbitrary and has no real dependencies.
//
//--
#define PSM_CONTAINER_NAME_MAX        32
//.End PSM_CONTAINER_NAME_MAX

//++
// Name:
//      PSM_LU_DEVICE_NAME_MAX
//
// Description:
//      The following defines the maximum length of the device name that
//      provides container storage in wide(not any more) characters.  This value is akin
//      to the maximum length that K10 will use to indicate the length
//      of consumable FLARE LU devices.  As of this writing, this was to be
//      32 ASCII characters, so we have some space to play with since we
//      do all of this junk in wide characters.
//
//--

#define PSM_LU_DEVICE_NAME_MAX        32
//.End PSM_LU_DEVICE_NAME_MAX

//++
// Name:
//      PSM_CLOSE_DISCARD_FLAG
//
// Description:
//      Value used while closing a data area that signifies discarding all
//      changes made to the data area.
//
//--
#define PSM_CLOSE_DISCARD_FLAG        FALSE
//.End PSM_CLOSE_DISCARD_FLAG

//++
// Name:
//      PSM_CLOSE_COMMIT_FLAG
//
// Description:
//      Value used while closing a data area that signifies committing all
//      changes made to the data area.
//
//--
#define PSM_CLOSE_COMMIT_FLAG         TRUE
//.End PSM_CLOSE_COMMIT_FLAG

//++
// Name:
//      PSM_SEEK_CURRENT
//
// Description:
//      This constant defines a value for PSM read/write operations that
//      specify the starting position of the operation with no seeking, i.e.,
//      starting from the current location which is the end of the previous
//      operation.
//
//--
#define PSM_SEEK_CURRENT              ((ULONG) -1)
//.End PSM_SEEK_CURRENT

//++
// Name:
//    PSM_DATA_AREA_TIMER_WAIT_FOREVER
//
// Description:
//    The value for the default amount of time that a Client can keep a Data
//    Area open
//
// Requirement:
//      Enhanced Timers - Requirement 3.1.3
//
//--
#define PSM_DATA_AREA_TIMER_WAIT_FOREVER  (0xFFFFFFFF)
//.End PSM_DATA_AREA_TIMER_WAIT_FOREVER

//++
// Name:
//      FILE_DEVICE_PSM
//
// Description:
//      Define the device type value.  Note that values used by Microsoft
//      Corp are in the range 0-32767, and 32768-65535 are reserved for use
//      by customers.
//
//--
#define     FILE_DEVICE_PSM             35000
//.End FILE_DEVICE_PSM

//++
// Name:
//      IOCTL_INDEX_PSM_BASE
//
// Description:
//      Define IOCTL index base.  Note that function codes 0-2047 are reserved
//      for Microsoft Corporation, and 2048-4095 are reserved for customers.
//
//--
#define     IOCTL_INDEX_PSM_BASE        3500
//.End IOCTL_INDEX_PSM_BASE


//++
// Name:
//      PSM_DATA_FLAG
//
// Description:
//      This flag is an input parameter to an open data area request that
//      specifies what action to take on the scratch side (the redundant
//      side) of the data area.
//
// Values:
//      PSM_DATA_CLEAR
//          Specifies the data in scratch side area should be cleared.
//
//      PSM_DATA_RETAIN
//          Specifies the data in the valid side of the data area should
//          be copied to the scratch side.
//
//      PSM_DATA_DO_NOTHING
//          Specifies the data in the scratch side of the data area
//          should be left alone.
//--
typedef enum _PSM_DATA_FLAG
{
    PSM_DATA_CLEAR = 0,
    PSM_DATA_RETAIN = 1,
    PSM_DATA_DO_NOTHING = 2,

} PSM_DATA_FLAG;
//.End PSM_DATA_FLAG

//++
// Name:
//      PSM_CONTENT_TYPE
//
// Description:
//      The types of information that will be returned during a list
//      contents device control operation.
//
typedef enum _PSM_CONTENT_TYPE
{
    PSM_TYPE_CONTAINER,
    PSM_TYPE_DATA_AREA,
    PSM_TYPE_DATA_AREA_LAYOUT

} PSM_CONTENT_TYPE;
//.End PSM_CONTENT_TYPE

//++
// Name:
//      PSM_OPERATION_INDEX
//
// Description:
//      The IOCTL index codes for PSM.  Any new index codes must be added
//      to the end of the list, not anywhere else.
//
//      PSM_OPEN_READ
//          Open a data area for READ access.
//
//      PSM_OPEN_WRITE
//          Open a data area for WRITE access.
//
//      PSM_CLOSE_COMMIT
//          Close a data area, committing all changes.
//
//      PSM_CLOSE_DISCARD
//          Close a data area, discarding all modifications.
//
//      PSM_READ
//          Read from an open data area.
//
//      PSM_WRITE
//          Write to an open data area.
//
//      PSM_DELETE
//          Delete a data area.
//
//      PSM_GET_SIZE
//          Retreive the size of an open data area.
//
//      PSM_FORMAT_SPACE
//          Format the PSM container.
//
//      PSM_LIST_CONTENTS
//          List all PSM data areas.
//
//      PSM_ADD_LU
//          Add a secondary PSM container.
//
//      PSM_REMOVE_LU
//          Remove a secondary PSM container.
//
//      PSM_DEFRAGMENT
//          Defragment the primary PSM container.
//
//      PSM_ADMIN_ENUM_OBJECTS
//          Admin operation to enumerate PSM objects.
//
//      PSM_ADMIN_GET_GENERIC_PROPS
//          Admin operation to retreive object properties.
//
//      PSM_OPEN_READ_WRITE
//          Open a data area for read-write access.
//
//      PSM_ADMIN_PUT_COMPAT_MODE
//          Set PSM in compat mode.
//
//      PSM_LIST_LAYOUT
//          List the layout of PSM data areas.
//
//      PSM_ADMIN_GET_COMPAT_MODE
//          Admin operation to retreive the current PSM compat mode.
//
//      PSM_CHG_VERSION_INFO
//          Used for down-grading the PSM version - TEST ONLY.
//
//      PSM_OPEN_WRITE_ONCE
//          Create a write-once data area.
//
//      PSM_GET_INFO
//          Retreive info about a data area.
//
//      PSM_SIMULATE_IO_ERROR_START
//          Start simulating IO errors from PSM storage - TEST ONLY.
//
//      PSM_SIMULATE_IO_ERROR_STOP
//          Stop simulating PSM IO errors - TEST ONLY.
//
//      PSM_EXPAND
//          Expand an existing data area.
//
//--
typedef enum _PSM_OPERATION_INDEX
{
    PSM_OPEN_READ,
    PSM_OPEN_WRITE,
    PSM_CLOSE_COMMIT,
    PSM_CLOSE_DISCARD,
    PSM_READ,
    PSM_WRITE,
    PSM_DELETE,
    PSM_GET_SIZE,
    PSM_FORMAT_SPACE,
    PSM_LIST_CONTENTS,
    PSM_ADD_LU,
    PSM_REMOVE_LU,
    PSM_DEFRAGMENT,
    PSM_ADMIN_ENUM_OBJECTS,
    PSM_ADMIN_GET_GENERIC_PROPS,
    PSM_OPEN_READ_WRITE,
    PSM_ADMIN_PUT_COMPAT_MODE,
    PSM_LIST_LAYOUT,
    PSM_ADMIN_GET_COMPAT_MODE,
    PSM_CHG_VERSION_INFO,
    PSM_OPEN_WRITE_ONCE,
    PSM_GET_INFO,
    PSM_SIMULATE_IO_ERROR_START,
    PSM_SIMULATE_IO_ERROR_STOP,
    PSM_EXPAND,
    PSM_SIMULATE_IO_TIMEOUT,

    //
    //  For the love of God, please make sure you add new operations here and
    //  not in the middle of the above list (which you should well know would
    //  cause interface revision incompatability problems :-)
    //

} PSM_OPERATION_INDEX;
//.End PSM_OPERATION_INDEX

//++
//.End Constants
//--

//++
// Macros
//--

//++
// Name:
//      PSM_BUILD_IOCTL
//
// Description:
//      Helper macro to build PSM IO control codes.
//
//--
#define PSM_BUILD_IOCTL( _x_ )                     \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_PSM,                     \
             (IOCTL_INDEX_PSM_BASE + _x_),         \
             EMCPAL_IOCTL_METHOD_BUFFERED,                      \
             EMCPAL_IOCTL_FILE_ANY_ACCESS)
//.End PSM_BUILD_IOCTL

//
// IOCTLS - The PSM device driver IOCTLs
//

#define IOCTL_PSM_OPEN_READ                        \
    PSM_BUILD_IOCTL (PSM_OPEN_READ)

#define IOCTL_PSM_OPEN_WRITE                       \
    PSM_BUILD_IOCTL (PSM_OPEN_WRITE)

#define IOCTL_PSM_CLOSE_COMMIT                     \
    PSM_BUILD_IOCTL (PSM_CLOSE_COMMIT)

#define IOCTL_PSM_CLOSE_DISCARD                    \
    PSM_BUILD_IOCTL (PSM_CLOSE_DISCARD)

#define IOCTL_PSM_READ                             \
    PSM_BUILD_IOCTL (PSM_READ)

#define IOCTL_PSM_WRITE                            \
    PSM_BUILD_IOCTL (PSM_WRITE)

#define IOCTL_PSM_DELETE                           \
    PSM_BUILD_IOCTL (PSM_DELETE)

#define IOCTL_PSM_GET_SIZE                         \
    PSM_BUILD_IOCTL (PSM_GET_SIZE)

#define IOCTL_PSM_FORMAT_SPACE                     \
    PSM_BUILD_IOCTL (PSM_FORMAT_SPACE)

#define IOCTL_PSM_LIST_CONTENTS                    \
    PSM_BUILD_IOCTL (PSM_LIST_CONTENTS)

#define IOCTL_PSM_ADD_LU                           \
    PSM_BUILD_IOCTL (PSM_ADD_LU)

#define IOCTL_PSM_REMOVE_LU                        \
    PSM_BUILD_IOCTL (PSM_REMOVE_LU)

#define IOCTL_PSM_DEFRAGMENT                       \
    PSM_BUILD_IOCTL (PSM_DEFRAGMENT)

#define IOCTL_PSM_ADMIN_ENUM_OBJECTS               \
    PSM_BUILD_IOCTL (PSM_ADMIN_ENUM_OBJECTS)

#define IOCTL_PSM_ADMIN_GET_GENERIC_PROPS          \
    PSM_BUILD_IOCTL (PSM_ADMIN_GET_GENERIC_PROPS)

#define IOCTL_PSM_OPEN_READ_WRITE                  \
    PSM_BUILD_IOCTL (PSM_OPEN_READ_WRITE)

#define IOCTL_PSM_ADMIN_PUT_COMPAT_MODE            \
    PSM_BUILD_IOCTL (PSM_ADMIN_PUT_COMPAT_MODE)

#define IOCTL_PSM_LIST_LAYOUT                      \
    PSM_BUILD_IOCTL (PSM_LIST_LAYOUT)

#define IOCTL_PSM_ADMIN_GET_COMPAT_MODE            \
    PSM_BUILD_IOCTL (PSM_ADMIN_GET_COMPAT_MODE)

#define IOCTL_PSM_CHG_VERSION_INFO                 \
    PSM_BUILD_IOCTL (PSM_CHG_VERSION_INFO)

#define IOCTL_PSM_OPEN_WRITE_ONCE                  \
    PSM_BUILD_IOCTL (PSM_OPEN_WRITE_ONCE)

#define IOCTL_PSM_GET_INFO                         \
    PSM_BUILD_IOCTL (PSM_GET_INFO)

#define IOCTL_PSM_SIMULATE_IO_ERROR_START          \
    PSM_BUILD_IOCTL (PSM_SIMULATE_IO_ERROR_START)

#define IOCTL_PSM_SIMULATE_IO_ERROR_STOP           \
    PSM_BUILD_IOCTL (PSM_SIMULATE_IO_ERROR_STOP)

#define IOCTL_PSM_EXPAND                           \
    PSM_BUILD_IOCTL (PSM_EXPAND)

#define IOCTL_PSM_SIMULATE_IO_TIMEOUT              \
    PSM_BUILD_IOCTL (PSM_SIMULATE_IO_TIMEOUT)

//++
//.End Macros
//--

//++
// Types
//--

//++
// Name:
//      PSM_HANDLE
//
// Description:
//      The PSM handle is used to reference a data area once it is opened.
//      A useable value is returned upon the successful open of a data area
//      and should be used for all subsequent access.  The contents of this
//      really should be treated as opaque from the users perspective
//      and they should not mess with the contents.
//
//--
typedef struct  _PSM_HANDLE
{
    UCHAR       DoNotUseThisField [sizeof (ULONG) * 2];

} PSM_HANDLE, *PPSM_HANDLE;
//.End PSM_HANDLE

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
//
// IOCTL Buffers - The IOCTL input and output buffers that are used
// to interface with PSM.
//

//++
// Name:
//      PSM_OPEN_IN_BUFFER
//
// Description:
//      The PSM open input buffer is used in conjunction with
//      an IOCTL_PSM_OPEN_[READ|WRITE] control code to gain access to a
//      PSM data area.  A currently open data area can not be opened
//      a second (or even third) time.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The container name where the data area to be opened resides.  This
//          always is PSM_DEFAULT_CONTAINER to access data areas on the
//          default PSM LU.
//
//      DataAreaName
//          The name of the data area to to be opened.
//
//      DataAreaSize
//          The size in bytes with which the data area will be created
//          if it is opened for write and it does not already exist.
//          This size is auto-aligned to 512 byte (or whatever
//          PSM_EXTENT_SIZE from the internal header file is set to)
//          increments for the caller if they do not specify the size in
//          512 byte extents.  Although the internal data structures are
//          all 64-bit, the 32 bit quantity expressed here is the throtle
//          to the size of a data area.  If anyone really needs all 64-bits,
//          we'll have to get scared and ask them why!
//
//      DataFlag
//          A flag that indicates what to do with the contents of the
//          data area on an open write.
//          - PSM_DATA_CLEAR value will cause the contents of the new data
//            half to be set to all zeros.
//          - PSM_DATA_RETAIN value will cause the contents of the
//            current valid half to be copied to the other data half that will
//            be written to.
//          - PSM_DATA_DO_NOTHING value will leave whatever data is in the
//            opened data area half alone and it is up to the caller to ensure
//            that everything is handled as it needs to be.
//
//          For performance critical operations one could argue against
//          using PSM, but this last flag will give a caller the best open
//          write performance that PSM can deliver.  This flag is ignored when
//          an open read or open read_write is specified.
//
//      TimeoutInMinutes (Version 2 only)
//          The longest period of time that client expects to hold this
//          Data Area Open. If this time is exceeded, the SP will panic.
//          (Enhanced Timers - Requirement 3.1.4).
//
//--
typedef struct _PSM_OPEN_IN_BUFFER_ONE
{
    ULONG       RevisionID;

    csx_char_t       ContainerName [PSM_CONTAINER_NAME_MAX];

    csx_char_t       DataAreaName [PSM_DATA_AREA_NAME_MAX];

    ULONG       DataAreaSize;

    UCHAR       DataFlag;

} PSM_OPEN_IN_BUFFER_ONE, *PPSM_OPEN_IN_BUFFER_ONE;

typedef struct _PSM_OPEN_IN_BUFFER_TWO
{
    ULONG       RevisionID;

    csx_char_t        ContainerName [PSM_CONTAINER_NAME_MAX];

    csx_char_t       DataAreaName [PSM_DATA_AREA_NAME_MAX];

    ULONG       DataAreaSize;

    UCHAR       DataFlag;

    ULONG       TimeoutInMinutes;

} PSM_OPEN_IN_BUFFER_TWO, *PPSM_OPEN_IN_BUFFER_TWO;
//.End PSM_OPEN_IN_BUFFER

//
// By default, use the newest version
//
#define _PSM_OPEN_IN_BUFFER _PSM_OPEN_IN_BUFFER_TWO
#define PSM_OPEN_IN_BUFFER PSM_OPEN_IN_BUFFER_TWO
#define PPSM_OPEN_IN_BUFFER PPSM_OPEN_IN_BUFFER_TWO

//++
// Name:
//      PSM_OPEN_OUT_BUFFER
//
// Description:
//      The PSM open output buffer is returned upon the successful open
//      of a data area.
//
//      Handle
//          A PSM handle that should be used on subsequent data area operations.
//
//--
typedef struct _PSM_OPEN_OUT_BUFFER
{
    PSM_HANDLE  Handle;

} PSM_OPEN_OUT_BUFFER, *PPSM_OPEN_OUT_BUFFER;
//.End PSM_OPEN_OUT_BUFFER

//++
// Name:
//      PSM_CLOSE_IN_BUFFER
//
// Description:
//      The PSM close input buffer is used in conjunction with the
//      IOCTL_PSM_CLOSE_[COMMIT|DISCARD] control code to finish access
//      to a PSM data area.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      Handle
//          A PSM handle previously returned from a successful
//          IOCTL_PSM_OPEN_* operation.
//
typedef struct _PSM_CLOSE_IN_BUFFER
{
    ULONG       RevisionID;

    PSM_HANDLE  Handle;

} PSM_CLOSE_IN_BUFFER, *PPSM_CLOSE_IN_BUFFER;
//.End PSM_CLOSE_IN_BUFFER


//++
// Name:
//      PSM_READ_IN_BUFFER
//
// Description:
//      The PSM read input buffer is used in conjunction with a
//      IOCTL_PSM_READ control code to retrieve bytes from an opened data area.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      Handle
//          A PSM handle previously returned from a successful
//          IOCTL_PSM_OPEN_* operation.
//
//      Position
//          Used to 'seek' to a position within the data area to begin the
//          read operation.  Users should specify PSM_READ_CURRENT to use the
//          current position and not seek.
//
//      Length
//          The number of bytes to read in.  The size of the output buffer
//          should be at least 'sizeof(PSM_READ_OUT_BUFFER)' plus this
//          value minus one byte.    Although this is an unsigned long field,
//          the IOCTL interface limits the actual maximum value of
//          this field to 2147483643 (INT_MAX + 1 - sizeof
//          (PSM_READ_OUT_BUFFER)) due to the precision of the input buffer
//          variable chosen.  Most likely, this will not really be of any
//          real consequence to anybody but is pointed out for completeness.
//
typedef struct _PSM_READ_IN_BUFFER
{
    ULONG       RevisionID;

    PSM_HANDLE  Handle;

    ULONG       Position;

    ULONG       Length;

} PSM_READ_IN_BUFFER, *PPSM_READ_IN_BUFFER;
//.End PSM_READ_IN_BUFFER

//++
// Name:
//      PSM_READ_OUT_BUFFER
//
// Description:
//      The PSM read output buffer is used in conjunction with an
//      IOCTL_PSM_READ control code to retrieve bytes from an opened data area.
//      The number of bytes to read in is specified within the input buffer.
//      The size of the output buffer should be at least
//      'sizeof (PSM_READ_OUT_BUFFER)' plus the size specified in the input
//      buffer minus one byte.
//
//      BytesRead
//          The number of bytes read in.  This number can be less than the
//          bytes requested if we reached the end of the data area while
//          reading in bytes.
//
//      Buffer
//          A place holder to access the returned data buffer.
//
//--
typedef struct _PSM_READ_OUT_BUFFER
{
    ULONG       BytesRead;

    CHAR        Buffer [1];

} PSM_READ_OUT_BUFFER, *PPSM_READ_OUT_BUFFER;
//.End PSM_READ_OUT_BUFFER


//++
// Name:
//      PSM_WRITE_IN_BUFFER
//
// Description:
//      The PSM write input buffer is used in conjunction with an
//      IOCTL_PSM_WRITE control code to deposit bytes into an opened data area.
//      The number of bytes to write in is specified within this input buffer.
//      The size of the input buffer should be at least
//      'sizeof (PSM_WRITE_IN_BUFFER)' plus the length specified minus one byte.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      Handle
//          A PSM handle previously returned from a successful
//          IOCTL_PSM_OPEN_* operation.
//
//      Position
//          Used to 'seek' to a position within the data area to begin the
//          write operation.  Users should specify PSM_WRITE_CURRENT to use
//          the current position and not seek.
//
//      Length
//          The number of bytes to deposit into the data area starting from the
//          'Buffer' element of the structure.  Although this is an unsigned
//          long field, the IOCTL interface limits the actual maximum value
//          of this field to 2147483627 (INT_MAX + 1 - sizeof
//          (PSM_WRITE_IN_BUFFER)) due to the precision of the input
//          buffer variable chosen.  Most likely, this will not really be
//          of any real consequence to anybody but is pointed out
//          for completeness.
//
//      Buffer
//          A place holder to access the write data buffer.
//
//--
typedef struct _PSM_WRITE_IN_BUFFER
{
    ULONG       RevisionID;

    PSM_HANDLE  Handle;

    ULONG       Position;

    ULONG       Length;

    CHAR        Buffer [1];

} PSM_WRITE_IN_BUFFER, *PPSM_WRITE_IN_BUFFER;
//.End PSM_WRITE_IN_BUFFER


//++
// Name:
//      PSM_WRITE_OUTPUT_BUFFER
//
// Description:
//      The PSM write output buffer is used in conjunction with an
//      IOCTL_PSM_WRITE control code to return to the caller the number
//      of bytes written.
//
//      BytesWritten
//          The number of bytes written.  This could be less than the number
//          specified if we reached the end of the data area.
//
//--
typedef struct _PSM_WRITE_OUT_BUFFER
{
    ULONG       BytesWritten;

} PSM_WRITE_OUT_BUFFER, *PPSM_WRITE_OUT_BUFFER;
//.End PSM_WRITE_OUT_BUFFER

//++
// Name:
//      PSM_DELETE_IN_BUFFER
//
// Description:
//      The PSM delete input buffer is used in conjunction with an
//      IOCTL_PSM_DELETE control code to remove a data area.  A currently
//      opened data area can not be deleted.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The container name where the data area to be deleted resides.
//          PSM_DEFAULT_CONTAINER should be copied here to delete data areas
//          on the default PSM LU.
//
//      DataAreaName
//          The name of the data area to to be deleted.
//
//--
typedef struct _PSM_DELETE_IN_BUFFER
{
    ULONG       RevisionID;

    csx_char_t       ContainerName [PSM_CONTAINER_NAME_MAX];

    csx_char_t       DataAreaName [PSM_DATA_AREA_NAME_MAX];

} PSM_DELETE_IN_BUFFER, *PPSM_DELETE_IN_BUFFER;
//.End PSM_DELETE_IN_BUFFER


//++
// Name:
//      PSM_GET_SIZE_IN_BUFFER
//
// Description:
//      The PSM get size input buffer is used in conjunction with an
//      IOCTL_PSM_GET_SIZE control code to retrieve the size of a PSM data
//      area.  This size is the size accessible to the user.  Actual storage
//      will be twice that specified.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      Handle
//          A PSM handle previously returned from a successful
//          IOCTL_PSM_OPEN_* operation.
//
//--
typedef struct _PSM_GET_SIZE_IN_BUFFER
{
    ULONG       RevisionID;

    PSM_HANDLE  Handle;

} PSM_GET_SIZE_IN_BUFFER, *PPSM_GET_SIZE_IN_BUFFER;
//.End PSM_GET_SIZE_IN_BUFFER

//++
// Name:
//      PSM_GET_SIZE_OUT_BUFFER
//
// Description:
//      The PSM get size output buffer is used in conjunction with an
//      IOCTL_PSM_GET_SIZE control code to return the size of the specified
//      data area.
//
//      Size
//          The number of bytes utilized by the data area.
//
//--
typedef struct _PSM_GET_SIZE_OUT_BUFFER
{
    ULONG       Size;

} PSM_GET_SIZE_OUT_BUFFER, *PPSM_GET_SIZE_OUT_BUFFER;
//.End PSM_GET_SIZE_OUT_BUFFER

//++
// Name:
//      PSM_GET_INFO_IN_BUFFER
//
// Description:
//        The PSM get info input buffer is used in conjunction with an
//        IOCTL_PSM_GET_INFO control code to retrieve all info of a of a PSM
//        data area.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      Handle
//          A PSM handle previously returned from a successful IOCTL_PSM_OPEN_*
//          operation.
//
//--
typedef struct _PSM_GET_INFO_IN_BUFFER
{
    ULONG       RevisionID;

    PSM_HANDLE  Handle;

} PSM_GET_INFO_IN_BUFFER, *PPSM_GET_INFO_IN_BUFFER;
//.End PSM_GET_INFO_IN_BUFFER

//++
// Name:
//      PSM_GET_INFO_OUT_BUFFER
//
// Description:
//      The PSM get info output buffer is used in conjunction with a
//      IOCTL_PSM_GET_INFO control code to return info about the specified data
//      area.
//
//      offsetOne
//          The starting logical disk offset for side 0 of the data area.
//
//      offsetTwo
//          The starting logical disk offset for side 1 of the data area.
//
//      currentPosition
//          The data area's current seek position.
//
//      logicalSize
//          The portion of the data area that contains valid information, i.e,
//          the last byte offset within the data area that was written to.
//
//      length
//          The size of the data area (rounded up to a multiple of
//          PSM_EXTENT_SIZE).
//
//      accessMode
//          The access-mode with which the data area was opened (RD or WR or
//          RD/WR).
//
//      openFirstTime
//          TRUE if the current open command created the data area;
//          FALSE otherwise.
//
//      openWriteOnce
//          Set to TRUE if the write-once flag was specified at the time of
//          open.
//
//      lastPosition
//          The logical position that of the last write.
//
//--
typedef struct _PSM_GET_INFO_OUT_BUFFER
{
    EMCPAL_LARGE_INTEGER      offsetOne;

    EMCPAL_LARGE_INTEGER      offsetTwo;

    EMCPAL_LARGE_INTEGER      currentPosition;

    EMCPAL_LARGE_INTEGER      logicalSize;

    ULONG              length;

    CHAR               accessMode;

    BOOLEAN            openFirstTime;

    BOOLEAN            openWriteOnce;

    EMCPAL_LARGE_INTEGER      lastPosition;

} PSM_GET_INFO_OUT_BUFFER, *PPSM_GET_INFO_OUT_BUFFER;
//.End PSM_GET_INFO_OUT_BUFFER

//++
// Name:
//      PSM_EXPAND_IN_BUFFER
//
// Description:
//      The PSM expand buffer is used in conjunction with an
//      IOCTL_PSM_EXPAND control code to expand an existing data area.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The name of the container.  The user should copy into this,
///         PSM_DEFAULT_CONTAINER, to expand a data area on the
//          the default PSM container.
//
//      DataAreaName
//          Name of the data area to be expanded.
//
//      NewSize
//          The requested size of the data area after expanding.
//
//      CurrentSize
//          The current size of the data area for validation.  It is only
//          used if the check size flag is set.
//
//      CurrentSizeCheck
//          A flag to validate the pre-expansion size of the data area.
//
//--
typedef struct _PSM_EXPAND_IN_BUFFER
{
    ULONG       RevisionID;

    csx_char_t       ContainerName [PSM_CONTAINER_NAME_MAX];

    csx_char_t      DataAreaName [PSM_DATA_AREA_NAME_MAX];

    ULONG       NewSize;

    ULONG       CurrentSize;

    BOOLEAN     CurrentSizeCheck;

} PSM_EXPAND_IN_BUFFER, *PPSM_EXPAND_IN_BUFFER;
//.End PSM_EXPAND_IN_BUFFER

//++
// Name:
//      PSM_EXPAND_OUT_BUFFER
//
// Description:
//      The PSM expand output buffer is used in conjunction with a
//      IOCTL_PSM_EXPAND control code to return the size of the data area after
//      the expand operation.
//
//      CurrentSize
//          The size of the data area after the expand operation.
//
//--
typedef struct _PSM_EXPAND_OUT_BUFFER
{
    ULONG       CurrentSize;

} PSM_EXPAND_OUT_BUFFER, *PPSM_EXPAND_OUT_BUFFER;
//.End PSM_EXPAND_OUT_BUFFER

//++
// Name:
//      PSM_SIMULATE_IO_ERROR_IN_BUFFER
//
// Description:
//      The PSM simulate IO error input buffer is used in conjunction with a
//      IOCTL_PSM_SIMULATE_IO_ERROR_START control code to start simulating IO
//      errors in PSM.
//
//      NOTE:  This is a DEBUG only IOCTL, meant to be used for testing
//             purposes only.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      SuccessCount
//          Number of IOs to 'pass-thru' to flare before simulating an error.
//
//      FailureCount
//          Number of consecutive IOs for which an error is to be
//          generated in PSM.
//
//--
typedef struct _PSM_SIMULATE_IO_ERROR_IN_BUFFER
{
    ULONG       RevisionID;

    ULONG       SuccessCount;

    ULONG       FailureCount;

} PSM_SIMULATE_IO_ERROR_IN_BUFFER, *PPSM_SIMULATE_IO_ERROR_IN_BUFFER;
//.End PSM_SIMULATE_IO_ERROR_IN_BUFFER


//++
// Name:
//      PSM_FORMAT_IN_BUFFER
//
// Description:
//      The PSM format input buffer is used in conjunction with an
//      IOCTL_PSM_FORMAT control code to (re-)format an LU for utilization
//      by PSM.  This would typically only be used after extending an LU
//      that PSM is already utilizing.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The container name where to format.  PSM_DEFAULT_CONTAINER
//          should be copied here reformat the default PSM LU.
//
//--
typedef struct _PSM_FORMAT_IN_BUFFER
{
    ULONG       RevisionID;

    csx_char_t       ContainerName [PSM_CONTAINER_NAME_MAX];

} PSM_FORMAT_IN_BUFFER, *PPSM_FORMAT_IN_BUFFER;
//.End PSM_FORMAT_IN_BUFFER


//++
// Name:
//      PSM_FORMAT_OUT_BUFFER
//
// Description:
//      The PSM format output buffer is used in conjunction with an
//      IOCTL_PSM_FORMAT control code to return the total number of bytes
//      available on the formatted LU.  Space available to data areas
//      will be less than this amount due to PSM structures.
//
//      ContainerSize
//          The total number of bytes on the formatted LU.
//
//--
typedef struct _PSM_FORMAT_OUT_BUFFER
{
    EMCPAL_LARGE_INTEGER   ContainerSize;

} PSM_FORMAT_OUT_BUFFER, *PPSM_FORMAT_OUT_BUFFER;
//.End PSM_FORMAT_OUT_BUFFER


//++
// Name:
//      PSM_ADD_LU_IN_BUFFER
//
// Description:
//      The PSM add LU input buffer is used in conjunction with an
//      IOCTL_PSM_ADD_LU control code to add a secondary LU for
//      PSM utilization.
//
//      WARNING!!!  This functionality is not implemented completely.  Using
//                  this IOCTL will result in undefined behavior (probably
//                  unsavory).
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The container name which users will use on attempts to
//          utilize the new LU storage.
//
//      LogicalUnitName
//          The actual NT device name that PSM should open to provide the
//          secondary storage.
//
//      RetainDataFlag
//          Flag to indicate whether to retain information on the
//          secondary LU or not.  If the flag is TRUE and the LU to be
//          added has correct PSM formatting information, the LU
//          will be added with its previous contents left intact;
//          otherwise, the LU will be formatted and all previous contents
//          (if any) will be lost.
//
//
//--
typedef struct _PSM_ADD_LU_IN_BUFFER
{
    ULONG           RevisionID;

    csx_char_t           ContainerName [PSM_CONTAINER_NAME_MAX];

    csx_char_t           LogicalUnitName [PSM_LU_DEVICE_NAME_MAX];

    BOOLEAN         RetainDataFlag;

} PSM_ADD_LU_IN_BUFFER, *PPSM_ADD_LU_IN_BUFFER;
//.End PSM_ADD_LU_IN_BUFFER


//++
// Name:
//      PSM_REMOVE_LU_IN_BUFFER
//
// Description:
//      The PSM remove LU input buffer is used in conjunction with an
//      IOCTL_PSM_REMOVE_LU control code to remove a secondary LU from
//      PSM utilization.
//
//      WARNING!!!  This functionality is not implemented completely.  Using
//                  this IOCTL will result in undefined behavior (probably
//                  unsavory).
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      ContainerName
//          The container name which users use to access the container.
//
//      ForceRemovalFlag
//          Flag to indicate whether to forceably remove the secondary LU
//          or not.  A container that has no open data areas is always removed.
//          If there are open data areas on the container and the flag is TRUE,
//          the container is removed and the open data areas are closed
//          (leaving user applications with invalid handles).  If there are
//          open data areas on the container and the flag is FALSE,
//          the operation fails.
//
//--
typedef struct _PSM_REMOVE_LU_IN_BUFFER
{
    ULONG       RevisionID;

    csx_char_t       ContainerName [PSM_CONTAINER_NAME_MAX];

    BOOLEAN     ForceRemovalFlag;

} PSM_REMOVE_LU_IN_BUFFER, *PPSM_REMOVE_LU_IN_BUFFER;
//.PSM_REMOVE_LU_IN_BUFFER


//+
// Name:
//      PSM_LIST_CONTENTS_IN_BUFFER
//
// Description:
//      The PSM list contents in buffer is used in conjunction with an
//      IOCTL_PSM_LIST_CONTENTS control code to retrieve the contents
//      of all PSM containers.
//
//      RevisionID
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//--
typedef struct _PSM_LIST_CONTENTS_IN_BUFFER
{
    ULONG       RevisionID;

} PSM_LIST_CONTENTS_IN_BUFFER, *PPSM_LIST_CONTENTS_IN_BUFFER;
//.End PSM_LIST_CONTENTS_IN_BUFFER


//++
// Name:
//      PSM_LIST_CONTENTS_NODE
//
// Description:
//      The PSM list contents node is used to return the contents of PSM.
//
//      Type
//          The type of the PSM entry returned.
//
//      ContainerName
//          The user referenced name for a secondary LU when the type is
//          PSM_TYPE_CONTAINER. e.g. 'RemoteMirroringLU'
//
//      DataAreaEntries
//          The number of data areas allocated within the container.
//
//      TotalSpace
//          The number of bytes in the total size of the container.
//
//      ReservedSpace
//          The number of bytes reserved within the container
//
//      FreeSpace
//          The number of bytes available within the container.
//          The free space plus the reserved space should equal the total space.
//
//      DataAreaName
//          The user referenced name for a data area when the type is
//          PSM_TYPE_DATA_AREA. e.g. 'RemoteMirror1'
//
//      Size
//          The number of bytes user accessible bytes for the data area.
//          The actual number of bytes allocated for the data area will be
//          2x this value.
//
//      ModifiedTime
//          The number of 100-nanosecond intervals since January 1, 1601.
//
//      Locked (added for Version 2)
//          Is the Data Area Locked (0 = Unlocked, Non-Zero = Locked)
//          on this SP?
//
//      DataAreaName
//          The user referenced name for a data area when the type is
//          PSM_TYPE_DATA_AREA. e.g. 'RemoteMirror1'
//
//      Size
//          The number of bytes user accessible bytes for the data area.
//          The actual number of bytes allocated for the data area will be
//          2x this value.
//
//      Offset
//          The LBA of the dataarea copy on the PSM LU
//
//--
typedef struct _PSM_LIST_CONTAINER
{
    csx_char_t                  ContainerName [PSM_CONTAINER_NAME_MAX];
    ULONG                  DataAreaEntries;
    EMCPAL_LARGE_INTEGER   TotalSpace;
    EMCPAL_LARGE_INTEGER   ReservedSpace;
    EMCPAL_LARGE_INTEGER   FreeSpace;
} PSM_LIST_CONTAINER, *PPSM_LIST_CONTAINER;

typedef struct _PSM_LIST_DATA_AREA_VERSION_ONE
{
    csx_char_t                 DataAreaName [PSM_DATA_AREA_NAME_MAX];
    EMCUTIL_SYSTEM_TIME   ModifiedTime;
    ULONG                 Size;
} PSM_LIST_DATA_AREA_VERSION_ONE;

typedef struct _PSM_LIST_DATA_AREA_VERSION_TWO
{
    csx_char_t                 DataAreaName [PSM_DATA_AREA_NAME_MAX];
    EMCUTIL_SYSTEM_TIME   ModifiedTime;
    ULONG                 Size;
    ULONG                 Locked;
} PSM_LIST_DATA_AREA_VERSION_TWO;

typedef struct _PSM_LIST_DATA_AREA_LAYOUT
{
    csx_char_t                 DataAreaName [PSM_DATA_AREA_NAME_MAX];
    EMCPAL_LARGE_INTEGER  Offset;
    ULONG                 Size;
} PSM_LIST_DATA_AREA_LAYOUT, *PPSM_LIST_DATA_AREA_LAYOUT;

typedef struct _PSM_LIST_CONTENTS_NODE_VERSION_ONE
{
    PSM_CONTENT_TYPE    Type;

    union
    {
        PSM_LIST_CONTAINER                Container;
        PSM_LIST_DATA_AREA_VERSION_ONE    DataArea;
        PSM_LIST_DATA_AREA_LAYOUT         DataAreaLayout;
    };

} PSM_LIST_CONTENTS_NODE_VERSION_ONE, *PPSM_LIST_CONTENTS_NODE_VERSION_ONE;

typedef struct _PSM_LIST_CONTENTS_NODE_VERSION_TWO
{
    PSM_CONTENT_TYPE    Type;

    union
    {
        PSM_LIST_CONTAINER                Container;
        PSM_LIST_DATA_AREA_VERSION_TWO    DataArea;
        PSM_LIST_DATA_AREA_LAYOUT         DataAreaLayout;
    };
    
} PSM_LIST_CONTENTS_NODE_VERSION_TWO, *PPSM_LIST_CONTENTS_NODE_VERSION_TWO;
//.End PSM_LIST_CONTENTS_NODE

//++
// Name:
//      PSM_LIST_CONTENTS_OUT_BUFFER
//
// Description:
//      The PSM list contents output buffer is used in conjunction with an
//      IOCTL_PSM_LIST_CONTENTS control code to retrieve the contents of PSM.
//
//      NodesRequired
//          The number of nodes required to retrieve the complete contents
//          listing of PSM.
//
//      NodesReturned
//          The nodes returned from the list contents device control operation.
//
//      Contents
//          A place holder to access the returned list content nodes.
//
//      NOTE: Version 2 uses a Version 2 contents Node
//
//--
typedef struct _PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_ONE
{
    ULONG                               NodesRequired;

    ULONG                               NodesReturned;

    PSM_LIST_CONTENTS_NODE_VERSION_ONE  Contents [1];

} PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_ONE,
  *PPSM_LIST_CONTENTS_OUT_BUFFER_VERSION_ONE;

typedef struct _PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO
{
    ULONG                               NodesRequired;

    ULONG                               NodesReturned;

    PSM_LIST_CONTENTS_NODE_VERSION_TWO  Contents [1];

} PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO,
  *PPSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO;
//.End PSM_LIST_CONTENTS_OUT_BUFFER

//
// Use the newest version by default
//
#define PSM_LIST_CONTENTS_NODE                \
    PSM_LIST_CONTENTS_NODE_VERSION_TWO

#define PPSM_LIST_CONTENTS_NODE               \
    PPSM_LIST_CONTENTS_NODE_VERSION_TWO

#define _PSM_LIST_CONTENTS_OUT_BUFFER         \
    _PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO

#define PSM_LIST_CONTENTS_OUT_BUFFER          \
    PSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO

#define PPSM_LIST_CONTENTS_OUT_BUFFER         \
    PPSM_LIST_CONTENTS_OUT_BUFFER_VERSION_TWO

//++
// Name:
//      PSM_LIST_LAYOUT_IN_BUFFER
//
// Description:
//      The PSM list layout input buffer is used in conjunction with an
//      IOCTL_PSM_LIST_LAYOUT IO control code to list the layout of data areas
//      on the PSM LU.
//
//      RevisionId
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//--
typedef struct _PSM_LIST_LAYOUT_IN_BUFFER
{
    ULONG           RevisionID;

} PSM_LIST_LAYOUT_IN_BUFFER, *PPSM_LIST_LAYOUT_IN_BUFFER;
//.End PSM_LIST_LAYOUT_IN_BUFFER


//++
// Name:
//      PSM_LIST_LAYOUT_OUT_BUFFER
//
// Description:
//      The PSM list layout output buffer is used in conjunction with an
//      IOCTL_PSM_LIST_LAYOUT IO control code to list the layout of data areas
//      on the PSM LU.
//
//      NodesRequired
//          The number of nodes required to retrieve the complete contents
//          listing of PSM.
//
//      NodesReturned
//          The nodes returned from the list contents device control operation.
//
//      Contents
//          A place holder to access the returned list content nodes.
//
//--
typedef struct _PSM_LIST_LAYOUT_OUT_BUFFER_VERSION_ONE
{
    ULONG                               NodesRequired;

    ULONG                               NodesReturned;

    PSM_LIST_CONTENTS_NODE_VERSION_ONE  Contents [1];

} PSM_LIST_LAYOUT_OUT_BUFFER_VERSION_ONE,
  *PPSM_LIST_LAYOUT_OUT_BUFFER_VERSION_ONE;

typedef struct _PSM_LIST_LAYOUT_OUT_BUFFER_VERSION_TWO
{
    ULONG                               NodesRequired;

    ULONG                               NodesReturned;

    PSM_LIST_CONTENTS_NODE_VERSION_TWO  Contents [1];

} PSM_LIST_LAYOUT_OUT_BUFFER_VERSION_TWO,
  *PPSM_LIST_LAYOUT_OUT_BUFFER_VERSION_TWO;
//
// Use the newest version by default
//
#define PSM_LIST_LAYOUT_OUT_BUFFER \
    PSM_LIST_LAYOUT_OUT_BUFFER_VERSION_TWO

#define PPSM_LIST_LAYOUT_OUT_BUFFER \
    PPSM_LIST_LAYOUT_OUT_BUFFER_VERSION_TWO

//.End PSM_LIST_LAYOUT_OUT_BUFFER

//++
// Name:
//      PSM_PUT_COMPAT_MODE_IN_BUFFER
//
// Description:
//      The put compatibility mode input buffer is used in conjunction with
//      an IOCTL_PSM_PUT_COMPAT_MODE IO control code. The CompatibilityMode
//      is used to tell the PSM driver to commit and start using new features
//      available in the current version of PSM.
//
//      RevisionId
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      CompatibilityMode
//          Mode of the PSM driver to be set : Latest, Old.
//
//--
typedef struct _PSM_PUT_COMPAT_MODE_IN_BUFFER
{
    ULONG           RevisionId;

    ULONG           CompatibilityMode;

} PSM_PUT_COMPAT_MODE_IN_BUFFER, *PPSM_PUT_COMPAT_MODE_IN_BUFFER;
//.End PSM_PUT_COMPAT_MODE_IN_BUFFER


//++
// Name:
//      PSM_GET_COMPAT_MODE_IN_BUFFER
//
// Description:
//      The get compatibility mode input buffer is used in conjunction with
//      an IOCTL_PSM_GET_COMPAT_MODE IO control code to retreive the
//      current PSM compatibility level.
//
//      RevisionId
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//--
typedef struct _PSM_GET_COMPAT_MODE_IN_BUFFER
{
    ULONG           RevisionID;

} PSM_GET_COMPAT_MODE_IN_BUFFER, *PPSM_GET_COMPAT_MODE_IN_BUFFER;
//.End PSM_GET_COMPAT_MODE_IN_BUFER

//++
// Name:
//      PSM_GET_COMPAT_MODE_OUT_BUFFER
//
// Description:
//      The get compatibility mode output buffer is used in conjunction with
//      an IOCTL_PSM_GET_COMPAT_MODE IO control code. The current
//      CompatibilityMode is returned so the caller can verify if PSM
//      compatibility is current, i.e., if PSM has started using the
//      features available in the current version.
//
//      CompatibilityMode
//          Mode of the PSM driver to be set : Latest, Old.
//
//      wLevel
//          Indicates what releases of Base could understand the current
//          format of the data.
//
//--
typedef struct _PSM_GET_COMPAT_MODE_OUT_BUFFER
{

    ULONG           CompatibilityMode;

    USHORT          wLevel;

} PSM_GET_COMPAT_MODE_OUT_BUFFER, *PPSM_GET_COMPAT_MODE_OUT_BUFFER;
//.End PSM_GET_COMPAT_MODE_OUT_BUFFER


//++
// Name:
//      PSM_CHG_VERSION_INFO_IN_BUFFER
//
// Description:
//      The PSM change version info input buffer is used in conjunction with an
//      IOCTL_PSM_CHG_VERSION_INFO control code to change the current PSM
//      version.  What is does is set the PSM version in the superblock to
//      the given version, useful for down-revving PSM.
//
//      NOTE:  This is a debug-only IOCTL and used only for testing purposes.
//
//      RevisionId
//          Revision ID of the API. Users should always set this to
//          PSM_CURRENT_API_VERSION.
//
//      NewVersionNo
//          The revision of PSM to switch to.
//
//--
typedef struct _PSM_CHG_VERSION_INFO_IN_BUFFER
{
    ULONG            RevisionID;

    ULONG            NewVersionNo;

} PSM_CHG_VERSION_INFO_IN_BUFFER, *PPSM_CHG_VERSION_INFO_IN_BUFFER;
//.End PSM_CHG_VERSION_INFO_IN_BUFFER


//++
// Name:
//      PSM_CHG_VERSION_INFO_OUT_BUFFER
//
// Description:
//      The PSM change version info input buffer is used in conjunction with an
//      IOCTL_PSM_CHG_VERSION_INFO control code to retreive the version
//      of PSM before the change.
//
//      NOTE:  This is a debug-only IOCTL and used only for testing purposes.
//
//      OldVersionNo
//          The old version of PSM that existed before the change.
//
//--
typedef struct _PSM_CHG_VERSION_INFO_OUT_BUFFER
{
    ULONG            OldVersionNo;

} PSM_CHG_VERSION_INFO_OUT_BUFFER, *PPSM_CHG_VERSION_INFO_OUT_BUFFER;
//.End PSM_CHG_VERSION_INFO_OUT_BUFFER
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

//++
//.End Types
//--

#ifdef __cplusplus
};
#endif

#endif // _PSM_H_

//++
//.End file Psm.h
//--

