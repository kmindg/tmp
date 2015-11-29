/*******************************************************************************
 * Copyright (C) EMC 2010 - 2011
 * All rights reserved.
 * Licensed material -- property of EMC
 ******************************************************************************/

#ifndef PS_EXPORTS_H
#define PS_EXPORTS_H

#include "EmcPAL.h"
#include <windows.h>
#include <stdio.h>
#include "k10defs.h"
#include "ktrace.h"
#include "UserDefs.h"
#include "../mgmt/interface/K10CommonExport.h"
#include "../layered/MLU/interface/mluTypes.h"
#include "FslTypes.h"
#include "FslAdminPerfData.h"

#ifdef PSLIB_EXPORT
#define PSLIB_API CSX_MOD_EXPORT
#else
#define PSLIB_API CSX_MOD_IMPORT
#endif


/* PS_MAX_CLIENT_NAME_WIDTH - Maximum length of the client name and buffer size.
 */
#define PS_MAX_CLIENT_NAME_WIDTH    8
#define PS_CLIENT_NAME_BUFFER_SIZE  (PS_MAX_CLIENT_NAME_WIDTH + 1)


/* PS_MAX_NICE_NAME_LEN - Maximum length for FSL Nice name in terms of bytes.
 * K10defs.h defines K10_LOGICAL_ID as an array of K10_LOGICAL_ID_LEN elementes
 * of unsinged short type.
 */
#define PS_MAX_NICE_NAME_LEN      (K10_LOGICAL_ID_LEN_NTWCHARS * sizeof(NTWCHAR))


/* PS_NICE_NAME_BUF_SIZE - Size of the buffer that can hold a FSL nice name.
 * 1 is added for NULL termination.
 */
#define PS_NICE_NAME_BUF_SIZE   (PS_MAX_NICE_NAME_LEN + 1)


/* PS_MAX_DEV_NAME_LEN - Maximum length of a MLU device object name.
 */
#define PS_MAX_DEV_NAME_LEN     (K10_DEVICE_ID_LEN - 1)


/* PS_DEV_NAME_BUF_SIZE - Buffer size of holding MLU device object name.
 */
#define PS_DEV_NAME_BUF_SIZE    (K10_DEVICE_ID_LEN)



/* PS_NICE_NAME_FSL_PREFIX - Nice name prefix for a FSL.
 */
#define PS_NICE_NAME_FSL_PREFIX         "FSL_"


/* PS_FSL_NICE_NAME_FIXED_PART - Length of the fixed FSL nice name prefix FSL_.
 * Note that -1 below is to take away the terminating NULL char.
 */
#define PS_NICE_NAME_FSL_PREFIX_LEN     (sizeof(PS_NICE_NAME_FSL_PREFIX) - 1)


/* Underlying MLU Nice name will in followig format
 * FSL_<client_name>_<supplied_nice_name>. +1 below is for '_'.
 */
#define PS_MAX_FSL_NAME_WIDTH      (PS_MAX_NICE_NAME_LEN -          \
                                    (PS_NICE_NAME_FSL_PREFIX_LEN +  \
                                     PS_MAX_CLIENT_NAME_WIDTH + 1))


/* PS_SEARCH_STR_BUF_SIZE - Max. length of the search string.
 */
#define PS_MAX_SEARCH_STR_LEN       PS_MAX_FSL_NAME_WIDTH


/* PS_FSL_NAME_BUFFER_SIZE - FSL Name buffer size.
 */
#define PS_FSL_NAME_BUFFER_SIZE     PS_MAX_FSL_NAME_WIDTH + 1


/* PS_SEARCH_STR_BUF_SIZE - Search string buffer size.
 */ 
#define PS_SEARCH_STR_BUF_SIZE      (PS_MAX_SEARCH_STR_LEN + 1)


/* PS_INVALID_HANDLE - Invalid handle.
 */
#define PS_INVALID_HANDLE           0


/* PS_ZF_BYTES_PER_BLOCK - Bytes per block for zero fill requests.
 */
#define PS_ZF_BYTES_PER_BLOCK       BYTES_PER_BLOCK


/* PS_SUCCESS - Checks the ps_status for success status.
 */
#define PS_SUCCESS(ps_status)   (ps_status == PS_STATUS_SUCCESS)


/* PS_ENQUEUED - Checks the ps_status for enqueued status.
 */
#define PS_ENQUEUED(ps_status)  (ps_status == PS_STATUS_REQUEST_ENQUEUED)


/* FSL_ADMIN - Defines a string litteral for FSL admin name.
 */
#define FSL_ADMIN               "K10FslAdmin"


/* PS_ASSERT - Pool service assert. DebugBreak in case of debug builds.
 */
#if DBG
#define PS_ASSERT(condition)            if(!(condition)) CSX_BREAK();
#else
#define PS_ASSERT 
#endif


/* HFSL - FSL handle.
 */
typedef ULONG       HFSL;


/* PsAlloc - Allocates memory from the default heap. Caller must check the
 * return pointer for possibility of it being NULL.
 */
#ifdef ALAMOSA_WINDOWS_ENV
#define PsAlloc(x)      HeapAlloc(GetProcessHeap(), 0, x)
#else
#define PsAlloc(x)      malloc(x)
#endif /* ALAMOSA_WINDOWS_ENV - COLLAPSE */


/* PsFree - Frees the specified memory from the default heap. If it fails to 
 * free a buffer, we only trace a warning message. There is no sense in failing
 * a request if it fails to free a buffer. Client need not check any return 
 * value.
 */
#ifdef ALAMOSA_WINDOWS_ENV
#define PsFree(x)       if(!HeapFree(GetProcessHeap(), 0, x))              \
                        {                                                   \
                            PsTrace(PS_COMPONENT_NAME,                      \
                                    PS_TRACE_LEVEL_WARN,                    \
                                    TRC_K_USER,                             \
                                    "%s Line: %d PsFree failed. Error: %d", \
                                    __FUNCTION__, __LINE__, GetLastError());\
                        }
#else
#define PsFree(x)       free(x)
#endif /* ALAMOSA_WINDOWS_ENV - COLLAPSE */


/* PS_STATUS - Defines the various error codes used by Pool Service.
 */
typedef enum ps_status
{
    PS_STATUS_SUCCESS = 0,
    PS_STATUS_REQUEST_ENQUEUED,
    PS_STATUS_INVALID_PARAMETER,
    PS_STATUS_INVALID_CALL,
    PS_STATUS_NOT_IMPLEMENTED,
    PS_STATUS_ALREADY_EXISTS,
    PS_STATUS_BUSY,
    PS_STATUS_NOT_FOUND,
    PS_STATUS_BAD_HANDLE,
    PS_STATUS_NOT_ENOUGH_MEMORY,
    PS_STATUS_SYNCH_OBJ_CREATE_FAILED,
    PS_STATUS_SYNCH_OBJ_OP_FAILED,
    PS_STATUS_WAIT_FAILED,
    PS_STATUS_PIPE_BROKEN,
    PS_STATUS_RESPONSE_CODE_ERROR,
    PS_STATUS_LOCAL_SHARING_VIOLATION,
    PS_STATUS_PEER_SHARING_VIOLATION,
    PS_STATUS_NOT_IN_USE,
    PS_STATUS_DIFFERENT_USER,
    PS_STATUS_K10_EXCEPTION,
    PS_STATUS_FSL_OPEN_ERROR,
    PS_STATUS_FSL_IN_USE_ERROR,
    PS_STATUS_TIMEOUT,
    PS_STATUS_PEER_NOT_REACHABLE,
    PS_STATUS_BAD_FSL_STATE,
    PS_STATUS_GENERIC_FAILURE,
    PS_STATUS_FILE_CREATE_FAILED,
    PS_STATUS_SEND_IOCTL_FAILED,
    PS_STATUS_BAD_DEVICE_NAME,
    PS_STATUS_POOL_UNAVAILABLE,
    PS_STATUS_FSL_COUNT_EXCEEDED,
    PS_STATUS_LU_WWN_EXISTS,
    PS_STATUS_LU_SIZE_OUT_OF_RANGE,
    PS_STATUS_MLU_GENERIC_FAILURE,
    PS_STATUS_MAX_OBJECTS_REACHED,
    PS_STATUS_SERVICE_UNAVAILABLE,
    PS_STATUS_LIB_NOT_INIT,
    PS_STATUS_DISCOVERY_ERROR,
    PS_STATUS_FSL_ALREADY_CONSUMED,
    PS_STATUS_FSL_NOT_CONSUMED,
    PS_STATUS_ALREADY_INITIALIZED
}PS_STATUS, *PPS_STATUS;


/* PS_PIPE_MESSAGE_CODE - Various pipe message OpCodes.
 */
typedef enum ps_pipe_message_code
{
    PS_MSG_TEST_PIPE = 0,
    PS_MSG_HAND_SHAKE,
    PS_MSG_HAND_SHAKE_RESP,
    PS_MSG_CREATE_FSL,
    PS_MSG_CREATE_FSL_RESP,
    PS_MSG_OPEN_FSL,
    PS_MSG_OPEN_FSL_RESP,
    PS_MSG_USE_FSL,
    PS_MSG_USE_FSL_RESP,
    PS_MSG_UNUSE_FSL,
    PS_MSG_UNUSE_FSL_RESP,
    PS_MSG_DELETE_FSL,
    PS_MSG_DELETE_FSL_RESP,
    PS_MSG_CLOSE_FSL,
    PS_MSG_CLOSE_FSL_RESP,
    PS_MSG_DOES_FSL_EXIST,
    PS_MSG_DOES_FSL_EXIST_RESP,
    PS_MSG_FORMAT_FSL,
    PS_MSG_REGISTER_POOL_NOTIFICATION,
    PS_MSG_REGISTER_POOL_NOTIFICATION_RESP,
    PS_MSG_GET_POOL_COUNT,
    PS_MSG_GET_POOL_COUNT_RESP,
    PS_MSG_GET_POOL_PROPERTIES,
    PS_MSG_GET_POOL_PROPERTIES_RESP,
    PS_MSG_LIST_POOLS,
    PS_MSG_LIST_POOLS_RESP,
    PS_MSG_REG_FSL_NOTIF,
    PS_MSG_REG_FSL_NOTIF_RESP,
    PS_MSG_LIST_FSL,
    PS_MSG_LIST_FSL_RESP,
    PS_MSG_GET_FSL_COUNT,
    PS_MSG_GET_FSL_COUNT_RESP,
    PS_MSG_GET_FSL_PROPERTIES,
    PS_MSG_GET_FSL_PROPERTIES_RESP,
    PS_MSG_ASYNCH_CREATE_COMPLETE,
    PS_MSG_ASYNCH_BUILD_COMPLETE,    
    PS_MSG_ASYNCH_USE_COMPLETE,
    PS_MSG_ASYNCH_UNUSE_COMPLETE,
    PS_MSG_ASYNCH_DELETE_COMPLETE,
    PS_MSG_POOL_EVENT_NOTIFICATION,
    PS_MSG_FSL_STATE_NOTIFICATION,
    PS_MSG_PEER_EVENT_NOTIFICATION,
    PS_MSG_DUMP_DEBUG_DATA,
    PS_MSG_DUMP_DEBUG_DATA_RESP,
    PS_MSG_CHOKE_REQUEST_QUEUE,
    PS_MSG_CHOKE_REQUEST_QUEUE_RESP,
    PS_MSG_BUILD_FSL,
    PS_MSG_BUILD_FSL_RESP,
    PS_MSG_ZERO_FILL,
    PS_MSG_ZERO_FILL_RESP,
    PS_MSG_RESYNCH_FSL,
    PS_MSG_RESYNCH_FSL_RESP,
    PS_MSG_KILL_SERVICE,
    PS_MSG_CLEAR_SUSPECT_FLAG,
    PS_MSG_CLEAR_SUSPECT_FLAG_RESP,
    PS_MSG_GET_NOTIF_CLIENT_COUNT,
    PS_MSG_GET_NOTIF_CLIENT_COUNT_RESP,
    PS_MSG_GET_ACTIVE_CLIENT_COUNT,
    PS_MSG_GET_ACTIVE_CLIENT_COUNT_RESP,    
    PS_MSG_ERROR_RESP

} PS_PIPE_MESSAGE_CODE;


/* PS_PEER_MSG_CODE - Defines peer communication message codes.
 */
typedef enum _ps_peer_msg_code
{
    PS_PEER_MSG_INVALID = -1,
    PS_PEER_MSG_MIN,
    PS_PEER_MSG_USE,
    PS_PEER_MSG_USE_RESP,
    PS_PEER_MSG_UNUSE,
    PS_PEER_MSG_UNUSE_RESP,
    PS_PEER_MSG_DELETE,
    PS_PEER_MSG_DELETE_RESP,
    PS_PEER_MSG_CANCEL_DELETE,
    PS_PEER_MSG_CANCEL_DELETE_RESP,
    PS_PEER_MSG_MAX

} PS_PEER_MSG_CODE, *PPS_PEER_MSG_CODE;


/* PS_POOL_EVENT - Pool events.
 */
typedef enum ps_pool_event
{
    PS_POOL_EVENT_INVALID = 0,
    PS_POOL_EVENT_NO_CHANGE,
    PS_POOL_EVENT_CREATED,
    PS_POOL_EVENT_EXPUNGING,
    PS_POOL_EVENT_DELETED

} PS_POOL_EVENT, *PPS_POOL_EVENT;


/* PS_POOL_STATE - Defines the Pool Service Pool states.
 *
 * Note: This enum is one to one with MLU_ADMIN_STATE defined in mlutypes.h. 
 * If there is any change in MLU_ADMIN_STATE it must be reflected here.
 */
typedef enum ps_pool_state
{
    PS_POOL_STATE_INVALID      = 0,
    PS_POOL_STATE_INITIALIZING,
    PS_POOL_STATE_READY,
    PS_POOL_STATE_FAULTED,
    PS_POOL_STATE_OFFLINE,
    PS_POOL_STATE_DESTROYING,
    PS_POOL_STATE_EXPUNGING, 
    PS_POOL_STATE_DELETED

} PS_POOL_STATE, *PPS_POOL_STATE;


/* PS_FSL_NOTIFICATION_TYPE - FSL notification types.
 */
typedef enum ps_fsl_notification_type
{
    PS_FSL_STATE_NOTIFICATION   = 0,
    PS_FSL_PEER_EVENT_NOTIFICATION

} PS_FSL_NOTIFICATION_TYPE, *PPS_FSL_NOTIFICATION_TYPE;


/* PS_FSL_STATE - FSL states.
 */
typedef enum ps_fsl_state
{
    PS_FSL_STATE_INVALID = 0,
    PS_FSL_STATE_INIT,
    PS_FSL_STATE_READY,
    PS_FSL_STATE_READY_SUSPECT,
    PS_FSL_STATE_ACTIVE,
    PS_FSL_STATE_ACTIVE_SUSPECT,
    PS_FSL_STATE_DELETING,
    PS_FSL_STATE_STOPPING,
    PS_FSL_STATE_STOPPING_ERRORS,
    PS_FSL_STATE_UNAVAILABLE,
    PS_FSL_STATE_DELETE_REQUESTED,
    PS_FSL_STATE_MAX                /* This must be the last element of enum. */

} PS_FSL_STATE, *PPS_FSL_STATE;


/* PS_FSL_EVENT - FSL events
 */
typedef enum ps_fsl_event
{
    PS_FSL_EVENT_INVALID = 0,
    PS_FSL_EVENT_PEER_USE,
    PS_FSL_EVENT_PEER_UNUSE,
    PS_FSL_EVENT_LOCAL_USE,
    PS_FSL_EVENT_LOCAL_UNUSE,
    PS_FSL_EVENT_DELETE,
    PS_FSL_EVENT_POOL_EXPUNGING,
    PS_FSL_EVENT_MAX

} PS_FSL_EVENT, *PPS_FSL_EVENT;


/* PS_POOL_INFO - Pool service pool info structure.
 */
typedef struct ps_pool_info
{
    MLU_THIN_POOL_ID    PoolId;
    PS_POOL_STATE       PoolState;
} PS_POOL_INFO, *PPS_POOL_INFO;


/* PS_RECOVERY_POLICY - Recovery policy enum.
 */
typedef enum ps_recovery_policy
{
    PS_RECOVERY_POLICY_INVALID = 0,
    PS_RECOVERY_POLICY_NONE,
    PS_RECOVERY_POLICY_AUTO

} PS_RECOVERY_POLICY, *PPS_RECOVERY_POLICY;


/* PS_CREATE_FSL_PARAM - Contains the parameter for creating the FSL LU.
 */
typedef struct ps_create_fsl_param
{
    MLU_THIN_POOL_ID    PoolId;
    char                FslName[PS_NICE_NAME_BUF_SIZE];

    EMCPAL_LARGE_INTEGER       LogicalSize;
    EMCPAL_LARGE_INTEGER       AllocationSize;

    MLU_SP_OWNERSHIP    AllocationOwner;    
    MLU_SP_OWNERSHIP    DefaultOwner;

    MLU_TIER_PREFERENCE TieringPreference;
    MLU_LU_ENUM_TYPE    LunType;
    PS_RECOVERY_POLICY  RecoveryPolicy;

    BOOLEAN             IsNTFS;

} PS_CREATE_FSL_PARAM, *PPS_CREATE_FSL_PARAM;


/* PS_FSL_INFO - FSL information strucutre.
 */
typedef struct ps_fsl_info
{
    MLU_THIN_POOL_ID        PoolId;
    char                    FslName[PS_NICE_NAME_BUF_SIZE];
    char                    MluDevName[PS_DEV_NAME_BUF_SIZE];
    char                    FslDevName[PS_DEV_NAME_BUF_SIZE];    
    K10_WWID                WWN;

    PS_FSL_STATE            FslState;

    EMCPAL_LARGE_INTEGER           LogicalSize;
    EMCPAL_LARGE_INTEGER           AllocationSize;

    MLU_SP_OWNERSHIP        AllocationOwner;
    MLU_SP_OWNERSHIP        DefaultOwner;
    MLU_SP_OWNERSHIP        CurrentOwner;

    MLU_TIER_PREFERENCE     TieringPreference;
    MLU_LU_ENUM_TYPE        LunType;
    PS_RECOVERY_POLICY      RecoveryPolicy;

    BOOLEAN                 IsNTFS;

    char                    UserClientName[PS_CLIENT_NAME_BUFFER_SIZE];

    FSL_PERFORMANCE_DATA    PerformanceData;    

} PS_FSL_INFO, *PPS_FSL_INFO;


/* Function prototypes.
 */
char * PsStatusString(PS_STATUS StatusCode);
char * PsPoolEventString(PS_POOL_EVENT PoolEvent);
char * PsFslEventString(PS_FSL_EVENT FslEvent);
char * PsFslStateString(PS_FSL_STATE StateCode);
char * PsMluStateString(MLU_ADMIN_STATE StateCode);
char * PsTierPreferenceString(MLU_TIER_PREFERENCE TierPreference);
char * PsLunTypeString(MLU_LU_ENUM_TYPE LunType);
char * PsRecoveryPolicyString(PS_RECOVERY_POLICY RecoveryPolicy);
char * PsPoolStateString(PS_POOL_STATE PoolState);


/* Client specified callback function for Pool service Asynchrnous API callback.
 */
typedef void (*PsApiCallback_t) (void *Context, PS_STATUS Status);


/* Client specified callback function for Pool notification callback.
 */
typedef void (*PsPoolNotifCallback_t) (ULONGLONG PoolId, PS_POOL_EVENT PoolEvent);


/* Client specified callback function for service notification callback.
 */
typedef void (*PsSrvcNotifCallback_t) (BOOLEAN IsServiceAvailable);


/* Client specified callback function for fsl state notification callback.
 */
typedef void (*PsFSLNotifCallback_t) (void *Context, PS_FSL_NOTIFICATION_TYPE NofificationType, ULONG NotificationCode);


/* PsLib exported Pool Service API.
 */
PSLIB_API PS_STATUS PsLibInit(char *ClientName);
PSLIB_API PS_STATUS PsCreateFSL(PPS_CREATE_FSL_PARAM pFslParam, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsBuildFSL(PPS_CREATE_FSL_PARAM pFslParam, BOOLEAN ExclusiveUse, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsCreateBuildFSL(PPS_CREATE_FSL_PARAM pFslParam, BOOLEAN Build, BOOLEAN ExclusiveUse, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsOpenFSL(char *FslName, HFSL *hFsl);
PSLIB_API PS_STATUS PsCloseFSL(HFSL hFsl);
PSLIB_API PS_STATUS PsUseFSL(HFSL handle, BOOLEAN ExclusiveAccess, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsUnuseFSL(HFSL handle, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsDeleteFSL(char *FslName, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PsRegisterForPoolChangeNotification(PsPoolNotifCallback_t Callback);
PSLIB_API PS_STATUS PsGetPoolCount(ULONG *pPoolCount);
PSLIB_API PS_STATUS PsGetPoolPropreties(ULONGLONG PoolId, PPS_POOL_INFO pPoolInfo);
PSLIB_API PS_STATUS PsListPools(void *pBuffer, ULONG BufferSize, BOOLEAN FreshStart, ULONG *pListedPoolCount, BOOLEAN *pMoreData);
PSLIB_API PS_STATUS PsDoesFSLExist(char *FslName, BOOLEAN *pFslExists);

PSLIB_API PS_STATUS PsRegisterForFSLChangeNotification(HFSL hFsl, PsFSLNotifCallback_t Callback, void *Context);
PSLIB_API PS_STATUS PsListFSLs(ULONGLONG PoolId, char *SearchString, void *pBuffer, ULONG BufferSize, BOOLEAN FreshStart, ULONG *pListedFslCount, BOOLEAN *pMoreData);
PSLIB_API PS_STATUS PsGetFSLCount(ULONGLONG PoolId, char *SearchString, ULONG *pFslCount);
PSLIB_API PS_STATUS PsGetFSLPropreties(HFSL hFsl, PPS_FSL_INFO pFslLun);
PSLIB_API PS_STATUS PsClearSuspectFSL(HFSL hFsl);
PSLIB_API PS_STATUS PsSendZeroFill(HFSL hFsl, LBA_T StartLBA, LBA_T Length);
PSLIB_API PS_STATUS PsDumpDebugData(void);
PSLIB_API PS_STATUS PsChokeRequestQueue(BOOLEAN Choke);
PSLIB_API PS_STATUS PsKillService(void);
PSLIB_API PS_STATUS PsRegisterForServiceStateNotification(PsSrvcNotifCallback_t Callback);

PSLIB_API PS_STATUS PsFormatFSL(HFSL hFsl, PsApiCallback_t Callback, PVOID Context);
PSLIB_API PS_STATUS PSGrowFSL(HFSL hFsl, LBA_T NewSize);
PSLIB_API PS_STATUS PSShrinkFSL(HFSL hFsl, LBA_T NewSize);

#endif // PS_LIB_EXPORTS_H
