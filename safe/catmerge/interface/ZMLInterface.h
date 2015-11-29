//**********************************************************************
//.++
// FILE NAME:
//      ZMLInterface.h
//
// DESCRIPTION:
//      The Header file, to be used by the drivers, to link in the iSCSI 
//      Zone Manager Library.  
//
//  COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2000
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//
// REVISION HISTORY:
// 		02-Oct-06  kdewitt		Initial Port to 64 bit OS.
//      05-Sep-06  jporrua      Defined tracing levels
//      11-Apr-06  kdewitt      Breaking up the test modules into two 
//                              groups: ZM_MINIMAL_TEST, ZM_MAXIMUM_TEST
//      11-Nov-05  kdewitt      Moved structures needed by users of 
//                              zml.lib from zml.h to zmlinterface.h
//      16-Feb-05  kdewitt		Created.
//.--                                                                    
//**********************************************************************

#ifndef _ZMLINTERFACE_H_
#define _ZMLINTERFACE_H_

#include "iSCSIZonesInterface.h"
#include "spid_types.h"

//**********************************************************************
//.++
// CONSTANT:
//      ZM_MINIMAL_TEST
//      ZM_MAXIMUM_TEST
//      
//
// DESCRIPTION:
// Two "defines" are used to control the amount of code that is linked 
// into ZMD/ZML for testing:
// ZM_MINIMAL_TEST: This define allows the minimal test code necessary 
//                  to be compiled into ZMD/ZML needed to run the 
//                  ISCSISanity.pl test
// ZM_MAXIMUM_TEST: This define allows all test code to be compiled into 
//                  ZMD/ZML. This is necessary to support ZMDTest.cpp.  
//                  ZM_MAXIMUM_TEST requires that ZM_MINIMAL_TEST also
//                  be defined. 
// It is assumed that the additional code that is included when 
// going from ZM_MINIMAL_TEST to ZM_MAXIMUM_TEST will be deleted before
// First Customer Ship (FCS).
//
// REVISION HISTORY:
//      11-Apr-06  kdewitt      Created.
//.--
//**********************************************************************

#define ZM_MINIMAL_TEST
#define ZM_MAXIMUM_TEST


//**********************************************************************
//.++
// CONSTANTS:
//    	ZML_MAX_REGISTERED_CALLBACKS
//
// DESCRIPTION:
//      The maximum number of drivers that can register with the ZML 
//      for notification of change to the Zone DB. This number is 
//      somewhat arbitrary and limits the space in 
//      SMD_ISCSI_ZML_GLOBALS that can be used for storing the address
//      of callback routines. This should be more than adequate, for 
//      the foreseeable future, since currently there are only two 
//      drivers that plan to use the ZML.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define	ZML_MAX_REGISTERED_CALLBACKS	8

//**********************************************************************
//.++
// CONSTANTS:
//      ZML_TRC_LEVEL_NONE
//      ZML_TRC_LEVEL_FUNCTION_ENTRY
//      ZML_TRC_LEVEL_FUNCTION_EXIT
//      ZML_TRC_LEVEL_INFORMATION
//      ZML_TRC_LEVEL_TESTING
//      ZML_TRC_LEVEL_ALWAYS
//      ZML_DEFAULT_TRACE_FLAG
//      ZML_TRC_LEVEL_ALL
//
// DESCRIPTION:
//      Tracing levels.
//
// REVISION HISTORY:
//      30-Mar-07  jporrua    Changed the default tracing level.
//      10-Nov-06  jporrua    Added ZML_TRC_LEVEL_ALL and 
//                            ZML_TRC_LEVEL_NONE (159129)
//      11-Sep-06  jporrua    Created.
//.--
//**********************************************************************

#define ZML_TRC_LEVEL_NONE             0x00000000
#define ZML_TRC_LEVEL_FUNCTION_ENTRY   0x00000001  
#define ZML_TRC_LEVEL_FUNCTION_EXIT    0x00000002  
#define ZML_TRC_LEVEL_INFORMATION      0x00000004  
#define ZML_TRC_LEVEL_TESTING          0x00000008
#define ZML_TRC_LEVEL_ALWAYS           0x00000010
#define ZML_TRC_LEVEL_DEFAULT          0x00000014
#define ZML_TRC_LEVEL_ALL              0x0000001F

//**********************************************************************
//.++
// CONSTANTS:
//    	ZML_MINIMUM_GETNEXT_BUFFER_SIZE
//
// DESCRIPTION:
//      The minimum size buffer used for a GetNext call. 
//      This will insure a buffer large enough to describe one path.
//
// REVISION HISTORY:
//       24-Oct-06 kdewitt      Changed ISCSI_ZONE_AUTH_INFO to
//                              ZML_ZONE_AUTH_INFO (157701)
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************
#define ZML_MINIMUM_GETNEXT_BUFFER_SIZE                         \
                       ((sizeof (ZML_PRIVATE))               +  \
                        (sizeof (ZML_GETNEXT_BUFFER_HEADER)) +  \
                        (sizeof (ZML_ZONE_HEADER))           +  \
                        (sizeof (ZML_ZONE_AUTH_INFO))        +  \
                        (sizeof (ISCSI_ADDRESS)))  


//**********************************************************************
//++
// DESCRIPTION: 
//      This typedef describes the elements of an array that will hold
//      the Call Back Function Pointers. 
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      14-Mar-05  kdewitt		Created.
// --
//**********************************************************************

typedef ULONG_PTR (*CALL_BACK_FUNCTION_PTR)(void);


//**********************************************************************
//.++
// DESCRIPTION: 
//      This enumeration is used when calling ZMLInitialize() and when
//      Locking / Unlocking the ZoneDB. It will be used as a "caller ID" 
//      and the holder of the Zone DB Lock.
//
// MEMBERS:
//      ZmlUninitializedID      ZML has not been initialized
//      ZmlZoneDBUnlocked       Zone DB is not locked
//      ZmlZmdWorkerThreadId    Identifier for ZMDNotifyWorkerThread
//      ZmlZmdId                Identifier for ZMD
//      ZmlFediskId             Identifier for FEDISK
//      ZmlCmiscdId             Identifier for CMISCD
//
// REVISION HISTORY:
//      20-Feb-06  kdewitt      Added ZmlZoneDBUnlocked &
//                              ZmlZmdWorkerThreadId
//      15-Mar-05  kdewitt		Created.
//.--
//**********************************************************************

typedef enum _ZML_CALLER_ID
{
    ZmlUninitializedID          = 0,
    ZmlZoneDBUnlocked           = 1,
    ZmlZmdWorkerThreadId        = 2,
    ZmlZmdId                    = 3,
    ZmlFediskId                 = 4,
    ZmlCmiscdId                 = 5
}
ZML_CALLER_ID, *pZML_CALLER_ID;


//**********************************************************************
//++
// DESCRIPTION: 
//      This structure is an opaque (private) structure used by ZML for 
//      debugging and testing only. It holds several counters that are 
//      incremented during a GetNext cycle. A GetNext cycle is defined
//      as the period of time from when ZMLGetNextPathInfoForPort()
//      is called with bStart = TRUE until it returns 
//      pGNBHeader->bDone = TRUE.
//      
//
// MEMBERS:
//      TotalGetNextCount   This counter is for diagnostic purposes only.
//                          It will count the number of times "GetNext" is
//                          called during a GetNext Cycle. It will be set
//                          to zero at the start of a GetNext cycle
//                          (bStart = TRUE) and incremented for all other 
//                          GetNext calls (bStart = FALSE). 
//
//      TotalZoneCount      This counter is for diagnostic purposes only.
//                          It will count the total number of Zones 
//                          returned to the caller during a GetNext Cycle. 
//                          It will be set to zero at the start of a GetNext
//                          cycle.
//
//      TotalPathCount      This counter is for diagnostic purposes only.
//                          It will count the total number of Paths 
//                          returned to the caller during a GetNext Cycle. 
//                          It will be set to zero at the start of a GetNext
//                          cycle.
//
//      TotalDbChangeCount  This counter is for diagnostic purposes only.
//                          It will count the total number of times the 
//                          GetNext Cycle was restarted (do to a change in
//                          ZoneDB) during the complete GetNext cycle.
//                          
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
// --
//**********************************************************************
#pragma pack(push, ZML_GN_DIAGNOSTIC_DATA, 4)
typedef struct _ZML_GN_DIAGNOSTIC_DATA    
{
    ULONG32           TotalGetNextCount;
    ULONG32           TotalDbChangeCount;
    ULONG32           TotalZoneCount;
    ULONG32           TotalPathCount;
}ZML_GN_DIAGNOSTIC_DATA, *PZML_GN_DIAGNOSTIC_DATA;
#pragma pack(pop, ZML_GN_DIAGNOSTIC_DATA)


//**********************************************************************
//++
// DESCRIPTION: 
//      This structure is an opaque (private) structure used by ZML for  
//      tracking the GetNext cycle as it traverses the Zone DB. 
//
// MEMBERS:
//      GenerationNumber:                   This number indicates the version or 
//                                          generation of the Zone DB that is being
//                                          referenced during a particular “GetNext” 
//                                          cycle. The ZML will set it equal to the Zone
//                                          DB’s generation number when the “GetNext” 
//                                          cycle starts. The ZML will compare this  
//                                          number to the Zone DB’s generation number 
//                                          for all subsequent “GetNext” request for a
//                                          particular cycle. If it detects a change it
//                                          will update this GenerationNumber and 
//                                          restart the “GetNext” process.
//
//      pLastZDBZoneEntryHeader:            Points to the last ISCSI_ZONE_ENTRY_HEADER 
//                                          structure that was processed in the Zone DB.
//                          
//      pLastZDBZoneEntryHeader_user64:     64bit place holder to ensure the struct size remains
//                                          the same between user and kernel space
//
//      pLastZDBZonePath:                   Points to the last ISCSI_ZONE_PATH structure 
//                                          to  be processed next in the Zone DB.
//
//      pLastZDBZonePath_user64:            64bit place holder to ensure the struct size remains
//                                          the same between user and kernel space
//
//      Diag                                This structure is used for testing and debug
//                                          only.
//                          
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
// --
//**********************************************************************
#define ZML_PRIVATE ZML_GETNEXT_TRACKING

#pragma pack(push, ZML_GETNEXT_TRACKING, 4)
typedef struct _ZML_GETNEXT_TRACKING    
{
    ULONG32                     GenerationNumber;
    union
    {
        PISCSI_ZONE_ENTRY_HEADER    pLastZDBZoneEntryHeader;
        DWORD64                     pLastZDBZoneEntryHeader_user64;
    };
    union
    {
        PISCSI_ZONE_PATH            pLastZDBZonePath;
        DWORD64                     pLastZDBZonePath_user64;
    };
    ZML_GN_DIAGNOSTIC_DATA      Diag;
}
ZML_GETNEXT_TRACKING, *PZML_GETNEXT_TRACKING;
#pragma pack(pop, ZML_GETNEXT_TRACKING)

//**********************************************************************
//++
// DESCRIPTION: 
//      This structure defines the authentication information used in  
//      the GetNext buffer. 
//
// MEMBERS:
//      pReverseGlobalAuth:         Pointer to the Cached Reverse Global Auth Info.
//                                  This cached data will be in the 
//                                  ZML_GETNEXT_BUFFER_HEADER.
//
//      pReverseGlobalAuth_user64:  64bit place holder to ensure the struct size remains
//                                  the same between user and kernel space
//
//      AuthInfo:                   defines the authentication information 
//                                  within a Zone as well the Global 
//                                  Authentication Information defined on 
//                                  the array
//
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      24-Oct-06  kdewitt      Created (157701)
// --
//**********************************************************************
#pragma pack(push, ZML_ZONE_AUTH_INFO, 4)
typedef struct _ZML_ZONE_AUTH_INFO 
{
    union
    {
        PVOID                      pReverseGlobalAuth;
        DWORD64                    pReverseGlobalAuth_user64;
    };
    ISCSI_ZONE_AUTH_INFO           AuthInfo;
}
ZML_ZONE_AUTH_INFO, *PZML_ZONE_AUTH_INFO;
#pragma pack(pop, ZML_ZONE_AUTH_INFO)

//**********************************************************************
//++
// DESCRIPTION: 
//      This structure is used to describe the state of the GetNext 
//      buffer.
//
// MEMBERS:
//		bDone:      True = ZML has returned all path information for 
//                  this GetNext Cycle.
//
//                  False = Additional GetNext cycles are needed to retrieve 
//                  all path information
//
//		ZoneCount:  The Zone count for this GetNext cycle.
//
//      ReverseGlobalAuth:
//                  Reverse Global Authentication will be used for all
//                  remote authentication requests. There will be a 
//                  pointer to this cached data in the 
//                  ZML_ZONE_AUTH_INFO structure. This will allow 
//                  ZMD to respond to remote authentication request
//                  without getting a lock or replicating the same
//                  Global Reverse authentication for each zone.
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      24-Oct-06 kdewitt       Changed ISCSI_ZONE_AUTH_INFO to
//                              ZML_ZONE_AUTH_INFO (157701)
//      21-Dec-05  kdewitt      Added ReverseGlobalAuth
//      12-Oct-04  kdewitt		Created.
// --
//**********************************************************************

typedef struct _ZML_GETNEXT_BUFFER_HEADER 
{
    csx_bool_t              bDone;  
    USHORT                  ZoneCount;
    ZML_ZONE_AUTH_INFO      ReverseGlobalAuth;
}
ZML_GETNEXT_BUFFER_HEADER, *PZML_GETNEXT_BUFFER_HEADER;



//**********************************************************************
//++
// DESCRIPTION: 
//      This structure is used to describe the path and authentication 
//      information being returned for one zone in the GetNext buffer.
//
// MEMBERS:
//      ZoneId
//		bAuthInfoEnabled:   True = ZML has returned all path information 
//                          for this GetNext Cycle.
//
//                          False = Additional GetNext cycles are needed  
//                          to retrieve all path information
//
//		PathCount:          The Zone count for this GetNext cycle.
//
//      HeaderDigest:       A flag used to specify whether the header
//                          digest will be used to protect the integrity 
//                          of the iSCSI header.  
//
//      DataDigest:         A flag used to specify whether the data 
//                          digest will be used to protect the 
//                          integrity of the iSCSI data.  
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      17-Jan-06  kdewitt      Added ZoneId and ZMLGetSinglePathInfo
//      12-Oct-04  kdewitt		Created.
// --
//**********************************************************************

typedef struct _ZML_ZONE_HEADER 
{
    ULONG32                 ZoneId;
    csx_bool_t              bAuthInfoEnabled;
    USHORT                  PathCount;
    UCHAR                   HeaderDigest;
    UCHAR                   DataDigest;
}
ZML_ZONE_HEADER, *PZML_ZONE_HEADER;


//**********************************************************************
//.++
// PROTOTYPES:
//
// DESCRIPTION:
//      Prototypes of the ZML exported functions 
//
// REVISION HISTORY:
//      19-Jan-06  kdewitt      Added ZMLGetSinglePathInfo
//      15-Feb-05  kdewitt		Created.
//.--
//**********************************************************************

EMCPAL_STATUS
ZMLInitialize( ZML_CALLER_ID );

EMCPAL_STATUS
ZMLReadZoneDBFromPSM( VOID );

EMCPAL_STATUS
ZMLGetNextPathInfoForPort ( IN  USHORT              FePortIndex,
                            IN  USHORT              VirtPortIndex,
                            IN  SP_ID               SpId,
                            IN  csx_bool_t          bStart,
                            IN  ULONG32             GNBSize,
                            IN  OUT PVOID           pGNB
                            );

EMCPAL_STATUS
ZMLGetSinglePathInfo (      IN  USHORT              FePortIndex,
                            IN  USHORT              VirtPortIndex,
                            IN  SP_ID               SpId,
                            IN  ISCSI_ZONE_ID       ZoneID,
                            IN  PISCSI_ADDRESS      pAddress,
                            IN  USHORT              PathNumber,
                            IN  ULONG32             GNBSize,
                            IN  OUT PVOID           pGNB
                            );

EMCPAL_STATUS
ZMLRegisterDataBaseChangeNotification( IN CALL_BACK_FUNCTION_PTR  pCallback );

EMCPAL_STATUS
ZMLDeRegisterDataBaseChangeNotification( IN  CALL_BACK_FUNCTION_PTR pCallback );



#endif  //_ZMDCONTROL_H_
