;//******************************************************************************
;//      COMPANY CONFIDENTIAL:
;//      Copyright (C) EMC Corporation, 2006
;//      All rights reserved.
;//      Licensed material - Property of EMC Corporation.
;//******************************************************************************
;
;//**********************************************************************
;//.++
;// FILE NAME:
;//      K10iSCSIZonesMessages.mc
;//
;// DESCRIPTION:
;//      This file defines iSCSIZonesAdmin DLL's Status Codes and
;//      messages. 
;//
;// REMARKS:
;//      This file is shared by ZonesAdmin DLL, ZML and ZMD
;//      The DLL error codes will be from 0x715Cx000 to 0x715Cx4FF
;//      The ZMD error codes will be from 0x715Cx500 to 0x715Cx9FF
;//      The ZML error codes will be from 0x715CxA00 to 0x715CxFFF
;//
;// REVISION HISTORY:
;//	 01-Oct-08  miranm	 Added virtual ports error codes.
;//      17-Aug-06  jporrua      Added support to force unlock the 
;//                              Zone DB (152075)
;//      20-Jun-06  kdewitt      Added support to unload the driver if
;//                              no ISCSI FE ports are found (148904)
;//
;//.--                                                                    
;//**********************************************************************
;#ifndef ISCSI_MESSAGES_H
;#define ISCSI_MESSAGES_H

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( System       = 0x0
                    RpcRuntime   = 0x2:FACILITY_RPC_RUNTIME
                    RpcStubs     = 0x3:FACILITY_RPC_STUBS
                    Io           = 0x4:FACILITY_IO_ERROR_CODE
                    ISCSI_ZONES  = 0x15C:FACILITY_ISCSI_ZONES 
                  )

SeverityNames   = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                    Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                    Warning       = 0x2 : STATUS_SEVERITY_WARNING
                    Error         = 0x3 : STATUS_SEVERITY_ERROR
                  )

;//-----------------------------------------------------------------------------
;//  Informational Status Codes
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
MessageId       = 0x0001
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_REQUEST_ON_COMPOSITE_OBJECT
Language        = English
Request for a Composite object.
.
;//
;//
;
;#define   IZ_ADMIN_STATUS_REQUEST_ON_COMPOSITE_OBJECT \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_REQUEST_ON_COMPOSITE_OBJECT)
;//-----------------------------------------------------------------------------
;
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Informational
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_TAG_LOGICAL_NOT_FOUND
Language        = English
TAG_LOGICAL was not found when parsing the input tld request.
.
;
;#define  IZ_ADMIN_STATUS_TAG_LOGICAL_NOT_FOUND       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_TAG_LOGICAL_NOT_FOUND)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Informational
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_TAG_OBJECTS_NOT_FOUND
Language        = English
TAG_OBJECTS was not found when parsing for the input tld request.
.
;
;#define  IZ_ADMIN_STATUS_TAG_OBJECTS_NOT_FOUND       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_TAG_OBJECTS_NOT_FOUND)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
MessageId       = +1
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_DB_GENERATION_UNCHANGED
Language        = English
Received a get request with the same generation number as that of the database.
.
;
;#define   IZ_ADMIN_STATUS_DB_GENERATION_UNCHANGED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_DB_GENERATION_UNCHANGED)
;
;//
;//
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event log entry
;//
;// Severity: Informational
;//
;// Symptom of Problem:
MessageId       = 0x0500
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_DRIVER_LOADED
Language        = English
The ZMD driver has been successfully loaded (%2).
.
;#define   ZMD_ADMIN_STATUS_DRIVER_LOADED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_DRIVER_LOADED)
;
;//
;//
;
;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Warning
;//
;// Symptom of Problem
;//MessageId       = 0x4000
;//Severity        = Warning
;//Facility        = ISCSI_ZONES
;//SymbolicName    = 
;//Language        = English
;//
;//.

;
;//#define \
;//        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR()
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Warning
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Warning
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ZONEDB_LOCKED
Language        = English
Internal information only. An attempt was made to access the Zone DB while it was locked.
.
;
;#define  ZML_ADMIN_STATUS_ZONEDB_LOCKED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ZONEDB_LOCKED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Warning
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Warning
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ZONEDB_FORCE_UNLOCKED
Language        = English
Internal information only. The Zone DB has been unlocked by force.
.
;
;#define  ZML_ADMIN_STATUS_ZONEDB_FORCE_UNLOCKED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ZONEDB_FORCE_UNLOCKED)
;
;//
;//
;
;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = 0x8000
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_TLD_ERROR
Language        = English
Invalid TLD formation.
.
;
;#define  IZ_ADMIN_STATUS_TLD_ERROR\
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_TLD_ERROR)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_PARAMETER
Language        = English
An invalid parameter passed in for the given operation.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_PARAMETER\
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_PARAMETER)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NOT_ENOUGH_RESOURCES
Language        = English
Not enough resources were available to fulfill the request. The request can be tried later when other features or sessions have completed thereby making the resources available.%2.
.
;
;#define  IZ_ADMIN_STATUS_NOT_ENOUGH_RESOURCES   \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NOT_ENOUGH_RESOURCES)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_GLOBAL_AUTH_ALREADY_EXISTS
Language        = English
Global Authentication already exists.
.
;
;#define  IZ_ADMIN_STATUS_GLOBAL_AUTH_ALREADY_EXISTS\
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_GLOBAL_AUTH_ALREADY_EXISTS)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_GLOBAL_AUTHENTICATION_NOT_EXIST
Language        = English
Global Authentication does not exist.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_GLOBAL_AUTHENTICATION_NOT_EXIST\
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_GLOBAL_AUTHENTICATION_NOT_EXIST)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INTERNAL_ERROR
Language        = English
Internal Error.
.
;
;#define  IZ_ADMIN_STATUS_INTERNAL_ERROR\
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INTERNAL_ERROR)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_PATH_DESCRIPTION_INVALID_LENGTH
Language        = English
The connection path description provided was longer than the maximum of 256 characters. Please shorten the path description and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_PATH_DESCRIPTION_INVALID_LENGTH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_PATH_DESCRIPTION_INVALID_LENGTH)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_AUTHENTICATION_USAGE
Language        = English
Invalid Authentication Usage (% 2).
.
;
;#define  IZ_ADMIN_STATUS_INVALID_AUTHENTICATION_USAGE       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_AUTHENTICATION_USAGE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_AUTHENTICATION_TYPE
Language        = English
Invalid Authentication Type (% 2).
.
;
;#define  IZ_ADMIN_STATUS_INVALID_AUTHENTICATION_TYPE       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_AUTHENTICATION_TYPE)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_UNSUPPORTED_AUTHENTICATION_TYPE
Language        = English
Unsupported Authentication Type (% 2).
.
;
;#define  IZ_ADMIN_STATUS_SUPPORTED_AUTHENTICATION_TYPE       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_UNSUPPORTED_AUTHENTICATION_TYPE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_AUTHENTICATION_METHOD
Language        = English
Invalid Authentication Method (% 2).
.
;
;#define  IZ_ADMIN_STATUS_INVALID_AUTHENTICATION_METHOD       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_AUTHENTICATION_METHOD)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_USERNAME_LENGTH
Language        = English
The authentication user name length was too long. Please use a user name that is no more than 255 characters and try the request again.(% 2).
.
;
;#define  IZ_ADMIN_STATUS_INVALID_USERNAME_LENGTH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_USERNAME_LENGTH)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_PASSWORD_LENGTH
Language        = English
The authentication secret length was invalid. Please use a secret that is no more than 99 characters and try the request again. (% 2).
.
;
;#define  IZ_ADMIN_STATUS_INVALID_PASSWORD_LENGTH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_PASSWORD_LENGTH)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_USERNAME_ALREADY_DEFINED
Language        = English
Authentication Username Already Exists.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_USERNAME_ALREADY_DEFINED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_USERNAME_ALREADY_DEFINED)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_PASSWORD_ALREADY_DEFINED
Language        = English
Authentication Password Already Exists.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_PASSWORD_ALREADY_DEFINED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_PASSWORD_ALREADY_DEFINED)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_AUTHENTICATION_ENTRY_DOES_NOT_EXIST
Language        = English
Authentication entry does not exist.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_AUTHENTICATION_ENTRY_DOES_NOT_EXIST       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_AUTHENTICATION_ENTRY_DOES_NOT_EXIST)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NO_FE_PORTS_SPECIFIED_IN_ISCSI_ZONE_PATH
Language        = English
Cannot have a connection path without any front end ports. Please ensure that there is at least one front end port specified in the path.
.
;
;#define  IZ_ADMIN_STATUS_NO_FE_PORTS_SPECIFIED_IN_ISCSI_ZONE_PATH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NO_FE_PORTS_SPECIFIED_IN_ISCSI_ZONE_PATH)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NO_IP_IDENTIFIER_SPECIFIED_IN_ISCSI_ZONE_PATH
Language        = English
No target was specified while adding a new connection path. Please specify a target and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_NO_IP_IDENTIFIER_SPECIFIED_IN_ISCSI_ZONE_PATH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NO_IP_IDENTIFIER_SPECIFIED_IN_ISCSI_ZONE_PATH)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_FE_PORT_SP_ID
Language        = English
Invalid SP ID %2 specified for %3.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_FE_PORT_SP_ID       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_FE_PORT_SP_ID)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_FE_PORT_INDEX
Language        = English
Invalid FE Port %2 specified.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_FE_PORT_INDEX       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_FE_PORT_INDEX)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NAME_NOT_SPECIFIED_IN_ISCSI_ZONE
Language        = English
Connection set name was not specified. Please provide a connection set name and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_NAME_NOT_SPECIFIED_IN_ISCSI_ZONE       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NAME_NOT_SPECIFIED_IN_ISCSI_ZONE)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_NAME_LENGTH
Language        = English
The provided connection set name was longer (%2) than the maximum of 64 characters. Please use a connection set name 64 characters or shorter and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_NAME_LENGTH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_NAME_LENGTH)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_PATH_ENTRY_DOES_NOT_EXIST
Language        = English
No Path with Path Number (%2) exists.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_PATH_ENTRY_DOES_NOT_EXIST       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_PATH_ENTRY_DOES_NOT_EXIST)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_PATH_DESC_DOES_NOT_EXIST
Language        = English
No Path Description with Path Number (%2) exists
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_PATH_DESC_DOES_NOT_EXIST       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_PATH_DESC_DOES_NOT_EXIST)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_DATABASE_CORRUPT
Language        = English
Database corrupt.
.
;
;#define  IZ_ADMIN_STATUS_DATABASE_CORRUPT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_DATABASE_CORRUPT)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_NOT_EXIST
Language        = English
Zone does not exists.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_NOT_EXIST       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_NOT_EXIST)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_AUTH_ENTRY_DOES_NOT_EXIST
Language        = English
Zone authentication entry does not exists.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_AUTH_ENTRY_DOES_NOT_EXIST       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_AUTH_ENTRY_DOES_NOT_EXIST)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ISCSI_ZONE_NAME_ALREADY_EXISTS
Language        = English
The provided connection set name already exists in connection set database. Please use a different connection set name and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_ISCSI_ZONE_NAME_ALREADY_EXISTS       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ISCSI_ZONE_NAME_ALREADY_EXISTS)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_UNKNOWN_EXCEPTION
Language        = English
Unknown exception.
.
;
;#define  IZ_ADMIN_STATUS_UNKNOWN_EXCEPTION       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_UNKNOWN_EXCEPTION)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ERROR_DLSFAIL
Language        = English
DLS lock failure.
.
;
;#define  IZ_ADMIN_STATUS_ERROR_DLSFAIL       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ERROR_DLSFAIL)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event log.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_COULD_NOT_OPEN_ZMD
Language        = English
Internal Information Only. Could not open the connection set manager Driver. This could be because the storage system does not have any iSCSI ports or they are not setup to be initiators.
.
;
;#define  IZ_ADMIN_STATUS_COULD_NOT_OPEN_ZMD       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_COULD_NOT_OPEN_ZMD)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event Log.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_COULD_NOT_OPEN_FEDISK
Language        = English
Internal Information Only. Could not open FEDisk Driver due to an internal error. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_COULD_NOT_OPEN_FEDISK       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_COULD_NOT_OPEN_FEDISK)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_INIT_MSG_DISPATCH
Language        = English
Zone Name already exists.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_INIT_MSG_DISPATCH       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_INIT_MSG_DISPATCH)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_T0_GET_PEER_SP_ID
Language        = English
Failed to get the Peer SP ID.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_T0_GET_PEER_SP_ID       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_T0_GET_PEER_SP_ID)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_CONNECT_TO_PEER
Language        = English
Failed to connect to the peer.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_CONNECT_TO_PEER       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_CONNECT_TO_PEER)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_SEND_MSG
Language        = English
Error trying to send a message to peer.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_SEND_MSG       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_SEND_MSG)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_CLOSE_CONNECTION
Language        = English
Failed to close connection.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_CLOSE_CONNECTION       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_CLOSE_CONNECTION)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_UNABLE_TO_CREATE_SERVER_THREAD
Language        = English
Could not create a thread to start the message dispatch server.
.
;
;#define  IZ_ADMIN_STATUS_UNABLE_TO_CREATE_SERVER_THREAD       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_UNABLE_TO_CREATE_SERVER_THREAD)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_RECEIVE_MSG
Language        = English
Error while waiting to receive a message.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_RECEIVE_MSG       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_RECEIVE_MSG)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_USERNAME_PROVIDED_WHEN_USAGE_NOT_DEFINED
Language        = English
Username must not be specified when Authentication Usage is not ISCSI_ZONE_AUTH_USAGE_DEFINED.
.
;
;#define  IZ_ADMIN_STATUS_USERNAME_PROVIDED_WHEN_USAGE_NOT_DEFINED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_USERNAME_PROVIDED_WHEN_USAGE_NOT_DEFINED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_PASSWORD_PROVIDED_WHEN_USAGE_NOT_DEFINED
Language        = English
Password must not be specified when Authentication Usage is not ISCSI_ZONE_AUTH_USAGE_DEFINED.
.
;
;#define  IZ_ADMIN_STATUS_PASSWORD_PROVIDED_WHEN_USAGE_NOT_DEFINED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_PASSWORD_PROVIDED_WHEN_USAGE_NOT_DEFINED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_PASSWORD_INFO_PROVIDED_WHEN_USAGE_NOT_DEFINED
Language        = English
PasswordInfo must not be specified when Authentication Usage is not ISCSI_ZONE_AUTH_USAGE_DEFINED.
.
;
;#define  IZ_ADMIN_STATUS_PASSWORD_INFO_PROVIDED_WHEN_USAGE_NOT_DEFINED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_PASSWORD_INFO_PROVIDED_WHEN_USAGE_NOT_DEFINED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NOT_SUPPORTED
Language        = English
This functionallity is not supported.
.
;
;#define  IZ_ADMIN_STATUS_NOT_SUPPORTED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NOT_SUPPORTED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_IPV4_ADDRESS_FORMAT
Language        = English
The target address is not specified in a valid IPV4 address format. Please check the IP address and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_IPV4_ADDRESS_FORMAT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_IPV4_ADDRESS_FORMAT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_IPV6_ADDRESS_FORMAT
Language        = English
The target address is not specified in a valid IPV6 address format. Please check the IP address and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_IPV6_ADDRESS_FORMAT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_IPV6_ADDRESS_FORMAT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_FQDN_ADDRESS_FORMAT
Language        = English
The target address is not specified in a valid FQDN address format. Please check the target address and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_FQDN_ADDRESS_FORMAT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_FQDN_ADDRESS_FORMAT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_WRITE_ZONE_DB_TO_PSM
Language        = English
Failed to write the connection set database persistently due to internal error. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_WRITE_ZONE_DB_TO_PSM       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_WRITE_ZONE_DB_TO_PSM)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ZONE_LOCK_ALREADY_ACQUIRED
Language        = English
Attempted to acquired the zone lock when already locked.
.
;
;#define  IZ_ADMIN_STATUS_ZONE_LOCK_ALREADY_ACQUIRED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ZONE_LOCK_ALREADY_ACQUIRED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ZONE_LOCK_NOT_ACQUIRED
Language        = English
Attempted to release the zone lock when the lock is not acquired.
.
;
;#define  IZ_ADMIN_STATUS_ZONE_LOCK_NOT_ACQUIRED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ZONE_LOCK_NOT_ACQUIRED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_ACQUIRE_ZONE_LOCK
Language        = English
Failed to acquire the zone lock. 
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_ACQUIRE_ZONE_LOCK       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_ACQUIRE_ZONE_LOCK)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_RELEASE_ZONE_LOCK
Language        = English
Failed to release the zone lock.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_RELEASE_ZONE_LOCK       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_RELEASE_ZONE_LOCK)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_ENCRYPT_PASSWORD
Language        = English
Failed to encrypt the secret. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_ENCRYPT_PASSWORD       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_ENCRYPT_PASSWORD)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_ENCRYPT_USERNAME
Language        = English
Failed to encrypt the username. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_ENCRYPT_USERNAME       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_ENCRYPT_USERNAME)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event viewer.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_DECRYPT_PASSWORD
Language        = English
Failed to decrypt the secret. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_DECRYPT_PASSWORD       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_DECRYPT_PASSWORD)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event Viewer.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_DECRYPT_USERNAME
Language        = English
Failed to decrypt the password. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_DECRYPT_USERNAME       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_DECRYPT_USERNAME)
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NO_FE_PORTS_SPECIFIED_IN_FOR_VERIFY
Language        = English
No front end port ID was specified to verify a device. Please specify a front end port ID and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_NO_FE_PORTS_SPECIFIED_IN_FOR_VERIFY     \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NO_FE_PORTS_SPECIFIED_IN_FOR_VERIFY)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_REFRESH_ZONE_DB_EVENT_FAILED
Language        = English
Failed to open refresh Zone DB event.
.
;
;#define  IZ_ADMIN_STATUS_REFRESH_ZONE_DB_EVENT_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_REFRESH_ZONE_DB_EVENT_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_REFRESH_ZONE_DB_THREAD_CREATION_FAILED
Language        = English
Failed to create refresh Zone DB thread.
.
;
;#define  IZ_ADMIN_STATUS_REFRESH_ZONE_DB_THREAD_CREATION_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_REFRESH_ZONE_DB_THREAD_CREATION_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: EventLog
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_SET_REFRESH_ZONE_DB_EVENT_FAILED
Language        = English
Internal information only. Failed to set refresh Zone DB event.
.
;
;#define  IZ_ADMIN_STATUS_SET_REFRESH_ZONE_DB_EVENT_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_SET_REFRESH_ZONE_DB_EVENT_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_MAX_DATABASE_SIZE_EXCEEDED
Language        = English
Maximum database size has been reached. Please remove unwanted connection sets from the database and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_MAX_DATABASE_SIZE_EXCEEDED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_MAX_DATABASE_SIZE_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: EventLog
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_PROCESS_TABLE_FULL
Language        = English
Internal information only. No empty process entry is available in the process table. The last entry will be returned.
.
;
;#define  IZ_ADMIN_STATUS_PROCESS_TABLE_FULL       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_PROCESS_TABLE_FULL)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER
Language        = English
An internal error has occurred while reading the connection set database. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_REQUEST_TO_ZMD_FAILED
Language        = English
A request to Zone Manager driver failed due to an internal error. (Ioctl: %1)
.
;
;#define  IZ_ADMIN_STATUS_REQUEST_TO_ZMD_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_REQUEST_TO_ZMD_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_TCP_LISTENING_PORT
Language        = English
Invalid TCP listening port provided. Please check the TCP port value (1-0xffff) and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_TCP_LISTENING_PORT      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_TCP_LISTENING_PORT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_BAD_CHAR_IN_USER_NAME
Language        = English
The user name contains one or more illegal characters.
.
;
;#define  IZ_ADMIN_STATUS_BAD_CHAR_IN_USER_NAME      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_BAD_CHAR_IN_USER_NAME)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_INVALID_VIRTUAL_PORT_INDEX
Language        = English
Invalid Virtual Port %2 specified.
.
;
;#define  IZ_ADMIN_STATUS_INVALID_VIRTUAL_PORT_INDEX       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_INVALID_VIRTUAL_PORT_INDEX)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_OPERATION_FAILED_DATABASE_VERSION_NOT_CURRENT
Language        = English
The operation failed because there is information in the database that needs to be internally updated. Please "commit" the FLARE-Operating-Environment package and retry the operation.
.
;
;#define  IZ_ADMIN_STATUS_OPERATION_FAILED_DATABASE_VERSION_NOT_CURRENT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_OPERATION_FAILED_DATABASE_VERSION_NOT_CURRENT)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_FAILED_TO_EXPAND_ISCSI_ZONE_DATA_AREA
Language        = English
Failed to convert the connection set database. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_FAILED_TO_EXPAND_ISCSI_ZONE_DATA_AREA       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_FAILED_TO_EXPAND_ISCSI_ZONE_DATA_AREA)

;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Internal Use Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Informational
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_GENERIC_INFO_TEXT
Language        = English
%1
.
;
;#define  IZ_ADMIN_STATUS_GENERIC_INFO_TEXT       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_GENERIC_INFO_TEXT)

;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_DATABASE_EXISTS_ZMD_NOT_LOADED
Language        = English
Could not open the connection set manager Driver. Please call your service provider.
.
;
;#define  IZ_ADMIN_STATUS_DATABASE_EXISTS_ZMD_NOT_LOADED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_DATABASE_EXISTS_ZMD_NOT_LOADED)

;//-----------------------------------------------------------------------------
;// Introduced In: Release 29
;//
;// Usage: Internal Use Only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = IZ_STATUS_NO_VIRTUAL_PORT_SPECIFIED_IN_FOR_VERIFY
Language        = English
Internal information only. No virtual port was specified to verify a path. Please specify a front end port ID and a virtual port ID and try the request again.
.
;
;#define  IZ_ADMIN_STATUS_NO_VIRTUAL_PORT_SPECIFIED_IN_FOR_VERIFY    \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(IZ_STATUS_NO_VIRTUAL_PORT_SPECIFIED_IN_FOR_VERIFY)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = 0x8500
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_NOT_ENOUGH_RESOURCES
Language        = English
Internal information only. Insufficient resources to load the ZMD driver.
.
;
;#define  ZMD_ADMIN_STATUS_NOT_ENOUGH_RESOURCES       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_NOT_ENOUGH_RESOURCES)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_DEVICE_HIERARCHY_FAILED
Language        = English
An attempt to create the device hierarchy failed.
.
;
;#define  ZMD_ADMIN_STATUS_DEVICE_HIERARCHY_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_DEVICE_HIERARCHY_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_CREATE_DEVICE_OBJECT_FAILED
Language        = English
Internal information only. An attempt to create the device object failed.
.
;
;#define  ZMD_ADMIN_CREATE_DEVICE_OBJECT_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_CREATE_DEVICE_OBJECT_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_INITIALIZE_FAILED
Language        = English
Internal information only. An attempt to initialize the ZMD driver failed.
.
;
;#define  ZMD_ADMIN_STATUS_INITIALIZE_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_INITIALIZE_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_CREATE_SYMLINK_FAILED
Language        = English
Internal information only. Could not create a symbolic link to the device node.
.
;
;#define  ZMD_ADMIN_STATUS_CREATE_SYMLINK_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_CREATE_SYMLINK_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_DRIVER_LOAD_FAILED
Language        = English
Internal information only. The ZMD driver failed to load.
.
;
;#define  ZMD_ADMIN_STATUS_DRIVER_LOAD_FAILED       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_DRIVER_LOAD_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event log entry
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_INSUFFICIENT_RESOURCES
Language        = English
There are insufficient resources to perform the request. The request can be tried later when other features or sessions have completed thereby making the resources available.
.
;
;#define  ZMD_ADMIN_STATUS_INSUFFICIENT_RESOURCES       \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_INSUFFICIENT_RESOURCES)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_ERROR_ZONE_DB_TIMEOUT
Language        = English
Internal information only. The Zone DB lock was held longer than expected.
.
;
;#define  ZMD_ADMIN_STATUS_ERROR_ZONE_DB_TIMEOUT      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_ERROR_ZONE_DB_TIMEOUT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_ZONE_DB_NOT_INITIALIZED
Language        = English
Internal information only. The Zone DB has been been initialized.
.
;
;#define  ZMD_ADMIN_STATUS_ZONE_DB_NOT_INITIALIZED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_ZONE_DB_NOT_INITIALIZED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_INVALID_IOCTL_REQUEST
Language        = English
Internal information only. Invalid IOCTL Request. (IOCTL: %2)
.
;
;#define  ZMD_ADMIN_STATUS_INVALID_IOCTL_REQUEST      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_INVALID_IOCTL_REQUEST)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_ERROR_INVALID_BUFFER_SIZE
Language        = English
Internal information only. A invalid buffer size was used for an IOCTL Request. (IOCTL: %2)
.
;
;#define  ZMD_ADMIN_STATUS_ERROR_INVALID_BUFFER_SIZE      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_ERROR_INVALID_BUFFER_SIZE)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER
Language        = English
Internal information only. Unable to return the User Virtual Address to the Zone DB
.
;
;#define  ZMD_ADMIN_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_ERROR_GETTING_ZONE_DB_UVA_POINTER)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_DELETE_SYMLINK_FAILED
Language        = English
Internal information only. Unable to delete the symbolic link to the ZMD device node.
.
;
;#define  ZMD_ADMIN_STATUS_DELETE_SYMLINK_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_DELETE_SYMLINK_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_USER_LOCK_ERROR
Language        = English
Internal information only. A "user" application went down while holding the ZoneDB lock. The lock will be released by ZMD.
.
;
;#define  ZMD_ADMIN_STATUS_USER_LOCK_ERROR      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_USER_LOCK_ERROR)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_DECRYPTION_ERROR
Language        = English
Internal information only. A decryption error has been detected.
.
;
;#define  ZMD_ADMIN_STATUS_DECRYPTION_ERROR      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_DECRYPTION_ERROR)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_ENCRYPTION_ERROR
Language        = English
Internal information only. An encryption error has been detected.
.
;
;#define  ZMD_ADMIN_STATUS_ENCRYPTION_ERROR      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_ENCRYPTION_ERROR)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_NULL_CACHE_POINTER
Language        = English
Internal information only. ZMDGetAuthenticationForPortal has been called with a NULL cache pointer.
.
;
;#define  ZMD_ADMIN_STATUS_NULL_CACHE_POINTER      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_NULL_CACHE_POINTER)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_CREATE_WORKER_THREAD_FAILED
Language        = English
Internal information only. Unable to create a Worker Thread.
.
;
;#define  ZMD_ADMIN_STATUS_CREATE_WORKER_THREAD_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_CREATE_WORKER_THREAD_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_MAX_REGISTERED_FE_PORTS_EXCEEDED
Language        = English
Internal information only. Error attempting to register too many FE Port with the DMD.
.
;
;#define  ZMD_ADMIN_STATUS_MAX_REGISTERED_FE_PORTS_EXCEEDED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_MAX_REGISTERED_FE_PORTS_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_STATUS_NO_ISCSI_FE_PORTS_FOUND
Language        = English
Internal information only. No ISCSI FE ports were found. 
.
;
;#define  ZMD_ADMIN_STATUS_NO_ISCSI_FE_PORTS_FOUND      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_STATUS_NO_ISCSI_FE_PORTS_FOUND)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 28
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZMD_IOCTL_CALLED_BY_INVALID_PROCESS
Language        = English
Internal information only. The IOCTL was called by an invalid 64-bit Process.
.
;
;#define  ZMD_ADMIN_IOCTL_CALLED_BY_INVALID_PROCESS      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZMD_IOCTL_CALLED_BY_INVALID_PROCESS)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = 0x8A00
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_INITIALIZATION_FAILED
Language        = English
Internal information only. Unable to complete the ZML initialization.
.
;
;#define  ZML_ADMIN_STATUS_INITIALIZATION_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_INITIALIZATION_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ZML_NOT_INITIALIZED
Language        = English
Internal information only. The calling driver has not initialized access to the ZML.
.
;
;#define  ZML_ADMIN_STATUS_ZML_NOT_INITIALIZED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ZML_NOT_INITIALIZED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ZONE_DB_NOT_INITIALIZED
Language        = English
Internal information only. ZMD has not initialized access to the Zone DB.
.
;
;#define  ZML_ADMIN_STATUS_ZONE_DB_NOT_INITIALIZED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ZONE_DB_NOT_INITIALIZED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_BUFFER_TOO_SMALL
Language        = English
Internal information only. The Get Next buffer is too small.
.
;
;#define  ZML_ADMIN_STATUS_BUFFER_TOO_SMALL      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_BUFFER_TOO_SMALL)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ERROR_READING_PSM
Language        = English
Internal information only. Unable to read the iSCSI Zones PSM data area.
.
;
;#define  ZML_ADMIN_STATUS_ERROR_READING_PSM      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ERROR_READING_PSM)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_MAX_REGISTERED_CALLBACKS_EXCEEDED
Language        = English
Internal information only. Too many processes attempted to register for Zone DB change notification.
.
;
;#define  ZML_ADMIN_STATUS_MAX_REGISTERED_CALLBACKS_EXCEEDED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_MAX_REGISTERED_CALLBACKS_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_DRIVER_NOT_REGISTERED_FOR_CALLBACK
Language        = English
Internal information only. A process not registered for Zone DB change notification attempted to deregoster.
.
;
;#define  ZML_ADMIN_STATUS_DRIVER_NOT_REGISTERED_FOR_CALLBACK      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_DRIVER_NOT_REGISTERED_FOR_CALLBACK)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_LOCK_REQUEST_FAILED
Language        = English
Internal information only. Unable to get the Zone DB lock.
.
;
;#define  ZML_ADMIN_STATUS_LOCK_REQUEST_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_LOCK_REQUEST_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_INVALID_ZONE_DB_UNLOCK_REQUEST
Language        = English
Internal information only. A request to unlock the Zone DB was made by a process that does not hold the lock (%2).
.
;
;#define  ZML_ADMIN_STATUS_INVALID_ZONE_DB_UNLOCK_REQUEST      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_INVALID_ZONE_DB_UNLOCK_REQUEST)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Event log entry.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_INSUFFICIENT_RESOURCES
Language        = English
There are insufficient resources to perform the request. The request can be tried later when other features or sessions have completed thereby making the resources available.
.
;
;#define  ZML_ADMIN_STATUS_INSUFFICIENT_RESOURCES      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_INSUFFICIENT_RESOURCES)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Panic.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_DATABASE_CORRUPT
Language        = English
Internal information only. Error encountered while updating the Zone DB from the PSM.
.
;
;#define  ZML_ADMIN_STATUS_DATABASE_CORRUPT      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_DATABASE_CORRUPT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_PSM_WRITE_FAILED
Language        = English
Internal information only. Unable to write the iSCSI Zones PSM data area (%2).
.
;
;#define  ZML_ADMIN_STATUS_PSM_WRITE_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_PSM_WRITE_FAILED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_SANITY_LIMIT_EXCEEDED
Language        = English
Testing information only. A counter used to prevent infinite loops exceeded its sanity limit.
.
;
;#define  ZML_ADMIN_STATUS_SANITY_LIMIT_EXCEEDED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_SANITY_LIMIT_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_UNEXPECTED_ZONE_DB_VERSION
Language        = English
Internal information only. The Zone DB has the wrong version number.
.
;
;#define  ZML_ADMIN_STATUS_UNEXPECTED_ZONE_DB_VERSION      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_UNEXPECTED_ZONE_DB_VERSION)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_UNSUPPORTED_IP_ADDRESS_FORMAT
Language        = English
Internal information only. The Zone DB contains an unsupported IP address format.
.
;
;#define  ZML_ADMIN_STATUS_UNSUPPORTED_IP_ADDRESS_FORMAT      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_UNSUPPORTED_IP_ADDRESS_FORMAT)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_DECRYPTION_ERROR
Language        = English
Internal information only. A decryption error has been detected (%2).
.
;
;#define  ZML_ADMIN_STATUS_DECRYPTION_ERROR      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_DECRYPTION_ERROR)
;
;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ENCRYPTION_ERROR
Language        = English
Internal information only. An encryption error has been detected.
.
;
;#define  ZML_ADMIN_STATUS_ENCRYPTION_ERROR      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ENCRYPTION_ERROR)
;
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;// Introduced In: Release 26
;//
;// Usage: Internal information only.
;//
;// Severity: Error
;//
;// Symptom of Problem
MessageId       = +1
Severity        = Error
Facility        = ISCSI_ZONES
SymbolicName    = ZML_STATUS_ZMD_IOCTL_FAILED
Language        = English
Internal information only. ZML failed in its attempt to send an IOCTL to ZMD.
.
;
;#define  ZML_ADMIN_STATUS_ZMD_IOCTL_FAILED      \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ZML_STATUS_ZMD_IOCTL_FAILED)
;
;//-----------------------------------------------------------------------------
;#endif

