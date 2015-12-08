// K10Defs.h
//
// Copyright (C) 1998-2010  EMC Corporation
//
//
// Header for global data definitions
//
//  Revision History
//  ----------------
//  10 Sep 98   D. Zeryck   Initial version.
//  14 Sep 98   D. Zeryck   Post Review Version: ifndefs cleaned up
//   9 Dec 98   D. Zeryck   ID lengths changed, device is char
//  17 Dec 98   D. Zeryck   K10_DVR_NAME defined, K10_LU_ID typedef
//  11 Jan 99   D. Zeryck   K10_DEVICE_ID_LEN to 64 chars
//  04 Feb 99   T. Westbom  Added required carriage return to end of file.  8-(
//  18 Feb 99   D. Zeryck   Add K10_SP_ID, array ID
//  17 Mar 99   D. Zeryck   Explicit pointer defs
//  06 Apr 99   D. Zeryck   Integrate Paul M's clarifications
//   8 Sep 99   D. Zeryck   Put Flare object name strings here
//  13 Sep 00   B. Yang     Added MAX_IOCTL_TIMEOUT
//  22 Feb 00   D. Zeryck   Add macros for creating error codes
//   6 Mar 00   H. Weiner   Added severity bit definitions
//   7 Mar 00   D. Zeryck   Mod error creation macro
//  27 Mar 00   J. Cook     Added K10_REGISTRY_GLOBAL.
//  18 May 00   D. Zeryck   CLARIION-->EMC
//   5 Jun 00   B. Yang     Added some constants for Page26&37 extension
//  20 Jun 00   H. Weiner   Added IS_K10_DRIVER_ERROR macro
//  06 Jul 00   B. Yang     Modified IS_K10_DRIVER_ERROR, check bit29 instead of bit 26.
//  11 Aug 00   D. Zeryck   chg 1621 - new category
//   3 Jan 01   H. Weiner   Phase 2:  added K10_BIOS_PATH
//  15 Jun 01   H. Weiner   Add IS_FLARE_ERROR macro
//   7 Jan 01   H. Weiner   Add PROM and DDBS path defines
//  13 May 03   H. Weiner   FLARE_ERROR_USER macro nows includes severity
//  27 Sep 05   R. Hicks    DIMS 130874: support delivering EAST
//  13 Apr 07   M. Khokhar  Modifications to incorporate FC WWN's changes
//  17 Jan 08   A. Bonde    DIMS 188009: Changes in mapping of SP port numbers to FC WWN port numbers
//  03 Sep 09   D. Hesi     Added K10_REGISTRY_USER_PROCESS macro
//  10 May 10   E. Carter   Added Celerra WWNs
//
#ifndef K10_DEFS_H
#define K10_DEFS_H

#include "core_config.h"

#undef FLARE__STAR__H

////++++
//
//  BEGIN Definitions
//
////----

////++
//
//  BEGIN Definitions of macros used to build error from a common base for both NT
//      internal operatoins and for higher level analysis.
//      The Navi team has expressed a wish to keep the upper 16 bits of an error
//      code to contain a flat namespacing for component identification. NT status
//      codes, to be passed up from a driver to a DeviceIoControl call, must adhere
//      to NT's design which uses the upper 16 bits to encode facility (component) but
//      also severity and source type error masks. So we use macros to create values
//      that meet each interface's formatting, ffrom a 16-bit error value
//
////--


/*
 * We build errors from a component-scoped set of 16-bit error codes (errEnum).
 * Used in Admin Library code when an error in input params is detected, or in driver
 * code to encode the error code in the standard K10 logging interface.
 */
#define MAKE_ADMIN_ERROR( errorEnum ) \
    (errorEnum | MY_ERROR_BASE)

#define K10_DVR_UPPER_NIBBLE_MASK 0x70000000

/*
 * Convert an NTSTATUS code to the IK10Admin-compatible 32-bit error code Navi is expecting
 */
// Conversion macro for K10 specific Driver errors
#ifdef ALAMOSA_WINDOWS_ENV
#define MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( driverError ) \
    ((driverError & 0x0FFFFFFF) | K10_DVR_UPPER_NIBBLE_MASK)
#else
#define MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( driverError ) \
    (driverError)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this all seems suspicious */

#ifdef ALAMOSA_WINDOWS_ENV
// Conversion macros for any K10 Driver errors. NT errors are left alone.
#define IS_K10_DRIVER_ERROR( driverError ) \
    (driverError & 0x20000000)
#else
#define IS_K10_DRIVER_ERROR( driverError ) \
    (!(CSX_STATUS_IN_CSX_RANGE(driverError)))
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this all seems suspicious */

//
// Define a common category value for Navi to key its search of NT event log entries on.
// This will probably change to some reasonably unique value.
//
#define K10_EVENT_LOG_CATEGORY  0x4d2

// Kernel drivers have a different data format; allow Navi to parse them differently
//
#define K10_EVENT_LOG_CATEGORY_KNL  0x4d3

////--
//
// END Definitions of macros used to create IK10Admin-compatible error codes
//
////++


////++
//
//  BEGIN Definitions of Flare strings for objects created in NT namespace
//
////--
/*
 * These are the constants for Flare created disk device names.
 * Combining them with the LU number generates the appropriate name, e.g. \Device\CLARiiONdisk0.
 */

#define FLARE_LU_DEVICE_NAME_PREFIX         "CLARiiONdisk"
#define FLARE_LU_DEVICE_PATH_PREFIX         "\\Device\\" FLARE_LU_DEVICE_NAME_PREFIX

/*
 * These are the constants for Flare created SP device names.
 * Combining them with the SP letter generates the appropriate name, e.g. \Device\CLARiiONspA.
 */

#define FLARE_SP_DEVICE_NAME_PREFIX         "CLARiiONsp"
#define FLARE_SP_DEVICE_PATH_PREFIX         "\\Device\\" FLARE_SP_DEVICE_NAME_PREFIX


/*
 * Not really a string, but a macro to return a 32-bit version of sunburst error code
 */

#define FLARE_ERROR_BASE 0x60000000
// use for kernel ops to Logging interface
//
#define FLARE_ERROR_KERNEL( val ) ((long)(FLARE_ERROR_BASE | (long)val))

// FLARE_ERROR_USER converts 16 bit Flare Sunburst codes to 32 bit k10 format
//  sunburst X0YZ  becomes 6000SXYZ,  where S is the severity.

// The following 2 defs exist only to simplify FLARE_ERROR_USER
#define SUNB_HI_NIB(val)            (val & 0xF000)
// Severity mapping:  80xx is warning, 90xx is error, A0xx is critical, rest are info.
#define SUNB_SEV( x ) \
( ((SUNB_HI_NIB(x) >= 0x8000) && (SUNB_HI_NIB(x) <= 0xA000)) ? 4*(SUNB_HI_NIB(x) - 0x7000) : 0 )

#define FLARE_ERROR_USER( val ) \
    ((( val & 0xFF) | (( val & 0xF000) >> 4)) | (long) FLARE_ERROR_BASE) | (long) SUNB_SEV(val)

#define IS_FLARE_ERROR(val)  ((val & 0xFFFF0000) == (FLARE_ERROR_BASE))

//--
//
// END Definitions of Flare strings for objects created in NT namespace
//
//++

//++
//
// BEGIN Definitions for StorageCentric
//
//--

// These values MUST be in byte order shown, as they are bitbuckets
// Note: DO NOT do this:
//  ULONGLONG ul = SC_DEFAULT_VARRAY_UPPER;
//
// The Intel processor will scramble your bytes!
//
#define SC_NULL_ARRAY_UPPER         CSX_CONST_U64(0x6006016000000000)
#define SC_NULL_ARRAY_LOWER         CSX_CONST_U64(0x0000000000000000)

#define SC_PHYSICAL_ARRAY_UPPER     CSX_CONST_U64(0x6006016000000000)
#define SC_PHYSICAL_ARRAY_LOWER     CSX_CONST_U64(0x0000000000000001)

#define SC_MGMT_ARRAY_UPPER         CSX_CONST_U64(0x6006016000000000)
#define SC_MGMT_ARRAY_LOWER         CSX_CONST_U64(0x0000000000000002)

#define SC_DEFAULT_VARRAY_UPPER     CSX_CONST_U64(0x6006016000000000)
#define SC_DEFAULT_VARRAY_LOWER     CSX_CONST_U64(0x0000000000000003)

#define SC_CELERRA_ARRAY_UPPER      CSX_CONST_U64(0x6006016000000000)
#define SC_CELERRA_ARRAY_LOWER      CSX_CONST_U64(0x0000000000000004)

#ifdef C4_INTEGRATED
// in VNXe fakeports numbers are starting at FAKEPORT_PORTNUM_BASE(64) + MP_COMM_MAX_DART_INSTANCES(5) * SP_COUNT_PER_ARRAY(2)
// + 2 (proxy ports per SP) = 76
#define K10_MAX_PORT_VALUE                              76
#else
// You can have up to 40 ports per SP, plus 5 fake ports for MP Comm, starting at 0.
#define K10_MAX_PORT_VALUE                              44
#endif /* C4_INTEGRATED - C4ARCH - config differences */

#define K10_MAX_PORT_VALUE_SPIN_4_BITS                  16

#define K10_PORT_VALUE_RANGE1_SP_SCOPED_SPA_START       0
#define K10_PORT_VALUE_RANGE1_SP_SCOPED_SPA_END         7
#define K10_PORT_VALUE_RANGE2_SP_SCOPED_SPA_START       16
#define K10_PORT_VALUE_RANGE2_SP_SCOPED_SPA_END         23
#define K10_PORT_VALUE_RANGE3_SP_SCOPED_SPA_START       32
#define K10_PORT_VALUE_RANGE3_SP_SCOPED_SPA_END         39
#define K10_PORT_VALUE_RANGE4_SP_SCOPED_SPA_START       48
#define K10_PORT_VALUE_RANGE4_SP_SCOPED_SPA_END         55

#define K10_PORT_VALUE_RANGE1_SP_SCOPED_SPB_START       8
#define K10_PORT_VALUE_RANGE1_SP_SCOPED_SPB_END         15
#define K10_PORT_VALUE_RANGE2_SP_SCOPED_SPB_START       24
#define K10_PORT_VALUE_RANGE2_SP_SCOPED_SPB_END         31
#define K10_PORT_VALUE_RANGE3_SP_SCOPED_SPB_START       40
#define K10_PORT_VALUE_RANGE3_SP_SCOPED_SPB_END         47
#define K10_PORT_VALUE_RANGE4_SP_SCOPED_SPB_START       56
#define K10_PORT_VALUE_RANGE4_SP_SCOPED_SPB_END         63

//--
//
// END Definitions for StorageCentric
//
//++



//++
//
// BEGIN Definitions used in IK10Admin implementation
//
//--

//
//  Define the maximum length in ASCII characters of a Host Port name.
//
#define K10_MAX_HPORT_NAME_LEN      64

//
//  Define the maximum length in ASCII characters of the per-Initiator info.
//
#define K10_MAX_INITIATOR_INFO_LEN  896

//
//  Define the maximum length in ASCII characters of a Virtual Array name.
//
#define K10_MAX_VARRAY_NAME_LEN     64

//
//  Define the UNICODE character length of the user friendly name that can be assigned to
//  exported devices. Because we are using UNICODE characters the byte length is 64.
//
#define K10_LOGICAL_ID_LEN_NTWCHARS     32
#define K10_LOGICAL_ID_LEN_CHARS    64

//
//  Define the length in ASCII characters can be assigned to exported devices.
//
#define K10_LOGICAL_ID_LEN_CHAR     64

//
//  Define the length in ASCII characters of device objects created by drivers on K10.
//
#define K10_DEVICE_ID_LEN           64

//
//  Define the length in ASCII characters of device links created by drivers on K10.
//
#define K10_DEVICE_LINK_LEN         (K10_DEVICE_ID_LEN + 5)

//
//  Define the length in characters for MessageManager IDs (see MMan.h for use of MmId in
//  interprocess communications for user Space processes, and the K10 Interprocess Communications
//  Design Specification).
//
#define MMAN_ADDR_NAME_LEN          32

//
//  Use MMAN_ADDR_NAME_LEN as the basis for defining the length of Admin library names.
//  This insures that implementors of IK10Admin can also be processes, and receive UserSpace
//  MessageManager communications
//
#define MMAN_LIBRARY_NAME_LEN       MMAN_ADDR_NAME_LEN

//
//  Define the maximum value for IK10Admin interface Database ID's.
//  Allows 65295 system private database IDs
//
#define K10_MAX_LIBDEF_DBID 0xFF0F

// Define the max timeout value for asynchronous DeviceIoControl, 10 mins
#define   MAX_IOCTL_TIMEOUT 600000

// Define the parameters for page26&37 extension
#define CAB_DPE_ONBOARD_AGENT   0x22

#define SP_MODEL    4700

#define PAGE_REVISION   1

#define INTERFACE_REVISION  1
//
// these are the hardcoded values we use when in degraded mode ONLY
//
#define LIC_FEATURE_LIST    (0x01 | 0x04 | 0x08)

//--
//
// END Definitions used in IK10Admin implementation
//
//++

//++
//
// BEGIN Definitions for K10 registry settings.
//
//--

#define K10_SOFTWARE_REVISION   "K10SoftwareRevision"
#define K10_BUNDLE_REVISION "K10BundleRevision"
#define K10_BIOS_PATH           "BiosPath"
#define K10_POST_PATH           "PostPath"
#define K10_DDBS_PATH           "DdbsPath"
#define K10_GPS_FW_PATH         "EastPath"
#define EMC_STRING              "EMC"
#define EMC_STRING_W            "EMC"
#define K10_ROOT                "K10"
#ifdef ALAMOSA_WINDOWS_ENV
#define K10_REGISTRY_GLOBAL_USER "SOFTWARE\\" EMC_STRING "\\K10\\Global"
#else
#define K10_REGISTRY_GLOBAL_USER "Software\\" EMC_STRING "\\K10\\Global"
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
#define K10_REGISTRY_GLOBAL "\\REGISTRY\\Machine\\SOFTWARE\\" EMC_STRING_W "\\K10\\Global"
#ifdef ALAMOSA_WINDOWS_ENV
#define K10_REGISTRY_USER_PROCESS "SOFTWARE\\" EMC_STRING "\\K10"
#else
#define K10_REGISTRY_USER_PROCESS "Software\\" EMC_STRING "\\K10"
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */

// The following is a unique string used by 64-bit drivers to access 32-bit registry components,
// under the software key
#define K10_REGISTRY_64_TO_32_GLOBAL "\\REGISTRY\\Machine\\SOFTWARE\\Wow6432Node\\" EMC_STRING "\\K10\\Global"

//--
//
// END Definitions for K10 registry settings.
//
//++

////++++
//
// BEGIN TYPEDEFS
//
////----

#ifndef NTWCHAR
#define NTWCHAR USHORT
#define PNTWCHAR PUSHORT
#endif

//++
//
// BEGIN Typedefs used in implementations of IK10Admin, including objects exported
//       by layered drivers
//
//--

//
//  Define the format of the LU name that is visible and configurable by the administrator
//  through the GenericProps functions.
//
#ifndef ALAMOSA_WINDOWS_ENV
#define K10_LOGICAL_ID_SANE_WITH_ACCESSORS
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - let non-Windows case test out abstraction of this odd structure */
#ifdef K10_LOGICAL_ID_SANE_WITH_ACCESSORS
typedef struct { NTWCHAR ntwchars[K10_LOGICAL_ID_LEN_NTWCHARS]; } K10_LOGICAL_ID;
#define GET_LID_CHARS(_lid) ((char *)(_lid.ntwchars))
#define GET_LID_PTR_CHARS(_lid) ((char *)(_lid->ntwchars))
#define GET_LID_NTWCHARS(_lid) ((NTWCHAR *)(_lid.ntwchars))
#else
typedef NTWCHAR K10_LOGICAL_ID[K10_LOGICAL_ID_LEN_NTWCHARS];
#define GET_LID_CHARS(_lid) ((char *)(_lid))
#define GET_LID_PTR_CHARS(_lid) ((char *)(_lid))
#define GET_LID_NTWCHARS(_lid) ((NTWCHAR *)(_lid))
#endif

//
//  Definition of a device object name.
//
typedef char K10_DVR_OBJECT_NAME[K10_DEVICE_ID_LEN];

//
//  Definition of a device object link.
//
typedef char K10_DVR_OBJECT_LINK[K10_DEVICE_LINK_LEN];

//
//  Definition of a K10 compliant SCM name for the driver.
//  Example: "SnapCopy"
//
typedef char K10_DVR_SCM_NAME[MMAN_LIBRARY_NAME_LEN];

//
//  Definition of an IK10Admin compliant admin library name.
//  Example: "SnapCopyAdmin"
//
typedef char K10_DVR_NAME[MMAN_LIBRARY_NAME_LEN];

//
//  Definition of string to list driver exclusions.  Allow for
//  up to 8 exclusions with 1 char token between entries.
//  Example: "K10SnapCopyAdmin;K10RemotMirrorAdmin"
//
typedef char K10_DVR_EXCL_LIST[((MMAN_LIBRARY_NAME_LEN+1)*8)];


//--
//
// END Typedefs used in implementations of IK10Admin
//
//++



//++
//
// BEGIN Typedefs used in implementations of SCSI and FiberChannel compliant identification
//       implementations.
//
//--

//
//  Name:
//
//      K10_WWN, *PK10_WWN
//
//  Description:
//
//      Definition of a 64-bit World Wide Name, IEEE type 5.
//
typedef struct K10_wwn {
    //
    //  Array containing the 64 bits that make up the World Wide Name.
    //
    unsigned char   bytes[8];

# ifdef __cplusplus
    // Cast to ULONGLONG returns value appropriate for printing as %I64X,
    // which is byteswapped.
    // The type "ULONGLONG" is not defined in all cases, so we instead use
    // its undelying defintion.
    operator unsigned __int64 () const{
        union {
            unsigned char   b[8];
            unsigned __int64        ull;
        } x;
        x.b[0] = bytes[7];
        x.b[1] = bytes[6];
        x.b[2] = bytes[5];
        x.b[3] = bytes[4];
        x.b[4] = bytes[3];
        x.b[5] = bytes[2];
        x.b[6] = bytes[1];
        x.b[7] = bytes[0];
        return x.ull;
    }
# endif
} K10_WWN, *PK10_WWN;

# ifdef __cplusplus
    extern "C++" inline bool operator ==(const K10_wwn & first, const K10_wwn & other)
    {
       /* using locals to avoid strict aliasing warning with -O2 (compiler should optimize away anyway) */
       __int64 * f = (__int64 *)first.bytes;
       __int64 * o = (__int64 *)other.bytes;
       return (*f == *o);
    }

    extern "C++" inline bool operator >(const K10_wwn & first, const K10_wwn & other)
    {
       /* using locals to avoid strict aliasing warning with -O2 (compiler should optimize away anyway) */
       __int64 * f = (__int64 *)first.bytes;
       __int64 * o = (__int64 *)other.bytes;
       return (*f > *o);
    }

    extern "C++" inline bool operator >=(const K10_wwn & first, const K10_wwn & other)
    {
       /* using locals to avoid strict aliasing warning with -O2 (compiler should optimize away anyway) */
       __int64 * f = (__int64 *)first.bytes;
       __int64 * o = (__int64 *)other.bytes;
       return (*f >= *o);
    }

    extern "C++" inline bool operator <(const K10_wwn & first, const K10_wwn & other)
    {
       /* using locals to avoid strict aliasing warning with -O2 (compiler should optimize away anyway) */
       __int64 * f = (__int64 *)first.bytes;
       __int64 * o = (__int64 *)other.bytes;
       return (*f < *o);
    }

    extern "C++" inline bool operator <=(const K10_wwn & first, const K10_wwn & other)
    {
       /* using locals to avoid strict aliasing warning with -O2 (compiler should optimize away anyway) */
       __int64 * f = (__int64 *)first.bytes;
       __int64 * o = (__int64 *)other.bytes;
       return (*f <= *o);
    }
# endif

//
//  Name:
//
//      K10_WWID, *PK10_WWID
//
//  Description:
//
//      Definition of a 128-bit World Wide ID, IEEE type 6. Represented as a FiberChannel unique ID
//
typedef struct K10_wwid {
    //
    //  Array containing the 64 bits that make up the nPort ID of a FibreChannel ID
    //
    K10_WWN     nPort;
    //
    //  Array containing the 64 bits that make up the nPort ID of a FibreChannel ID
    //
    K10_WWN     node;
# ifdef __cplusplus
    bool operator ==(const K10_wwid & other) const
    {
        return nPort == other.nPort && node == other.node;
    }

        bool operator !=(const K10_wwid & other) const
    {
        return !(nPort == other.nPort && node == other.node);
    }

    bool operator >(const K10_wwid & other) const
    {
        return nPort > other.nPort ||
            (nPort == other.nPort && node > other.node);
    }

    bool operator >=(const K10_wwid & other) const
    {
        return nPort > other.nPort ||
            (nPort == other.nPort && node >= other.node);
    }

    bool operator <(const K10_wwid & other) const
    {
        return nPort < other.nPort ||
            (nPort == other.nPort && node < other.node);
    }

    bool operator <=(const K10_wwid & other) const
    {
        return nPort < other.nPort ||
            (nPort == other.nPort && node <= other.node);
    }

    ULONG       Hash() const {
       /* using locals to avoid strict aliasing warning when building with -O2 (compiler should optimize away anyway) */
       ULONG * node0 = (ULONG *)&node.bytes[0];
       ULONG * node4 = (ULONG *)&node.bytes[4];
       ULONG * nport0 = (ULONG *)&nPort.bytes[0];
       ULONG * nport4 = (ULONG *)&nPort.bytes[4];
       return *node0 ^ *node4 ^ *nport0 ^ *nport4;
    }
# endif

} K10_WWID, *PK10_WWID;

# ifdef __cplusplus

struct K10_WWID_Zero : K10_WWID {
    K10_WWID_Zero() {
         for (ULONG i=0; i < 8; i++) {
            nPort.bytes[i] = 0;
            node.bytes[i] = 0;
        }
    }
};
# endif

//--
//
// END Typedefs used in implementations of SCSI and FiberChannel compliant identification
//       implementations.
//
//++


//++
//
// BEGIN Typedefs used to define implementations of SCSI and FiberChannel identifications
//        as they pertain to CLARiiON implementations.
//
//--


//
//  Define a Logical Unit ID as being a K10_WWID ( 128 bits ). We have a separate definition mainly for (requested)
//  convenience, as this identifier type is defined by SCSI standard and unlikely to change.
//
typedef K10_WWID    K10_LU_ID;
typedef K10_WWID    *PK10_LU_ID;

//
//  Define the Storage Processor ID as a K10_WWN ( 64 bits ). We have a separate definition mainly for (requested)
//  convenience, as this identifier type is defined by SCSI standard and unlikely to change.
//
typedef K10_WWN K10_SP_ID;
typedef K10_WWN *PK10_SP_ID;

//
//  Define the Storage Processor ID as a K10_WWN ( 64 bits ). This is not a SCSI standard,
//  as SCSI does not define a redundant array, only controllers, and thus may change.
//
typedef K10_WWN K10_ARRAY_ID;
typedef K10_WWN *PK10_ARRAY_ID;


//--
//
// END Typedefs used to define implementations of SCSI and FiberChannel identifications
//        as they pertain to CLARiiON implementations.
//
//++

//
//  On disk data compatability helpers
//

#include "oddc.h"

////----
//
// END TYPEDEFS
//
////++++

#endif // #ifndef K10_DEFS_H
