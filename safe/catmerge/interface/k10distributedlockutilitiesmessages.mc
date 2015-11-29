;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_DISTRIBUTED_LOCK_UTILITIES_MESSAGES_H 
;#define K10_DISTRIBUTED_LOCK_UTILITIES_MESSAGES_H 
;
;//
;//++
;// File:            K10DistributedLockUtilitiesMessages.h (MC)
;//
;// Description:     This file defines DLU Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  DLU_xxx and DLU_ADMIN_xxx.
;//
;// History:         03-Mar-00       MWagner     Created
;//                  13-Feb-04       CVaidya     Added DLU_LOG_FAIL_FAST_DELAY.
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( Dlu= 0x112 : FACILITY_DLU )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x0001
Severity	= Informational
Facility	= Dlu
SymbolicName	= DLU_INFO_GENERIC
Language	= English
Generic DLU Information.
.

;
;#define DLU_ADMIN_INFO_GENERIC                                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_INFO_GENERIC)
;
                
;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dlu
SymbolicName	= DLU_INFO_LOAD_VERSION
Language	= English
Compiled on %2 at %3, %4.
.

;
;#define DLU_ADMIN_INFO_LOAD_VERSION                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_INFO_LOAD_VERSION)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dlu
SymbolicName	= DLU_INFO_LOADED
Language	= English
DriverEntry() returned at %2.
.

;
;#define DLU_ADMIN_INFO_LOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_INFO_LOADED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dlu
SymbolicName	= DLU_INFO_UNLOADED
Language	= English
Unloaded.
.

;
;#define DLU_ADMIN_INFO_UNLOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_INFO_UNLOADED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dlu
SymbolicName = DLU_LOG_FAIL_FAST_DELAY
Language	= English
User specified fail fast delay is %2.
.

;
;#define DLU_ADMIN_LOG_FAIL_FAST_DELAY                                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_LOG_FAIL_FAST_DELAY)
;

;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= Dlu
SymbolicName	= DLU_WARNING_GENERIC
Language	= English
Generic DLU Warning.
.

;
;#define DLU_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_WARNING_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning 
Facility	= Dlu
SymbolicName	= DLU_STATUS_WARNING_SEMAPHORE_LOCK_CONVERT_DENIED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_WARNING_SEMAPHORE_LOCK_CONVERT_DENIED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_STATUS_WARNING_SEMAPHORE_LOCK_CONVERT_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning 
Facility	= Dlu
SymbolicName	= DLU_STATUS_WARNING_LOCK_OPEN_DENIED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_WARNING_LOCK_OPEN_DENIED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_STATUS_WARNING_LOCK_OPEN_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning 
Facility	= Dlu
SymbolicName	= DLU_STATUS_WARNING_LOCK_CLOSE_DENIED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_WARNING_LOCK_CLOSE_DENIED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_STATUS_WARNING_LOCK_CLOSE_DENIED)

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_ERROR_GENERIC
Language	= English
Generic DLU Error Code.
.

;
;#define DLU_ADMIN_ERROR_GENERIC                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_ERROR_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_COULD_NOT_OPEN_LOCK      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_COULD_NOT_OPEN_LOCK       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_COULD_NOT_OPEN_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_COULD_NOT_CLOSE_LOCK     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_COULD_NOT_CLOSE_LOCK      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_COULD_NOT_CLOSE_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_COULD_NOT_CONVERT_LOCK   
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_COULD_NOT_CONVERT_LOCK    \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_COULD_NOT_CONVERT_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_CONVERT_UNEXPECTED_MODE  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_CONVERT_UNEXPECTED_MODE   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_CONVERT_UNEXPECTED_MODE)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_OPEN_SEMAPHORE_LIMIT_EXCEEDED       
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_OPEN_SEMAPHORE_LIMIT_EXCEEDED        \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_OPEN_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_WAITFOR_SEMAPHORE_LIMIT_EXCEEDED    
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_WAITFOR_SEMAPHORE_LIMIT_EXCEEDED     \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_WAITFOR_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_RELEASE_SEMAPHORE_LIMIT_EXCEEDED    
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_RELEASE_SEMAPHORE_LIMIT_EXCEEDED     \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_RELEASE_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SEMAPHORE_CALLBACK_UNEXPECTED_EVENT 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SEMAPHORE_CALLBACK_UNEXPECTED_EVENT  \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SEMAPHORE_CALLBACK_UNEXPECTED_EVENT)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CLOSING_NULL_LOCK        
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CLOSING_NULL_LOCK         \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CLOSING_NULL_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CONVERTING_NULL_LOCK     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CONVERTING_NULL_LOCK      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CONVERTING_NULL_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_OPEN_SYNC_LOCK_LIMIT_EXCEEDED      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_OPEN_SYNC_LOCK_LIMIT_EXCEEDED       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_OPEN_SYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_CONVERT_SYNC_LOCK_LIMIT_EXCEEDED   
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_CONVERT_SYNC_LOCK_LIMIT_EXCEEDED    \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_CONVERT_SYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_CLOSE_SYNC_LOCK_LIMIT_EXCEEDED     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_CLOSE_SYNC_LOCK_LIMIT_EXCEEDED      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_CLOSE_SYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_INIT_SYNC_LOCK_LIMIT_EXCEEDED      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_INIT_SYNC_LOCK_LIMIT_EXCEEDED       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_INIT_SYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_OPEN_LOCK      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_COULD_NOT_OPEN_LOCK       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_OPEN_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_CLOSE_LOCK     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_COULD_NOT_CLOSE_LOCK      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_CLOSE_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_CONVERT_LOCK    
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_COULD_NOT_CONVERT_LOCK     \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_COULD_NOT_CONVERT_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_SYNC_LOCK_CALLBACK_UNEXPECTED_EVENT  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_SYNC_LOCK_CALLBACK_UNEXPECTED_EVENT   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_SYNC_LOCK_CALLBACK_UNEXPECTED_EVENT)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CLOSING_NULL_LOCK        
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CLOSING_NULL_LOCK         \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CLOSING_NULL_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CONVERTING_NULL_LOCK     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CONVERTING_NULL_LOCK      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CONVERTING_NULL_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_OPEN_ASYNC_LOCK_LIMIT_EXCEEDED      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_OPEN_ASYNC_LOCK_LIMIT_EXCEEDED       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_OPEN_ASYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_CONVERT_ASYNC_LOCK_LIMIT_EXCEEDED   
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_CONVERT_ASYNC_LOCK_LIMIT_EXCEEDED    \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_CONVERT_ASYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_CLOSE_ASYNC_LOCK_LIMIT_EXCEEDED     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_CLOSE_ASYNC_LOCK_LIMIT_EXCEEDED      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_CLOSE_ASYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_INIT_ASYNC_LOCK_LIMIT_EXCEEDED      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_INIT_ASYNC_LOCK_LIMIT_EXCEEDED       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_INIT_ASYNC_LOCK_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_OPEN_LOCK      
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_COULD_NOT_OPEN_LOCK       \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_OPEN_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_OPEN_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_CLOSE_LOCK     
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_COULD_NOT_CLOSE_LOCK      \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_CLOSE_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_CLOSE_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_CONVERT_LOCK    
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_COULD_NOT_CONVERT_LOCK     \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_COULD_NOT_CONVERT_LOCK)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_CONVERT_SEMAPHORE_LIMIT_EXCEEDED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_UNEXPECTED_EVENT  
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_ASYNC_LOCK_CALLBACK_UNEXPECTED_EVENT   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_ASYNC_LOCK_CALLBACK_UNEXPECTED_EVENT)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_INIT_SEMAPHORE_LIMIT_EXCEEDED 
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_INIT_SEMAPHORE_LIMIT_EXCEEDEDT   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_INIT_SEMAPHORE_LIMIT_EXCEEDED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE
Language	= English
Insert message here.
.

;
;#define DLU_ADMIN_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE   \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE)
;

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= Dlu
SymbolicName	= DLU_CRTICIAL_ERROR_GENERIC
Language	= English
Generic DLU Critical Error Code.
.

;
;#define DLU_ADMIN_CRTICIAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLU_CRTICIAL_ERROR_GENERIC)
;

;#endif
