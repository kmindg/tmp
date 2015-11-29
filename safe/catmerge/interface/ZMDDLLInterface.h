//**********************************************************************
//.++
// FILE NAME:
//      ZMDDLLInterface.h
//
// DESCRIPTION:
//      Definitions of exported data structures used by the DLL 
//      to interface with the ZMD
//
//  COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2000 - 2012
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//
// REVISION HISTORY:
// 		02-Oct-06  kdewitt		Initial Port to 64 bit OS.
//      15-Nov-05  kdewitt		Added support for 
//                              ZMD_IOCTL_REFRESH_ZONE_DB_FROM_PSM
//      12-Oct-04  kdewitt		Created.
//.--                                                                    
//**********************************************************************

#ifndef _ZMDDLLINTERFACE_H_
#define _ZMDDLLINTERFACE_H_

#include <stddef.h>
//**********************************************************************
//.++
// CONSTANTS:
//      ZONEMANAGER_DRIVER_NAME
//
// DESCRIPTION:
//      
//      This constant defines the name of this driver as it will be 
//      accessed by the ZoneManagerAdminLib.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZONEMANAGER_DRIVER_NAME     "ZoneManager"

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_VERSION_NUMBER
//
// DESCRIPTION:
//      The definition of the current version of the Zone Manager Driver.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_VERSION_NUMBER 0x001

//**********************************************************************
//.++
// CONSTANTS:
//		ZMD_DEVICE_TYPE
//
// DESCRIPTION:
//		This constant defines a custom device type used by the Windows NT
//      CTL_CODE macro to define our custom IOCTLs. Any value in the 
//		range of 0x8000-0xFFFF is valid.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_DEVICE_TYPE                    0xE000   


//**********************************************************************
//.++
// CONSTANTS:
//
// DESCRIPTION:
//      This constants define the custom function codes used by the Windows NT
//      CTL_CODE macro to define our custom IOCTLs. Any value in the 
//      range of 0x800-0xFFF is valid.
//
// REVISION HISTORY:
//      12-May-06  kdewitt      Added ZMD_IOCTL_NOTIFY_REGISTERED_DRIVERS_FUNCTION_CODE 
//      08-Mar-06  kdewitt      Added ZMD_IOCTL_GET_GLOBAL_DATA_PTR 
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_GET_GLOBAL_DATA_PTR_FUNCTION_CODE               0x0900 
#define ZMD_IOCTL_NOTIFY_REGISTERED_DRIVERS_FUNCTION_CODE   0x0901 
#define ZMD_GET_ZONE_UVA_PTR_FUNCTION_CODE                  0x0902 
#define ZMD_LOCK_ZONE_DB_FUNCTION_CODE                      0x0903
#define ZMD_UNLOCK_ZONE_DB_FUNCTION_CODE                    0x0904
#define ZMD_DISPATCH_STATUS_CHANGE_FUNCTION_CODE            0x0905
#define ZMD_REFRESH_ZONE_DB_FROM_PSM_FUNCTION_CODE          0x0906
#define ZMD_TEST_FUNCTION_CODE                              0x0911

//**********************************************************************
//.++
// CONSTANT:
//      ZMD_ZONE_DB_SHM_NAME
//
// DESCRIPTION:
//      Zone database shared memory section name.
//
// REVISION HISTORY:
//      01-Sept-12  Swati Fursule      Created.
//.--
//**********************************************************************

#define ZMD_ZONE_DB_SHM_NAME        "zmd_zone_db_shm"

//**********************************************************************
//.++
// CONSTANT:
//      ZMD_ZONE_TOKENS_SHM_NAME
//
// DESCRIPTION:
//      Zone polling tokens shared memory section name.
//
// REVISION HISTORY:
//      06-May-13  Ren Ren      Created.
//.--
//**********************************************************************

#define ZMD_ZONE_TOKENS_SHM_NAME        "zmd_zone_tokens_shm"

//**********************************************************************
//.++
// CONSTANT:
//      ZMD_ZONE_COMMON_SHM_NAME
//
// DESCRIPTION:
//      izdaemon, izadmin and zmd common shared memory section name.
//
// REVISION HISTORY:
//      03-July-13  Ren Ren      Created.
//.--
//**********************************************************************

#define ZMD_ZONE_COMMON_SHM_NAME        "zmd_zone_common_shm"

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_GET_GLOBAL_DATA_PTR
//
// DESCRIPTION:
//      This IOCTL is used by ZML to get a pointer to ZMD's global
//      data structure (ZMD_GLOBALS)  used to manage the Zone DB.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_GET_GLOBAL_DATA_PTR EMCPAL_IOCTL_CTL_CODE(                     \
                            ZMD_DEVICE_TYPE,                        \
                            ZMD_GET_GLOBAL_DATA_PTR_FUNCTION_CODE,  \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                        \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_NOTIFY_REGISTERED_DRIVERS
//
// DESCRIPTION:
//      This IOCTL is used  to notify all registered drivers
//      that there has been a change to the Zone DB
//
// REVISION HISTORY:
//      12-May-06  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_NOTIFY_REGISTERED_DRIVERS EMCPAL_IOCTL_CTL_CODE(               \
                ZMD_DEVICE_TYPE,                                    \
                ZMD_IOCTL_NOTIFY_REGISTERED_DRIVERS_FUNCTION_CODE,  \
                EMCPAL_IOCTL_METHOD_BUFFERED,                                    \
                EMCPAL_IOCTL_FILE_ANY_ACCESS )

 
//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_GET_ZONE_UVA_PTR
//
// DESCRIPTION:
//      This IOCTL is used by the Zone DLL to get a User Virtual Address 
//      pointer to the Zone DB. 
//
//
// NOTE:
//      This IOCTL is used to just sync zone database manager with PSM.
//      Clients shall then use ZMD_ZONE_DB_SHM_NAME
//      shared memory section to get Zone DB ptr.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_GET_ZONE_UVA_PTR EMCPAL_IOCTL_CTL_CODE(                    \
                            ZMD_DEVICE_TYPE,                    \
                            ZMD_GET_ZONE_UVA_PTR_FUNCTION_CODE, \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                    \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_LOCK_ZONE_DB
//
// DESCRIPTION:
//      This IOCTL is used by the Zone DLL to lock the Zone DB 
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_LOCK_ZONE_DB EMCPAL_IOCTL_CTL_CODE(                        \
                            ZMD_DEVICE_TYPE,                    \
                            ZMD_LOCK_ZONE_DB_FUNCTION_CODE,     \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                    \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_UNLOCK_ZONE_DB
//
// DESCRIPTION:
//      This IOCTL is used by the Zone DLL to unlock the Zone DB 
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_UNLOCK_ZONE_DB EMCPAL_IOCTL_CTL_CODE(                      \
                            ZMD_DEVICE_TYPE,                    \
                            ZMD_UNLOCK_ZONE_DB_FUNCTION_CODE,   \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                    \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_DISPATCH_STATUS_CHANGE 
//
// DESCRIPTION:
//      This IOCTL is used by the Zone DLL to warn ZMD that it 
//      ability to communicate with its peer SP has changed.
//
// REVISION HISTORY:
//      12-Oct-04  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_DISPATCH_STATUS_CHANGE EMCPAL_IOCTL_CTL_CODE(                     \
                            ZMD_DEVICE_TYPE,                           \
                            ZMD_DISPATCH_STATUS_CHANGE_FUNCTION_CODE,  \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                           \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

                            
//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_REFRESH_ZONE_DB_FROM_PSM 
//
// DESCRIPTION:
//      This IOCTL is used by the Zone DLL to request that 
//      Zone DB be refreshed from the PSM
//
// REVISION HISTORY:
//      15-Nov-05  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_REFRESH_ZONE_DB_FROM_PSM EMCPAL_IOCTL_CTL_CODE(                    \
                            ZMD_DEVICE_TYPE,                            \
                            ZMD_REFRESH_ZONE_DB_FROM_PSM_FUNCTION_CODE, \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                            \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

                        

//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_IOCTL_TEST 
//
// DESCRIPTION:
//      This IOCTL is only used for testing
//
// REVISION HISTORY:
//      11-Mar-05  kdewitt		Created.
//.--
//**********************************************************************

#define ZMD_IOCTL_TEST EMCPAL_IOCTL_CTL_CODE(                                            \
                            ZMD_DEVICE_TYPE,                                \
                            ZMD_TEST_FUNCTION_CODE,                         \
                            EMCPAL_IOCTL_METHOD_BUFFERED,                                \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )


//**********************************************************************
//.++
// CONSTANTS:
//      ZMD_ZONE_DB_BUFFER_OFFSET 
//       
// DESCRIPTION:
//      This constant defines the offset of Zone DB data in Zone DB shared
//      memory section.
//
// REVISION HISTORY:
//      18-Jan-11  ppurohit     Created.
//.--
//**********************************************************************
#define ZMD_ZONE_DB_BUFFER_OFFSET    (offsetof(PSM_READ_OUT_BUFFER, Buffer))


//**********************************************************************
//++
// DESCRIPTION: 
//      This structure is used to describe the input buffer
//      for ioctl ZMD_IOCTL_MSG_DISPATCH_STATUS_CHANGE
//
// MEMBERS:
//		DispatchThreadActive:      
//                  A Boolean used to indicate the DLL's ability to
//                  communicate with its peer SP.
//
//                  FALSE = The DLL can no longer communicate
//                  with the peer SP. ZMD will notify the ZML that
//                  if needs to refresh the Zone DB from the PSM for 
//                  every GetNext request.
//
//                  TRUE = The DLL has re-established 
//                  communication with the peer SP. ZMD will 
//                  notify ZML that it no longer needs to refresh
//                  the Zone DB for every GetNext request.
//
// REMARKS: 
//       
//
// REVISION HISTORY:
//      16-Nov-05  kdewitt		Created.
// --
//**********************************************************************

typedef struct _ZMD_IOCTL_MSG_DISPATCH_STATUS_CHANGE_INBUF    
{
    csx_bool_t     DispatchThreadActive;
}
ZMD_IOCTL_MSG_DISPATCH_STATUS_CHANGE_INBUF, 
*PZMD_IOCTL_MSG_DISPATCH_STATUS_CHANGE_INBUF;
                                

#define ISCSI_ZONE_CHANGED_TOKEN_MAX_SIZE 8
#define ISCSI_ZONE_DELETED_TOKEN_MAX_SIZE 5

//**********************************************************************
//++
// DESCRIPTION: 
//      This structure defines the zone id/token pair information.
//
// MEMBERS:
//      ZoneID is used to indicate the ID of the specific Zone.
//      Token is used to record the change of the specific Zone.
//
// REMARKS: 
//
// REVISION HISTORY:
//      6-May-13  renr2        Created.
// --
//**********************************************************************
#pragma pack(push, ISCSI_ZONE_TOKEN_PAIR, 1)
typedef struct _ISCSI_ZONE_TOKEN_PAIR
{
    ISCSI_ZONE_ID       ZoneID;
    ULONG32             Token;
} ISCSI_ZONE_TOKEN_PAIR,
*PISCSI_ZONE_TOKEN_PAIR;
#pragma pack(pop, ISCSI_ZONE_TOKEN_PAIR)


//**********************************************************************
//++
// DESCRIPTION: 
//      The ZMD_GLOBAL_ISCSI_ZONE_TOKENS structure defines the token information 
// for delta polling..
//
// MEMBERS:
//      ZoneDBToken is used to record any change of Zone Database. 
//          Because there is one and only one Zone Database, just use one token for it.
//      
//      GlobalAuthToken is used to record any change (including add operation) of Global
//          Authentication information. There are only two type of Global Authentication: 
//          Forward Global Authentication and Reverse Global Authentication.
//      
//      GlobalAuthDeletedToken is used to record the deletion of the Global Authentication.
//      
//      ZoneChangedToken is an array of ISCSI_ZONE_TOKEN_PAIR and used to recode any 
//          change (including add operation) of a specific Zone.
//      
//      ZoneDeletedToken is an array of ISCSI_ZONE_TOKEN_PAIR and used to record the deletion
//          of a specific Zone.
//  
//      GlobalToken is used to keep the latest value of changed token.
//
// REMARKS: 
//
// REVISION HISTORY:
//      6-May-13  renr2        Created.
// --
//**********************************************************************

#pragma pack(push, ZMD_GLOBAL_ISCSI_ZONE_TOKENS, 1)
typedef struct _ZMD_GLOBAL_ISCSI_ZONE_TOKENS
{
    ULONG32                 ZoneDBToken;
    ULONG32                 ForwardGlobalAuthToken;
    ULONG32                 ForwardGlobalAuthDeletedToken;
    ULONG32                 ReverseGlobalAuthToken;
    ULONG32                 ReverseGlobalAuthDeletedToken;
    ISCSI_ZONE_TOKEN_PAIR   ZoneChangedToken[ISCSI_ZONE_CHANGED_TOKEN_MAX_SIZE];
    ISCSI_ZONE_TOKEN_PAIR   ZoneDeletedToken[ISCSI_ZONE_DELETED_TOKEN_MAX_SIZE];
    ULONG32                 GlobalToken;
    BOOL                    IsTokensPrecocious;
} ZMD_GLOBAL_ISCSI_ZONE_TOKENS, 
* PZMD_GLOBAL_ISCSI_ZONE_TOKENS;
#pragma pack(pop, ZMD_GLOBAL_ISCSI_ZONE_TOKENS)

//**********************************************************************
//++
// DESCRIPTION: 
//      The ZMD_GLOBAL_COMMON_SHARED_INFO structure defines the shared information 
// for izdaemon, izadmin and zmd.
//
// MEMBERS:
//      bMsgDispatchActive - It indicates whether the message dispatch mechanism 
// used by the DLL to communicate with the peer has been initialized.
//      nOpenZMDFailures - This varible keeps track of how many open Zone Manager 
// driver errors we have traced since the last successful one.
// 
// REMARKS: 
//
// REVISION HISTORY:
//      2-July-13  renr2        Created.
// --
//**********************************************************************

#pragma pack(push, ZMD_GLOBAL_COMMON_SHARED_INFO, 1)
typedef struct _ZMD_GLOBAL_COMMON_SHARED_INFO
{
    int                     nOpenZMDFailures;
    csx_bool_t              bMsgDispatchActive;
} ZMD_GLOBAL_COMMON_SHARED_INFO, 
* PZMD_GLOBAL_COMMON_SHARED_INFO;
#pragma pack(pop, ZMD_GLOBAL_COMMON_SHARED_INFO)


#endif  //_ZMDDLLINTERFACE_H_

