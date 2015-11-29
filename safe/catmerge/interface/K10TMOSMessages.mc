;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_TMOS_MESSAGES_H 
;#define K10_TMOS_MESSAGES_H 
;
;//
;//++
;// File:            K10TMOSMessages.h (MC)
;//
;// Description:     This file defines TMOS Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  TMOS_xxx and ULS_ADMIN_xxx.
;//
;// History:         06-Jun-03       MWagner     Created
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( TMOS= 0x146 : FACILITY_TMOS )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Unused
;//
;// Severity:           Info
;//
;// Symptom of Problem: This is a place holder for TMOS Info messages
;//
;//
MessageId	= 0x0001
Severity	= Informational
Facility	= TMOS
SymbolicName	= TMOS_INFO_GENERIC
Language	= English
Generic TMOS Information.
.

;
;#define TMOS_ADMIN_STATUS_INFO_GENERIC  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_INFO_GENERIC)
;
                
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Unused
;//
;// Severity:           Info
;//
;// Symptom of Problem: This is a place holder for TMOS Info messages
;//
;//
MessageId	= +1
Severity	= Informational
Facility	= TMOS
SymbolicName	= TMOS_INFO_SUPERVISOR_PROCESS_REQUEUE
Language	= English
Generic TMOS Information.
.

;
;#define TMOS_ADMIN_STATUS_INFO_SUPERVISOR_PROCESS_REQUEUE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_INFO_SUPERVISOR_PROCESS_REQUEUE)
;

;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Unused
;//
;// Severity:           Warning
;//
;// Symptom of Problem: This is a place holder for Warning Info messages
;//
;//
MessageId	= 0x4000
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_GENERIC
Language	= English
Generic TMOS Warning.
.

;
;#define TMOS_ADMIN_WARNING_GENERIC  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_GENERIC)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOSInitializeHandle() could not allocate a TMOS_Body
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_BODY
Language	= English
TMOSInitializeHandle() could not allocate a TMOS_Body
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_BODY \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_BODY)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOSRebuildDocket() could not allocate a TMOS_Docket
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_DOCKET
Language	= English
TMOSRebuildDocket() could not allocate a TMOS_Docket
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_DOCKET \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_DOCKET)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS was called with an un-initialized handle
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_UNINITIALIZED_HANDLE
Language	= English
TMOS was called with an un-initialized handle
.

;
;#define TMOS_ADMIN_WARNING_UNINITIALIZED_HANDLE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_UNINITIALIZED_HANDLE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem:An attempt was made to process a Multiple Array TMOS 
;//                    Operation. This may also be caused by a corrupt SPID.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_MULTIPLE_ARRAYS_NOT_SUPPORTED
Language	= English
An attempt was made to process a Multiple Array TMOS Operation. This may also
be caused by a corrupt SPID.
.

;
;#define TMOS_ADMIN_WARNING_MULTIPLE_ARRAYS_NOT_SUPPORTED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_MULTIPLE_ARRAYS_NOT_SUPPORTED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: Driver Name specified in the TMOS_InitializeHandle() is too long.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_CLIENT_NAME_TOO_LONG
Language	= English
The Driver Name specified in the TMOS_InitializeHandle() is too long.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_CLIENT_NAME_TOO_LONG  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_CLIENT_NAME_TOO_LONG)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::OpenDocket() called with invalid open mode.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_OPEN_MODE
Language	= English
TMOS_Docket::OpenDocket() called with invalid open mode.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_OPEN_MODE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_OPEN_MODE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::CallPsm() called with invalid Ioctl.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_PSM_IOCTL
Language	= English
TMOS_Docket::CallPsm() called with invalid Ioctl
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_PSM_IOCTL  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_PSM_IOCTL)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::CallPsm() could not allocate an IRP
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_PSM_IRP
Language	= English
TMOS_Docket::CallPsm() could not allocate an IRP
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_COULD_NOT_ALLOCATE_PSM_IRP  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_PSM_IRP)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Docket could not allocate a buffer to 
;//                     read the Journal Header
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_JOURNAL_HEADER
Language	= English
The TMOS_Docket could not allocate a buffer to read the Journal Header
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_COULD_NOT_ALLOCATE_JOURNAL_HEADER  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_JOURNAL_HEADER)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::CloseDocket() called with invalid close mode.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_CLOSE_MODE
Language	= English
TMOS_Docket::CloseDocket() called with invalid open mode.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_CLOSE_MODE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_CLOSE_MODE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::ReadDocket() could not allocate a PSM_READ_OUT_BUFFER
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_READ_BUFFER
Language	= English
TMOS_Docket::ReadDocket() could not allocate a PSM_READ_OUT_BUFFER
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_COULD_NOT_ALLOCATE_READ_BUFFER  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_READ_BUFFER)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::WriteDocket() could not allocate a PSM_WRITE_IN_BUFFER
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_WRITE_BUFFER
Language	= English
TMOS_Docket::WriteDocket() could not allocate a PSM_WRITE_IN_BUFFER
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_COULD_NOT_ALLOCATE_WRITE_BUFFER  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_WRITE_BUFFER)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Version does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_SP_VERSION
Language	= English
TMOS_Docket::AssayDocket() Docket SP Version for my SP does not match SP's Current Version
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_SP_VERSION  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_SP_VERSION)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Version does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_DOCKET_VERSION
Language	= English
TMOS_Docket::AssayDocket() Docket Version for does not match SP's Current Version
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_DOCKET_VERSION  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_CURRENT_VERSION_DOES_NOT_MATCH_DOCKET_VERSION)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Entry Size does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_DOCKET_ENTRY_SIZE
Language	= English
TMOS_Docket::AssayDocket() Docket Entry Size does not match
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_DOCKET_ENTRY_SIZE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_DOCKET_ENTRY_SIZE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Number of Entries does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_DOCKET_NUMBER_OF_ENTRIES
Language	= English
TMOS_Docket::AssayDocket() Docket Number of Entries does not match
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_DOCKET_NUMBER_OF_ENTRIES  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_DOCKET_NUMBER_OF_ENTRIES)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Name does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_DOCKET_NAME
Language	= English
TMOS_Docket::AssayDocket() Docket Name does not match
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_DOCKET_NAME  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_DOCKET_NAME)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Name does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_INVALID_DOCKET_SIZE
Language	= English
TMOS_Docket::FormatDocket() PSM Docket wrong size
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_INVALID_DOCKET_SIZE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_INVALID_DOCKET_SIZE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS_Docket::AssayDocket() Docket Name does not match
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_FORMAT_BUFFER
Language	= English
TMOS_Docket::FormatDocket() could not allocate a buffer
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_COULD_NOT_ALLOCATE_FORMAT_BUFFER  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_COULD_NOT_ALLOCATE_FORMAT_BUFFER)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A Docket operation is being attempted on an uncertified Docket
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_NOT_CERTIFIED
Language	= English
A Docket operation is being attempted on an uncertified Docket
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_NOT_CERTIFIED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_NOT_CERTIFIED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A Body operation is being attempted on an uncertified Body
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_BODY_NOT_CERTIFIED
Language	= English
A Body operation is being attempted on an uncertified Body
.

;
;#define TMOS_ADMIN_WARNING_BODY_NOT_CERTIFIED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_BODY_NOT_CERTIFIED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not allocate a buffer to read/write a Docket Entry 
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_DOCKET_ENTRY
Language	= English
TMOS could not allocate a buffer to read/write a Docket Entry 
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_DOCKET_ENTRY  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_DOCKET_ENTRY)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: An attempt was made to read/write a Docket Entry larger
;//                     than the Docket's Entry Size.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_ENTRY_TOO_LARGE
Language	= English
An attempt was made to read/write a Docket Entry larger than the Docket's Entry Size.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_ENTRY_TOO_LARGE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_ENTRY_TOO_LARGE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: An attempt was made to store a Docket Entry, and there
;//                     was not an empty slot
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_FULL
Language	= English
An attempt was made to store a Docket Entry and there was not an empty slot
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_FULL  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_FULL)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: GetNextCookie() cannot find a next Cookie
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_NO_MORE_COOKIES
Language	= English
GetNextCookie() cannot find a next Cookie
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_NO_MORE_COOKIES  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_NO_MORE_COOKIES)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Get Entry did specified fewer Complicit SPs 
;//                     than were stored in the Docket
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_GET_ENTRY_NUMBER_OF_COMPLICIT_SPS_MISMATCH
Language	= English
A call to Get Entry did specified fewer Complicit SPs than were stored in the Docket
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_GET_ENTRY_NUMBER_OF_COMPLICIT_SPS_MISMATCH  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_GET_ENTRY_NUMBER_OF_COMPLICIT_SPS_MISMATCH)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Get Entry did specified a smaller Client Data size
;//                     than was stored in the Docket
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_GET_ENTRY_CLIENT_DATA_SIZE_MISMATCH
Language	= English
A call to Get Entry did specified a smaller Client Data Size than was stored in the Docket
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_GET_ENTRY_CLIENT_DATA_SIZE_MISMATCH  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_GET_ENTRY_CLIENT_DATA_SIZE_MISMATCH)
;


;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Find Entry specified a Journal Slot that is not
;//                     in use.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_FIND_ENTRY_SLOT_NOT_IN_USE
Language	= English
A call to Read Entry did specified a Journal Slot that is not in use.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_FIND_ENTRY_SLOT_NOT_IN_USE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_FIND_ENTRY_SLOT_NOT_IN_USE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Get Entry found an Entry with an unknown version
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_READ_ENTRY_UNKNOWN_VERSION
Language	= English
A call to Read Entry found an Entry with an unknown version.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_READ_ENTRY_UNKNOWN_VERSION  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_READ_ENTRY_UNKNOWN_VERSION)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Get Entry found an Entry with a Cookie that did
;//                     not match the specigied Cookie.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_READ_ENTRY_COOKIE_DOES_NOT_MATCH
Language	= English
A call to Get Entry found an Entry with a Cookie that did not match the specigied Cookie.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_READ_ENTRY_COOKIE_DOES_NOT_MATCH  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_READ_ENTRY_COOKIE_DOES_NOT_MATCH)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Store Client Data exceeds the available space
;//                     in the Entry
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_STORE_CLIENT_DATA_NOT_ENOUGH_SPACE
Language	= English
A call to Store Client Data exceeds the available space in the Entry.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_STORE_CLIENT_DATA_NOT_ENOUGH_SPACE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_STORE_CLIENT_DATA_NOT_ENOUGH_SPACE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A call to Docket Build with the Force Create flag set
;//                     was made on a Docket that is Certified
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DOCKET_BUILD_FORCE_CREATE_IGNORED
Language	= English
A call to Docket Build with the Force Create flag set was made on a Docket that 
is Certified.
.

;
;#define TMOS_ADMIN_WARNING_DOCKET_BUILD_FORCE_CREATE_IGNORED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DOCKET_BUILD_FORCE_CREATE_IGNORED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A TMOS Supervisor has been given Ticket with an Unknown Task.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_INVALID_TASK
Language	= English
A TMOS Supervisor has been given Ticket with an Unknown Task.
.

;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_INVALID_TASK  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_INVALID_TASK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A TMOS Supervisor has been given Ticket with an Unknown Task.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DELEGATE_INVALID_TASK
Language	= English
A TMOS Supervisor has been given Ticket with an Unknown Task.
.

;
;#define TMOS_ADMIN_WARNING_DELEGATE_INVALID_TASK  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DELEGATE_INVALID_TASK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem:  A TMOS Ward has been asked to Enqueue a Ticket when 
;//                      the Warden has been stopped
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_WARD_ENQUEUE_WITH_CRY_HAVOC_SET
Language	= English
A TMOS Ward has been asked to Enqueue a Ticket when the Warden has been stopped
.

;
;#define TMOS_ADMIN_WARNING_WARD_ENQUEUE_WITH_CRY_HAVOC_SET  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_WARD_ENQUEUE_WITH_CRY_HAVOC_SET)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A Supervisor operation is being attempted on an uncertified Supervisor
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_NOT_CERTIFIED
Language	= English
A Supervisor operation is being attempted on an uncertified Supervisor
.

;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_NOT_CERTIFIED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_NOT_CERTIFIED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A Delegate operation is being attempted on an uncertified Delegate
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_DELEGATE_NOT_CERTIFIED
Language	= English
A Delegate operation is being attempted on an uncertified Delegate
.

;
;#define TMOS_ADMIN_WARNING_DELEGATE_NOT_CERTIFIED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_DELEGATE_NOT_CERTIFIED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not allocate an MPS Message
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_MPS_MESSAGE
Language	= English
TMOS could not allocate an MPS Message.
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_MPS_MESSAGE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_MPS_MESSAGE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not allocate a Ticket
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_TICKET
Language	= English
TMOS could not allocate a Ticket
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_TICKET  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_TICKET)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not allocate a Operation Record
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_OPERATION_RECORD
Language	= English
TMOS could not allocate a Operation Record
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_OPERATION_RECORD  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_OPERATION_RECORD)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not allocate a Status Packet
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_COULD_NOT_ALLOCATE_STATUS_PACKET
Language	= English
TMOS could not allocate a Status Packet
.

;
;#define TMOS_ADMIN_WARNING_COULD_NOT_ALLOCATE_STATUS_PACKET  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_COULD_NOT_ALLOCATE_STATUS_PACKET)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS got a message that it did not understand
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_INVALID_ILK
Language	= English
TMOS got a message that it did not undertand
.
;
;#define TMOS_ADMIN_WARNING_INVALID_ILK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_INVALID_ILK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOSStart() was called with an Initial Status bigger than
;//                      the stored Client Status Size
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_START_INITIAL_STATUS_TOO_LARGE
Language	= English
TMOSStart() was called with an Initial Status bigger than the stored Client Status Size
.
;
;#define TMOS_ADMIN_WARNING_START_INITIAL_STATUS_TOO_LARGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_START_INITIAL_STATUS_TOO_LARGE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOSStart() could not allocate an array of SPIDS to 
;//                     read the Complicit SPs from the Docket.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_START_COULD_NOT_ALLOCATE_SPS
Language	= English
TMOSStart() could not allocate an array of SPIDS to read the Complicit SPs from
the Docket.
.
;
;#define TMOS_ADMIN_WARNING_START_COULD_NOT_ALLOCATE_SPS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_START_COULD_NOT_ALLOCATE_SPS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor has been asked to Send a Message, but
;//                     it can't tell which message to Send.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_SEND_REQUEST_INVALID_TASK
Language	= English
The TMOS Supervisor has been asked to Send a Message, but
it can't tell which Message to Send.
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_SEND_REQUEST_INVALID_TASK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_SEND_REQUEST_INVALID_TASK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor has been asked to Send a Message, but
;//                     it can't tell which message to Send.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_SEND_RESPONSE_INVALID_TASK
Language	= English
The TMOS Supervisor has been asked to Send a Message, but
it can't tell which Message to Send.
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_SEND_RESPONSE_INVALID_TASK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_SEND_RESPONSE_INVALID_TASK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor cannot allocate Operation Reports 
;//                     to send back to the Client.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_COULD_NOT_ALLOCATE_CLIENT_REPORTS
Language	= English
The TMOS Supervisor cannot allocate Operation Reports to send back to the Client
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_CALCULATE_STATUS_COULD_NOT_ALLOCATE_OPERATION_REPORTS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_COULD_NOT_ALLOCATE_CLIENT_REPORTS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor cannot allocate Operation Reports 
;//                     to send back to the Client.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_RETURN_STATUS_COULD_NOT_ALLOCATE_OPERATION_REPORTS
Language	= English
The TMOS Supervisor cannot allocate Operation Reports to send back to the Client
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_RETURN_STATUS_COULD_NOT_ALLOCATE_OPERATION_REPORTS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_RETURN_STATUS_COULD_NOT_ALLOCATE_OPERATION_REPORTS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor is trying to start a Transition while
;//                     a Transition is in progress.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_TRANSITION_IN_PROGRESS
Language	= English
The TMOS Supervisor is trying to start a Transition while a Transition is in progress.
.
;
;#define TMOS_ADMIN_WARNING_TRANSITION_IN_PROGRESS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_TRANSITION_IN_PROGRESS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor is trying to complete a Transition while
;//                     no Transition is in progress.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_NO_TRANSITION_IN_PROGRESS
Language	= English
The TMOS Supervisor is trying to complete a Transition while no Transition is in progress.
.
;
;#define TMOS_ADMIN_WARNING_NO_TRANSITION_IN_PROGRESS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_NO_TRANSITION_IN_PROGRESS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor CompleteTransition() called when 
;//                     the Supervisor is still waiting to hear from an SP
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_TRANSITION_INCOMPLETE
Language	= English
The TMOS Supervisor CompleteTransition() called when the Supervisor is still 
waiting to hear from an SP
.
;
;#define TMOS_ADMIN_WARNING_TRANSITION_INCOMPLETE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_TRANSITION_INCOMPLETE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: An invalid Transition has been specified
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_TRANSITION_INVALID
Language	= English
An invalid Transition has been specified
.
;
;#define TMOS_ADMIN_WARNING_TRANSITION_INVALID  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_TRANSITION_INVALID)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: An invalid State has been specified
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_STATE_INVALID
Language	= English
An invalid State has been specified
.
;
;#define TMOS_ADMIN_WARNING_STATE_INVALID  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_STATE_INVALID)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor ClientReturnStatus() retuned a transient error
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_RETURN_STATUS_RETRY
Language	= English
The TMOS Supervisor ClientReturnStatus() returned a transient error;
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_RETURN_STATUS_RETRY  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_RETURN_STATUS_RETRY)
;


;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor ClientCalculateStatus() returned a transient error
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_CALCULATE_STATUS_RETRY
Language	= English
The TMOS Supervisor ClientCalculateStatus() returned a transient error
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_CALCULATE_STATUS_RETRY  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_CALCULATE_STATUS_RETRY)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not find an Operation associated with the specified Cookie
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_NO_OPERATION_RECORD_FOUND
Language	= English
TMOS could not find an Operation associated with the specified Cookie
.
;
;#define TMOS_ADMIN_WARNING_NO_OPERATION_RECORD_FOUND  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_NO_OPERATION_RECORD_FOUND)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS could not find an Status Packet associated with 
;//                     the SP for the Operation
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_NO_STATUS_PACKET_FOUND
Language	= English
TMOS could not find an Status Packet associated with the SP for the Operation
.
;
;#define TMOS_ADMIN_WARNING_NO_STATUS_PACKET_FOUND  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_NO_STATUS_PACKET_FOUND)
;
;//------------------------------------------------------------------------------

;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS is trying to assign some Status for a stale (old) Tranistion
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_STALE_TRANSITION_SEQUENCE
Language	= English
TMOS is trying to assign some Status for a stale (old) Tranistion
.
;
;#define TMOS_ADMIN_WARNING_STALE_TRANSITION_SEQUENCE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_STALE_TRANSITION_SEQUENCE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS is trying to assign some SP Status to an Operation 
;//                     that already has Status for that SP
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_STALE_STATUS_SEQUENCE
Language	= English
TMOS is trying to assign some SP Status to an Operation that already has Status for that SP
.
;
;#define TMOS_ADMIN_WARNING_STALE_STATUS_SEQUENCE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_STALE_STATUS_SEQUENCE)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS is trying to assign some SP Status to an Operation 
;//                     for an SP that isn't involved in the Operation.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_RECEIVE_MESSAGE_NO_SP_FOUND
Language	= English
TMOS is trying to assign some SP Status to an Operation for an SP that isn't 
involved in the Operation.
.
;
;#define TMOS_ADMIN_WARNING_RECEIVE_MESSAGE_NO_SP_FOUND  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_RECEIVE_MESSAGE_NO_SP_FOUND)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor received a Message, but can't 
;//                     figure out how to process it.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SUPERVISOR_RECEIVE_REQUEST_INVALID_TASK
Language	= English
The TMOS Supervisor received a Message, but can't figure out how to process it.
.
;
;#define TMOS_ADMIN_WARNING_SUPERVISOR_RECEIVE_REQUEST_INVALID_TASK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SUPERVISOR_RECEIVE_REQUEST_INVALID_TASK)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A Complict SP has died.
;//                     doesn't understand the required transition.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_SP_CONTACT_LOST
Language	= English
A Complicit SP has died.
.
;
;#define TMOS_ADMIN_WARNING_SP_CONTACT_LOST \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_SP_CONTACT_LOST)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Morgue has searched all of its Gurneys
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_MORGUE_NO_MORE_GURNEYS
Language	= English
The TMOS Morgue has searched all of its Gurneys
.
;
;#define TMOS_ADMIN_WARNING_MORGUE_NO_MORE_GURNEYS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_MORGUE_NO_MORE_GURNEYS)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: An SP died while we are processing its previous death
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_MORGUE_PREVIOUSLY_EMBALMED
Language	= English
An SP died while we are processing its previous death
.
;
;#define TMOS_ADMIN_WARNING_MORGUE_PREVIOUSLY_EMBALMED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_MORGUE_PREVIOUSLY_EMBALMED)
;


;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS Morge Reanimate() failed because the SP had died twice.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_MORGUE_REANIMATION_DENIED
Language	= English
TMOS Morge Reanimate() failed because the SP had died twice.
.
;
;#define TMOS_ADMIN_WARNING_MORGUE_REANIMATION_DENIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_MORGUE_REANIMATION_DENIED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOSInitializeHandle() was called for an already initialized Handle
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_HANDLE_ALREADY_INITIALIZED
Language	= English
TMOSInitializeHandle() was called for an already initialized Handle
.
;
;#define TMOS_ADMIN_WARNING_HANDLE_ALREADY_INITIALIZED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_HANDLE_ALREADY_INITIALIZED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor is trying to Admit an an operation to
;//                     Emergency Room, but there are no empty beds.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_EMERGENCY_ROOM_FULL
Language	= English
The TMOS Supervisor is trying to Admit an an operation to Emergency Room, but there are no empty beds.
.
;
;#define TMOS_ADMIN_WARNING_EMERGENCY_ROOM_FULL \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_EMERGENCY_ROOM_FULL)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor is trying to Discharge or Find an operation 
;//                      from/in the Emergency Room, but there are no occupied Beds
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_EMERGENCY_ROOM_EMPTY
Language	= English
The TMOS Supervisor is trying to Discharge or Find an operationfrom/in the Emergency Room, but there are no occupied Beds
.
;
;#define TMOS_ADMIN_WARNING_EMERGENCY_ROOM_EMPTY \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_EMERGENCY_ROOM_EMPTY)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: The TMOS Supervisor is trying to Discharge or Find an operation 
;//                      from/in the Emergency Room, but there is no such patient.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_EMERGENCY_ROOM_PATIENT_NOT_FOUND
Language	= English
The TMOS Supervisor is trying to Discharge or Find an operationfrom/in the Emergency Room, but there are no occupied Beds
.
;
;#define TMOS_ADMIN_WARNING_EMERGENCY_ROOM_PATIENT_NOT_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_EMERGENCY_ROOM_PATIENT_NOT_FOUND)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: Client's Client Status Size does not match TMOS's Client Status Size
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_CLIENT_STATUS_SIZE_MISMATCH
Language	= English
Client's Client Status Size does not match TMOS's Client Status Size
.

;
;#define TMOS_ADMIN_WARNING_CLIENT_STATUS_SIZE_MISMATCH  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_CLIENT_STATUS_SIZE_MISMATCH)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS Received a message with a version that it did not
;//                     understand 
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_MESSAGE_UNKNOWN_VERSION
Language	= English
TMOS Received a message with a version that it did not understand.
.

;
;#define TMOS_ADMIN_WARNING_MESSAGE_UNKNOWN_VERSION  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_MESSAGE_UNKNOWN_VERSION)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 16
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: TMOS Received a lost contact for a remote array.
;//
;//
MessageId	= +1
Severity	= Warning
Facility	= TMOS
SymbolicName	= TMOS_WARNING_REMOTE_LOST_CONTACT_RECEIVED
Language	= English
TMOS received a lost contact message for a remote array not participating in an operation.
.

;
;#define TMOS_ADMIN_WARNING_REMOTE_LOST_CONTACT_RECEIVED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_WARNING_REMOTE_LOST_CONTACT_RECEIVED)
;


;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Unused
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is a place holder for Error Info messages
;//
;//
MessageId	= 0x8000
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_ERROR_GENERIC
Language	= English
Generic TMOS Error Code.
.

;
;#define TMOS_ADMIN_ERROR_GENERIC  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_ERROR_GENERIC)
;
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: The TMOS Supervisor had been handed a Ticket with a Task
;//                     it did not understand.
;//
;//
MessageId	= +1
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_ERROR_SUPERVISOR_UNKNOWN_TASK
Language	= English
The TMOS Supervisor had been handed a Ticket with a Task it did not understand.
[1] PTicket
[2] PBody
[3] Line Number
[4] Unknown Task
.

;
;#define TMOS_ADMIN_ERROR_SUPERVISOR_UNKNOWN_TASK  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_ERROR_SUPERVISOR_UNKNOWN_TASK)
;
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal  BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: The TMOS Supervisor had been handed a Ticket with a Task
;//                     it did not understand.
;//
;//
MessageId	= +1
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_ERROR_DELEGATE_UNKNOWN_TASK
Language	= English
The TMOS Delegate had been handed a Ticket with a Task it did not understand.
[1] PTicket
[2] PBody
[3] Line Number
[4] Unknown Task
.

;
;#define TMOS_ADMIN_ERROR_DELEGATE_UNKNOWN_TASK  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_ERROR_DELEGATE_UNKNOWN_TASK)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: TMOS_LookasideList::Free() was called with a ptr with
;//                     an invalid allocation type.
;//
;//
MessageId	= +1
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_BUGCHECK_LOOKASIDE_LIST_INVALID_ALLOC_TYPE
Language	= English
TMOS_LookasideList::Free() was called with a ptr with an invalid allocation type.
[1] Ptr
[2] AllocationHeader
[3] __LINE__
[4] Type
.

;
;#define TMOS_ADMIN_BUGCHECK_LOOKASIDE_LIST_INVALID_ALLOC_TYPE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_BUGCHECK_LOOKASIDE_LIST_INVALID_ALLOC_TYPE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: TMOS_Body::FreeTicket() was called with a ptr with
;//                     an invalid allocation type.
;//
;//
MessageId	= +1
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_BUGCHECK_BODY_FREE_TICKET_INVALID_ALLOC_TYPE
Language	= English
TMOS_Body::FreeTicket() was called with a ptr with an invalid allocation type.
[1] Ptr
[2] AllocationHeader
[3] __LINE__
[4] Type
.

;
;#define TMOS_ADMIN_BUGCHECK_BODY_FREE_TICKET_INVALID_ALLOC_TYPE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_BUGCHECK_BODY_FREE_TICKET_INVALID_ALLOC_TYPE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: Used to intialize the Client's status value in
;//                     the TMOS SP operation record when the Ringleader
;//                     goes down.
;//
;//
MessageId	= +1
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_ERROR_CLIENT_MOURN_RINGLEADER
Language	= English
Mourning the lost contact of the Ringleader.
.

;
;#define TMOS_ADMIN_ERROR_CLIENT_MOURN_RINGLEADER  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_ERROR_CLIENT_MOURN_RINGLEADER)
;
;//-----------------------------------------------------------------------------


;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
;//------------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage:              Unused
;//
;// Severity:           Critical Error
;//
;// Symptom of Problem: This is a place holder for Critical Error Info messages
;//
;//
MessageId	= 0xC000
Severity	= Error
Facility	= TMOS
SymbolicName	= TMOS_CRTICIAL_ERROR_GENERIC
Language	= English
Generic DLS Critical Error Code.
.

;
;#define TMOS_ADMIN_CRTICIAL_ERROR_GENERIC   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(TMOS_CRTICIAL_ERROR_GENERIC)
;


;#endif
