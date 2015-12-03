#ifndef __IdmInterface__
#define __IdmInterface__

//***************************************************************************
//
//  Copyright (C) EMC Corporation 2002-2003
//  Licensed material -- property of EMC Corporation
//
//**************************************************************************/

//++
// 
//  File Name: IdmInterface.h     
//
//  Contents:
//      This class provides Admin the tools to create an interface 
//      request to be sent to the driver via an IOCTL call.  The Driver 
//      will cast the buffer sent in the IOCTL to this class and use
//      its methods to parse the interface request.        
//
//  Revision History:
//--

#include "k10defs.h"
#include "core_config.h"
#include "EmcPAL_Ioctl.h"


#define IdmDriverName                           L"Idm"

#define IdmUserDriverName                       "Idm"

#define IdmDeviceString                         "\\Device\\Idm"

#define IdmSymbolicString                       "\\DosDevices\\Idm"


#define FILE_DEVICE_IDM                         77000

#define IOCTL_INDEX_IDM_BASE                    7700

#ifndef IDM_CTL_CODE

#define IDM_CTL_CODE( DeviceType, Function, Method, Access )                    \
                      ( ( ( DeviceType ) << 16 ) |                              \
                      ( ( Access ) << 14 ) |                                    \
                      ( ( Function ) << 2 ) | ( Method ) )
#endif

#define IDM_BUILD_IOCTL( _IoctlValue_ )                                         \
                      IDM_CTL_CODE( FILE_DEVICE_IDM,                            \
                                    ( IOCTL_INDEX_IDM_BASE + _IoctlValue_ ),    \
                                    EMCPAL_IOCTL_METHOD_BUFFERED,                            \
                                    EMCPAL_IOCTL_FILE_ANY_ACCESS )


#define IDM_DLS_PREFIX                          "IdmDlsName"

#define IDM_DLS_NAME_SIZE                       10


//
//  The lower order two bytes are used for the Driver version
//
#define IDM_DRIVER_VERSION                      0x00000001

#define TAGVALUE_IDM_CFG_DRV_OBJ                "IdmConfigDriverObject"

typedef enum _IDM_IOCTL_INDEX
{
    IDM_SET,

    IDM_LOOK_UP_BY_WWN,

    IDM_LOOK_UP_BY_ID,

    IDM_ASSIGN_ID,

    IDM_CHANGE_ID,

    IDM_RELEASE_ID,
    IDM_TABLE_SIZE,
    IDM_GET_TABLE,
    IDM_DISPLAY_TABLE,
    IDM_RELEASE_TABLE,
    IDM_DELETE_DB_FILE,
    IDM_READ_DB_FILE,
    IDM_SET_DEBUG_LEVEL,
    IDM_DH_UPDATE_MEDIA_ERROR_LIMITS,
    IDM_RELEASE_IDS
} IDM_IOCTL_INDEX;

#define IDM_DELETE_LIST_SIZE                    10

#define IOCTL_IDM_SET                           IDM_BUILD_IOCTL( IDM_SET )

#define IOCTL_IDM_LOOK_UP_BY_WWN                IDM_BUILD_IOCTL( IDM_LOOK_UP_BY_WWN )

#define IOCTL_IDM_LOOK_UP_BY_ID                 IDM_BUILD_IOCTL( IDM_LOOK_UP_BY_ID )

#define IOCTL_IDM_ASSIGN_ID                     IDM_BUILD_IOCTL( IDM_ASSIGN_ID )

#define IOCTL_IDM_CHANGE_ID                     IDM_BUILD_IOCTL( IDM_CHANGE_ID )

#define IOCTL_IDM_RELEASE_ID                    IDM_BUILD_IOCTL( IDM_RELEASE_ID) 

#define IOCTL_IDM_TABLE_SIZE                    IDM_BUILD_IOCTL( IDM_TABLE_SIZE )

#define IOCTL_IDM_GET_TABLE                     IDM_BUILD_IOCTL( IDM_GET_TABLE )

#define IOCTL_IDM_DISPLAY_TABLE                 IDM_BUILD_IOCTL( IDM_DISPLAY_TABLE )

#define IOCTL_IDM_RELEASE_TABLE                 IDM_BUILD_IOCTL( IDM_RELEASE_TABLE )

#define IOCTL_IDM_DELETE_DATABASE_FILE          IDM_BUILD_IOCTL( IDM_DELETE_DB_FILE )

#define IOCTL_IDM_READ_DATABASE_FILE            IDM_BUILD_IOCTL( IDM_READ_DB_FILE )

#define IOCTL_SET_DEBUG_LEVEL                   IDM_BUILD_IOCTL( IDM_SET_DEBUG_LEVEL )

#define IOCTL_IDM_DH_UPDATE_MEDIA_ERROR_LIMITS IDM_BUILD_IOCTL( IDM_DH_UPDATE_MEDIA_ERROR_LIMITS )

#define IOCTL_IDM_RELEASE_IDS                    IDM_BUILD_IOCTL( IDM_RELEASE_IDS )

typedef enum IdmStatus
{
    IDM_NO_STATUS,

    IDM_SET_SUCCESS,

    IDM_LOOKUP_BY_WWN_SUCCESS,

    IDM_LOOKUP_BY_ID_SUCCESS,

    IDM_ASSIGN_ID_SUCCESS,

    IDM_CHANGE_ID_SUCCESS,

    IDM_RELEASE_ID_SUCCESS,

    IDM_TABLE_SIZE_SUCCESS,

    IDM_GET_TABLE_SUCCESS,

    IDM_DISPLAY_TABLE_SUCCESS,

    IDM_RELEASE_TABLE_SUCCESS,

    IDM_READ_DB_SUCCESS,

    IDM_DELETE_DB_SUCCESS,

    IDM_SET_FAIL,

    IDM_LOOKUP_BY_WWN_FAIL,

    IDM_LOOKUP_BY_ID_FAIL,

    IDM_ASSIGN_ID_FAIL,

    IDM_CHANGE_ID_FAIL,

    IDM_RELEASE_ID_FAIL,

    IDM_READ_DB_FAIL,

    IDM_TABLE_SIZE_FAIL,

    IDM_GET_TABLE_FAIL,

    IDM_DISPLAY_TABLE_FAIL,

    IDM_DELETE_TABLE_FAIL,

    IDM_DELETE_DB_FAIL,

    IDM_NO_MEMORY_AVAILABLE, 

    IDM_WWID_EXISTS_IN_LIST, 

    IDM_LUNID_EXISTS_IN_LIST, 

    IDM_WWID_DOES_NOT_EXIST_IN_LIST, 

    IDM_LUNID_DOES_NOT_EXIST_IN_LIST, 

    IDM_NO_MEMORY_AVAILABLE_FOR_NEW_ASSOCIATION,

    IDM_EMPTY_TABLE,

    IDM_INVALID_LUN_ID,

    IDM_INVALID_RANGE_SPECIFIER,

    IDM_NO_LUNIDS_AVAILABLE,

    IDM_READ_PSM_ERROR,

    IDM_WRITE_PSM_ERROR,

    IDM_EMPTY_PSM_FILE,

    IDM_INSUFFICIENT_RESOURCES,

    IDM_LIST_ERROR_SELF_TEST,

    IDM_SET_DEBUG_LEVEL_SUCCESS

} IDMSTATUS, *PIDMSTATUS;


typedef enum IdmRange 
{
    IDM_NO_RANGE,

    IDM_LOW,

    IDM_HIGH

} IDMRANGE, *PIDMRANGE;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

typedef struct IdmBaseStatus 
{
    IDMSTATUS                                   status;

} IDMBASESTATUS, *PIDMBASESTATUS;


typedef struct IdmBaseStruct 
{
    K10_WWID                                    wwid;

    LONG                                        lunid;

} IDMBASE, *PIDMBASE;
  

//
//  use in idm_set
//
typedef struct IdmSetStruct 
{
    IDMSTATUS                                   status;

    K10_WWID                                    wwid;

    LONG                                        lunid;

} IDMSET, *PIDMSET;


//
//  use in idm_look_up functions
//
typedef struct IdmLookUpStruct 
{
    K10_WWID                                    wwid;

    LONG                                        lunid;

    IDMSTATUS                                   status;

} IDMLOOKUP, *PIDMLOOKUP;


//
//  use in idm_assign/change functions
//
typedef struct IdmModifyStruct 
{
    K10_WWID                                    wwid;

    LONG                                        lunid;

    IDMSTATUS                                   status;

    IDMRANGE                                    range;

} IDMMODIFY, *PIDMMODIFY;


//
//  use in idm_assign/change functions
//
typedef struct IdmGetTableStruct 
{
    IDMSTATUS                                   status;

    ULONG                                       size;

    IDMBASE                                     IDMPair[ 1 ];

} IDMGETTABLE, *PIDMGETTABLE;

//
//  use in idm_release function
//
typedef struct IdmReleaseIdStruct 
{
    K10_WWID                                    wwid;

    LONG                                        lunid;

    IDMSTATUS                                   status;

} IDMRELEASEID, *PIDMRELEASEID;


//
//  use in idm_release multiple WWNs function
//
typedef struct IdmReleaseIdsStruct 
{
    K10_WWID                                    wwid[IDM_DELETE_LIST_SIZE];

    ULONG					NumberWWNsToRelease;

    IDMSTATUS                                   status;

} IDMRELEASEIDS, *PIDMRELEASEIDS;


//
//  use in idm_table_size function
//
typedef struct IdmTableSizeStruct 
{
    LONG                                        size;

    IDMSTATUS                                   status;

} IDMOJECTTABLESIZE, *PIDMOBJECTTABLESIZE;


//
//  use in idm_release_table function
//
typedef struct IdmReleaseTableStruct 
{
    IDMSTATUS                                   status;

} IDMRELEASETABLE, *PIDMRELEASETABLE;


//
//  use in idm_release_db_file function
//
typedef struct IdmReleaseDB 
{
    IDMSTATUS                                   status;

} IDMRELEASEDB, *PIDMRELEASEDB;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
   
#endif


