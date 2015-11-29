//**********************************************************************
//.++
// COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2006, 2009
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//.--                                                                    
//**********************************************************************

//**********************************************************************
//.++
//  FILE NAME:
//      iSCSIZonesInterface.h
//
//  DESCRIPTION:
//      
//
//  REVISION HISTORY:
//      12-Jan-09   Patsinr3    Removed #define ZONE_PSM_DATA_AREA_NAME  L"iSCSI Zones"
//      24-Sep-08   miranm      Modified for virtual ports support:
//                              - added/modified data types
//                              - increased DB version to 2_1
//      13-Dec-06   kdewitt     Added #pragma pack to PSM structures:
//                              IP_IDENTIFIER, IP_ADDR.
//      27-Jam-06   kdewitt     Changed the ISCSI_ZONE_DB_VERSION to
//                              support the new ISCSI_ZONE_AUTH_INFO
//                              structure (157701).
//      02-Oct-06   kdewitt		Initial Port to 64 bit OS.
//      05-Jan-06   kdewitt     Fixed GET_REVERSE_GLOBAL_AUTH calculation
//      04-Jan-06   kdewitt     Modified IP_ADDR_TYPE
//      22-Dec-05   majmera     Added #define for version number.
//      21-Dec-05   kdewitt     Added pReverseGlobalAuth to the
//                              ISCSI_ZONE_AUTH_INFO structure.
//      26-May-05   kdewitt     Added ISCSI_ZONE_DB_VERSION
//      31-Aug-04   majmera     Created.
//.--                                                                   
//**********************************************************************

#ifndef _ISCSI_ZONES_INTERFACE_H_
#define _ISCSI_ZONES_INTERFACE_H_

#include "generic_types.h"
#include "csx_ext.h"

//
// Added to fix compilation problems from including the messages.h file
//
//
#ifndef UMODE_ENV
#include "k10ntddk.h"
#endif
#include "k10ntstatus.h" 

//
// This is the CPD file that contains all the iSCSI definitions that we need.
// The file was created by us to get around the compilation problems arising from
// include cpd_generics.h and cpd_interface.h. 
//

#include "cpd_generics.h"

#include "spid_types.h"

//
// This file contains the iSCSI Zone database versions
//
#include "ndu_toc_common.h"

//
// iSCSI Zone database PSM data area name
//
#ifdef UMODE_ENV
#define ZONE_PSM_DATA_AREA_NAME            "iSCSI Zones"
#else
#define ZONE_PSM_DATA_AREA_NAME            "iSCSI Zones"
#endif

#define IZ_ADMIN_TRACE_NAME   "iSCSIZA"

//
// Data type for zone ID
//
typedef	ULONG32		ISCSI_ZONE_ID;

//
// iSCSI Zone database versions were moved to ndu_toc_common.h.
//

//
// Max number of FE ports that the DLL supports per SP.
//
#define MAX_PHYS_PORTS_SUPPORTED            64

//
// Max number of virtual ports that can be associated with a FE port. 
// This is limited by the number of bits we use in the virtual port container.
// 
#define MAX_VIRTUAL_PORTS_SUPPORTED       16

//
// This bitmap represents the list of virtual port numbers
// associated with a FE port
// 
typedef UINT_16E                          VIRTUAL_PORTS_CONTAINER;

//
// This bitmap represents the list of used path number of a zone.
//
typedef ULONG64				PATH_NUMBER_BITMAP;

//
// This bitmap represents the list of used auth number of a zone.
//
typedef USHORT				AUTH_NUMBER_BITMAP;

// 
// Data type for virtual ports
// 
#pragma pack(push, ISCSI_ZONE_VIRTUAL_PORT, 1)
typedef struct _ISCSI_ZONE_VIRTUAL_PORT
{
    USHORT      FEPortIndex;
    USHORT      VirtualPortIndex;
    SP_ID       SPOwner;
} ISCSI_ZONE_VIRTUAL_PORT, *PISCSI_ZONE_VIRTUAL_PORT;
#pragma pack(pop, ISCSI_ZONE_VIRTUAL_PORT)

//
// Invalid zone ID value
//
#define INVALID_ZONE_ID                   0

//
// Invalid path number value
//
#define INVALID_PATH_NUMBER               0

//
// Invalid auth number value
//
#define INVALID_AUTH_NUMBER               0

//
// Invalid FE Port index value
//
#define INVALID_FE_PORT_INDEX             -1

//
// Invalid virtual port index value
//
#define INVALID_VIRTUAL_PORT_INDEX        -1

//
// Maximum length of iSCSI zone name
//
#define IZ_NAME_MAX_LENGTH                  64

//
// Maximum length of iSCSI path desc
//
#define IZ_PATH_DESC_MAX_LENGTH             256

//
// List of different types of IP address that we can store in the DB.
//
enum _IP_ADDR_TYPE
{
    INVALID = 0,
    IP_V4   = 1,
    IP_V6   = 2,
    FQDN    = 3
};
typedef UCHAR IP_ADDR_TYPE;


//
// Maximum length of an IP Address
//
#define IP_ADDR_MAX_LENGTH                256

//
// Union to store the actual IP address in different formats.
//
#pragma pack(push, IP_ADDR, 1)
typedef union _IP_ADDR
{   
    csx_uchar_t IPV4_addr[CPD_IP_ADDRESS_V4_LEN];
    csx_uchar_t IPV6_addr[CPD_IP_ADDRESS_V6_LEN];
    char FQDN_addr[IP_ADDR_MAX_LENGTH];
}IP_ADDR;
#pragma pack(pop, IP_ADDR)

// 
// Data type for IP address
//    An FQDN is 255 chars long + a terminating NULL.
//
#pragma pack(push, IP_IDENTIFIER, 1)
typedef struct _IP_IDENTIFIER
{
    IP_ADDR         IPAddr;
    IP_ADDR_TYPE    IPAddrType;
}IP_IDENTIFIER, * PIP_IDENTIFIER;
#pragma pack(pop, IP_IDENTIFIER)

#define ISCSI_ZONES_DATA_AREA_MAX_SIZE                  1024 * 1024 // 1 MByte PSM for Zone Database

#define ISCSI_ZONES_GLOBAL_AUTH_COUNT                   2

//#define IZ_ADMIN_MAX_ZONE_RECORDS_COUNT_AT_LEAST       (((ISCSI_ZONES_DATA_AREA_MAX_SIZE) - \
//                                                            (ISCSI_ZONES_GLOBAL_AUTH_COUNT)*sizeof(ISCSI_ZONE_AUTH_INFO) - \
//                                                            sizeof(ISCSI_ZONE_DB_HEADER)) / (sizeof(ISCSI_ZONE_ENTRY_HEADER) + \
//                                                           64 * sizeof(ISCSI_ZONE_PATH) + \
//                                                           64 * sizeof(ISCSI_ZONE_AUTH_INFO) + \
//                                                           64 * sizeof(ISCSI_ZONE_PATH_DESC_HEADER) + \
//                                                           64 * sizeof(IZ_PATH_DESC_MAX_LENGTH) * sizeof(csx_uchar_t) ))

#define IZ_ADMIN_MAX_ZONE_RECORDS_COUNT_IN_THEORY       (((ISCSI_ZONES_DATA_AREA_MAX_SIZE) - \
                                                            (ISCSI_ZONES_GLOBAL_AUTH_COUNT)*sizeof(ISCSI_ZONE_AUTH_INFO) - \
                                                            sizeof(ISCSI_ZONE_DB_HEADER)) / (sizeof(ISCSI_ZONE_ENTRY_HEADER)))

#define IZ_ADMIN_MAX_ZONEDB_COUNT                       1


//
// List of Authentication info used during login.
//
enum _K10_ISCSI_ZONE_AUTH_USAGE {
    ISCSI_ZONE_AUTH_USAGE_NONE = 0,
    ISCSI_ZONE_AUTH_USAGE_DEFINED,
    ISCSI_ZONE_AUTH_USAGE_GLOBAL,
};

typedef UCHAR K10_ISCSI_ZONE_AUTH_USAGE;

//*****************************************************************************
//.++
//
//  TYPE:
//      ISCSI_ZONE_DB_HEADER
//
//  DESCRIPTION:
//      ISCSI_ZONE_DB_HEADER structure will be used to represent the header 
//      the iSCSI Zone database area stored in PSM
//
//  MEMBERS:
//      Version                  : Indicates the version no. of the zone database
//      Generation               : Indicates the Generation number of the  database.
//      DatabaseSize             : Size of the database in bytes (including the header).
//      ZoneCount                : no. of Zones in the database.
//      Seed                     : Used to generate zone ID.
//      PadBytes[16]             : Reserved for future use.
//
//  REVISION HISTORY
//      1-Sep-04    majmera    Created       
//
//.--
//*****************************************************************************
#pragma pack(push, ISCSI_ZONE_DB_HEADER, 1)
typedef struct _ISCSI_ZONE_DB_HEADER
{
    USHORT      Version;    
    ULONG32     Generation;  
    ULONG32     DatabaseSize;
    ULONG32     ZoneCount;
    ULONG32     Seed;
    csx_uchar_t PadBytes[16];
}ISCSI_ZONE_DB_HEADER, * PISCSI_ZONE_DB_HEADER;
#pragma pack(pop, ISCSI_ZONE_DB_HEADER)

//*****************************************************************************
//.++
//
//  TYPE:
//      ISCSI_ZONE_ENTRY_HEADER
//
//  DESCRIPTION:
//      This structure will be used to represent the header of an iSCSI Zone 
//      Entry stored in memory
//  MEMBERS:
//      ZoneID              : ID for this Zone. Used as a key for the record.
//      ZoneName            : User defined name for the Zone.
//      ZoneSize            : Size of the entire zone. Needed to indicate the end of 
//                            this zone's data.
//      ZoneAuthCount       : Number of user name/passwd pairs in this zone.
//      ZonePathCount       : The number of IP Identifier/TCPListeningPort/FEPortMask 
//                            sets defined in this iSCSI Zone
//      HeaderDigest        : Flag to indicate if the header digest will be used in the
//                            iSCSI PDUs
//      DataDigest          : Flag to indicate if the data digest will be used in the
//                            iSCSI PDUs
//      PadBytes            : Reserved for future use.
//
//  REVISION HISTORY
//      1-Sep-04    majmera    Created       
//
//.--
//*****************************************************************************
#pragma pack(push, ISCSI_ZONE_ENTRY_HEADER, 1)
typedef struct _ISCSI_ZONE_ENTRY_HEADER
{
    ISCSI_ZONE_ID    ZoneID;
    CHAR             ZoneName[IZ_NAME_MAX_LENGTH];
    ULONG32          ZoneSize;
    ULONG32          ZoneAuthCount;
    ULONG32          ZonePathCount;
    csx_bool_t          HeaderDigest;
    csx_bool_t          DataDigest;
    csx_uchar_t      PadBytes[16];
}ISCSI_ZONE_ENTRY_HEADER,* PISCSI_ZONE_ENTRY_HEADER;
#pragma pack(pop, ISCSI_ZONE_ENTRY_HEADER)


//*****************************************************************************
//.++
//
//  TYPE:
//      ISCSI_ZONE_AUTH_INFO
//
//  DESCRIPTION:
//      This structure defines the authentication information defined within a Zone as 
//      well the Global Authentication Information defined on the array
//  MEMBERS:
//      AuthNumber          : This is the number assigned to each authentication information
//                            set.  This number will be unique per zone. 
//      AuthUsage           : This is authentication usage information (none/defined/global)
//      AuthType            : The type of authentication (forward/reverse)
//      AuthMethod          : The method of authentication used. Chap etc
//      UserNameLength      : Length of the username for authentication
//      UserName            : UserName specified for authentication
//      PasswordLength      : Length of the password for authentication.
//      Password            : Password specified for Authentication.
//      PasswordInfo        : Used by Navi to describe the format of the password
//      PadBytes[8]         : Reserved for future use.
//
//  REVISION HISTORY
//      24-Oct-06   kdewitt    Moved pReverseGlobalAuth to ZML_ZONE_AUTH_INFO
//                             (157701)
//      21-Dec-05   kdewitt    Added pReverseGlobalAuth
//      1-Sep-04    majmera    Created       
//
//.--
//*****************************************************************************
#pragma pack( push, ISCSI_ZONE_AUTH_INFO, 1 )

typedef struct _ISCSI_ZONE_AUTH_INFO
{
    USHORT                         AuthNumber;
    K10_ISCSI_ZONE_AUTH_USAGE      AuthUsage;
    ULONG32                        AuthType;
    ULONG32                        AuthMethod;
    USHORT                         UserNameLength;
    CHAR                           UserName[ISCSI_NAME_NAME_MAX_SIZE];
    USHORT                         PasswordLength;
    CHAR                           Password[CPD_AUTH_PASSWORD_MAX_SIZE];
    ULONG32                        PasswordInfo;
    csx_uchar_t                    PadBytes[8];
}ISCSI_ZONE_AUTH_INFO, * PISCSI_ZONE_AUTH_INFO;
#pragma pack( pop, ISCSI_ZONE_AUTH_INFO)

//*****************************************************************************
//.++
//
//  TYPE:
//      ISCSI_ZONE_PATH
//
//  DESCRIPTION:
//      This structure contains the information for each iSCSI Path defined 
//      within a Zone
//  MEMBERS:
//      PathNumber          : This is the number assigned to each path. 
//                            This number will be unique per zone.
//      IPIdentifier        : IP address for the target.
//      TCPListeningPort    : TCP Port that the target is listening on.
//      VLanID              : Virtual LAN identifier for this path.
//      SPAPortList         : Bitmap representing a list of SPA's Virtual Ports 
//                            to be used for connecting to a Target Portal (defined
//                            above by the IPIdentifier and TCPListeningPort)
//      SPBPortList         : Bitmap representing a list of SPB's Virtual Ports 
//                            to be used for connecting to a Target Portal (defined
//                            above by the IPIdentifier and TCPListeningPort)
//      OriginatorIDMask    : Bitmap representing a list of Originators (users) of 
//                            this Zone Path
//      PadBytes[32]         : Reserved for future use.
//
//  REVISION HISTORY
//      25-Sep-08   miranm     Removed VLanID. Modified FE port masks and pad bytes.
//      1-Sep-04    majmera    Created       
//
//.--
//*****************************************************************************
#pragma pack( push, ISCSI_ZONE_PATH, 1 )
typedef struct _ISCSI_ZONE_PATH
{
    USHORT                    PathNumber;
    IP_IDENTIFIER             IPIdentifier;
    USHORT                    TCPListeningPort;
    VIRTUAL_PORTS_CONTAINER   SPAPortList[MAX_PHYS_PORTS_SUPPORTED];
    VIRTUAL_PORTS_CONTAINER   SPBPortList[MAX_PHYS_PORTS_SUPPORTED];
    ULONG32                   OriginatorIDMask;
    csx_uchar_t               PadBytes[32];
}ISCSI_ZONE_PATH, * PISCSI_ZONE_PATH;
#pragma pack( pop, ISCSI_ZONE_PATH)

//
// This is the ISCSI_ZONE_PATH structure as it exists in a zone database 
// with version ISCSI_ZONE_DB_VERSION _1_2 (before virtual port changes). 
// This will be used to traverse a database of the old format during the 
// conversion (to virtual port support) process.
//
#pragma pack( push, ISCSI_ZONE_PATH_1_2, 1 )
typedef struct _ISCSI_ZONE_PATH_1_2
{
    USHORT              PathNumber;
    IP_IDENTIFIER       IPIdentifier;
    USHORT              TCPListeningPort;
    ULONG32             VLanID;
    ULONG32             SPAFEPortMask;
    ULONG32             SPBFEPortMask;
    ULONG32             OriginatorIDMask;
    csx_uchar_t         PadBytes[8];
}ISCSI_ZONE_PATH_1_2, * PISCSI_ZONE_PATH_1_2;
#pragma pack( pop, ISCSI_ZONE_PATH_1_2)

//*****************************************************************************
//.++
//
//  TYPE:
//      ISCSI_ZONE_PATH_DESC_HEADER
//
//  DESCRIPTION:
//      This structure contains the information for each iSCSI Path defined 
//      within a Zone
//  MEMBERS:
//      PathNumber          : This is the number assigned to each path. 
//                            This number will be unique per zone.
//      PathDescLength      : Length in bytes for description of the path.
//      PadBytes[4]         : Reserved for future use.
//
//  REVISION HISTORY
//      1-Sep-04    majmera    Created       
//
//.--
//*****************************************************************************
#pragma pack( push, ISCSI_ZONE_PATH_DESC_HEADER, 1 )
typedef struct _ISCSI_ZONE_PATH_DESC_HEADER
{
    USHORT           PathNumber;
    csx_uchar_t      PathDescLength;
    csx_uchar_t      PadBytes[4];
}ISCSI_ZONE_PATH_DESC_HEADER, * PISCSI_ZONE_PATH_DESC_HEADER;
#pragma pack( pop, ISCSI_ZONE_PATH_DESC_HEADER)

#endif //_ISCSI_ZONES_INTERFACE_H_

//
// Common offset calculation macros
//

//
// Get the offset to the first zone
//
#define GET_ZONE_LIST_OFFSET()                                              \
    (ULONG) (sizeof(ISCSI_ZONE_DB_HEADER) +                                 \
            (ISCSI_ZONES_GLOBAL_AUTH_COUNT * sizeof(ISCSI_ZONE_AUTH_INFO)))

//
// Given the DB pointer, get the first zone.
//
#define GET_ZONE_LIST(_pZoneDB_)                                          \
    (PISCSI_ZONE_ENTRY_HEADER) (((csx_uchar_t *) _pZoneDB_) +             \
                                GET_ZONE_LIST_OFFSET())

//
// Given the ZoneHeader, get the next ZoneHeader
//
#define GET_NEXT_ZONE(_pZoneEntryHeader_)                                 \
    (PISCSI_ZONE_ENTRY_HEADER) (((csx_uchar_t *) _pZoneEntryHeader_) + _pZoneEntryHeader_->ZoneSize)

//
// Given the ZoneHeader, get the zone size
//
#define GET_ZONE_SIZE(_pZoneEntryHeader_)                                      \
     _pZoneEntryHeader_->ZoneSize

//
// Given the ZoneHeader, get the offset to the first path
//
#define GET_ZONE_PATH_LIST_OFFSET(_pZoneEntryHeader_)                          \
    (ULONG) (sizeof(ISCSI_ZONE_ENTRY_HEADER) +                                 \
            _pZoneEntryHeader_->ZoneAuthCount * sizeof(ISCSI_ZONE_AUTH_INFO))

//
// Given the ZoneHeader, get the first path
//
#define GET_ZONE_PATH_LIST(_pZoneEntryHeader_)                                 \
    (PISCSI_ZONE_PATH)                                                         \
        ((csx_uchar_t *)_pZoneEntryHeader_ + GET_ZONE_PATH_LIST_OFFSET(_pZoneEntryHeader_))

//
// Given the ZoneHeader, get the first path for database version 0x0102
//
#define GET_ZONE_PATH_LIST_1_2(_pZoneEntryHeader_)                                 \
    (PISCSI_ZONE_PATH_1_2)                                                         \
        ((csx_uchar_t *)_pZoneEntryHeader_ + GET_ZONE_PATH_LIST_OFFSET(_pZoneEntryHeader_))

//
// Given the Path, get the next Path
//
#define GET_NEXT_PATH(_pPathInfo_)                                 \
    (PISCSI_ZONE_PATH)(((csx_uchar_t *) _pPathInfo_) + sizeof(ISCSI_ZONE_PATH))

//
// Given the Path, get the next Path for database version 0x0102
//
#define GET_NEXT_PATH_1_2(_pPathInfo_)                                 \
    (PISCSI_ZONE_PATH_1_2)(((csx_uchar_t *) _pPathInfo_) + sizeof(ISCSI_ZONE_PATH_1_2))

//
// Given the ZoneHeader, get the first Path Desc
//
#define GET_ZONE_PATH_DESC_LIST(_pZoneEntryHeader_)                            \
    (PISCSI_ZONE_PATH_DESC_HEADER)                                             \
        ((csx_uchar_t *)GET_ZONE_PATH_LIST(_pZoneEntryHeader_) +               \
                 _pZoneEntryHeader_->ZonePathCount * sizeof(ISCSI_ZONE_PATH))

//
// Given the ZoneHeader, get the first Path Desc for database version 0x0102
//
#define GET_ZONE_PATH_DESC_LIST_1_2(_pZoneEntryHeader_)                        \
    (PISCSI_ZONE_PATH_DESC_HEADER)                                             \
        ((csx_uchar_t *)GET_ZONE_PATH_LIST(_pZoneEntryHeader_) +               \
                 _pZoneEntryHeader_->ZonePathCount * sizeof(ISCSI_ZONE_PATH_1_2))

//
// Given the ZoneHeader, get the first Auth
//
#define GET_ZONE_AUTH_LIST(_pZoneEntryHeader_)                                 \
    (PISCSI_ZONE_AUTH_INFO) ((csx_uchar_t *)_pZoneEntryHeader_ + sizeof(ISCSI_ZONE_ENTRY_HEADER))

//
// Given the DB pointer, get the GlobalAuth
//
#define GET_GLOBAL_AUTH_LIST(_pZoneDB_)                                        \
    ( PISCSI_ZONE_AUTH_INFO)((csx_uchar_t *)_pZoneDB_ + sizeof(ISCSI_ZONE_DB_HEADER))

//
// Given the DB pointer, get the Forward GlobalAuth
//
#define GET_FORWARD_GLOBAL_AUTH(_pZoneDB_)                                     \
    ( PISCSI_ZONE_AUTH_INFO)((csx_uchar_t *)_pZoneDB_ + sizeof(ISCSI_ZONE_DB_HEADER))

//
// Given the DB pointer, get the Reverse GlobalAuth
//
#define GET_REVERSE_GLOBAL_AUTH(_pZoneDB_)                                       \
    ( PISCSI_ZONE_AUTH_INFO)((csx_uchar_t *)GET_FORWARD_GLOBAL_AUTH(_pZoneDB_) + \
    sizeof(ISCSI_ZONE_AUTH_INFO))

//
// Checks to see if the global Auth is valid.
//
#define IS_GLOBAL_AUTH_VALID(_pGlobalAuth_)                                    \
    (_pGlobalAuth_->AuthNumber != INVALID_AUTH_NUMBER)
