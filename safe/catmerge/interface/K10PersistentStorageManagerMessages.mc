;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_PERSISTENT_STORAGE_MANAGER_MESSAGES_H
;#define K10_PERSISTENT_STORAGE_MANAGER_MESSAGES_H
;
;//
;//++
;// File:            K10PersistentStorageManagerMessages.h (MC)
;//
;// Description:     This file defines PSM Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  PSM_xxx and PSM_ADMIN_xxx.
;//
;// History:         03-Mar-00       MWagner     Created
;//--
;//                  14-Mar-01       CVaidya     Added Flare Timeout Warning
;//                                              status code.                                              
;//                  18-Apr-01       CVaidya     Added Flare Timeout Error
;//                                              status code.
;//                  18-Jun-02       Majmera     Changed PSM_ERROR_UNEXPECTED_IO_FAILURE 
;//                                              to PSM_INFO_SOFT_IO_FAILURE
;//                  02-Jul-02       majmera     Backing out of the prev changes, 
;//                                              Changing the info mesg to a error.
;//                  18-Nov-02       majmera     Adding warning on automatic closing of
;//                                              a data area.
;//                  24-Jul-03       majmera     Adding info status to log IO errors. 89593.
;//                  24-Jul-03       majmera     Adding Internal Information Only. 89593.
;//                  24-Jul-03       majmera     Adding message for deletion of data area. 91370.
;//                  27-Oct-03       majmera     Added bugcheck for invalid dad entry. 94337
;//                  17-Nov-03       majmera     Corrected typo for data area. 93085.
;//                  17-Nov-03       majmera     Added panic code for situation when an empty,
;//                                              data area name is passed for delete/open. 95694.
;//                  31-Mar-04       majmera     Added PSM_ERROR_INVALID_DATA_AREA_OPEN_PARAMETER. 
;//                  30-Apr-04       majmera     Added PSM_ERROR_UNKNOWN_VERSION_DETECTED. 106170
;#include "k10defs.h" 
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( Psm= 0x115 : FACILITY_PSM )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x0001
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_GENERIC
Language        = English
Generic PSM Information.
.

;
;#define PSM_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_LOAD_VERSION
Language        = English
Compiled on %2 at %3, %4.
.

;
;#define PSM_ADMIN_INFO_LOAD_VERSION                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_LOAD_VERSION)
;



;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_LOADED
Language        = English
DriverEntry() returned %2.
.

;
;#define PSM_ADMIN_INFO_LOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_LOADED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_UNLOADED
Language        = English
Unloaded.
.

;
;#define PSM_ADMIN_INFO_UNLOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_UNLOADED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DEFAULT_CONTAINER_ENTRY_SUCCESS
Language        = English
Read and processed default Persistent Container %2 
.

;
;#define PSM_ADMIN_INFO_DEFAULT_CONTAINER_ENTRY_SUCCESS                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DEFAULT_CONTAINER_ENTRY_SUCCESS)
;



;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_INFO_NO_MORE_SECONDARY_CONTAINERS_OBSOLETE
Language        = English
This Status code is obsolete - it doesn't make any sense to have a status
code called "INFO" with a Severity of Warning! The new status code is
PSM_WARNING_NO_MORE_SECONDARY_CONTAINERS
.

;
;#define PSM_ADMIN_INFO_NO_MORE_SECONDARY_CONTAINERS_OBSOLETE               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_NO_MORE_SECONDARY_CONTAINERS_OBSOLETE)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DSA_LOCAL_SP_IS_OWNER
Language        = English
The local SP does own the PSM LU.
.

;
;#define PSM_ADMIN_INFO_DSA_LOCAL_SP_IS_OWNER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DSA_LOCAL_SP_IS_OWNER)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DSA_LOCAL_SP_IS_NOT_OWNER
Language        = English
The local SP does not own the PSM LU.
.

;
;#define PSM_ADMIN_INFO_DSA_LOCAL_SP_IS_NOT_OWNER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DSA_LOCAL_SP_IS_NOT_OWNER)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_OWNER_SP_COMMIT_SUCCESSFUL
Language        = English
The commit changes were made successfully on the SP owning the PSM.
.

;
;#define PSM_ADMIN_INFO_OWNER_SP_COMMIT_SUCCESSFUL         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_OWNER_SP_COMMIT_SUCCESSFUL)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DSA_COMMIT_SUCCESSFUL
Language        = English
The DSA commit request was received and successfully completed. 
.

;
;#define PSM_ADMIN_INFO_DSA_COMMIT_SUCCESSFUL         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_DSA_COMMIT_SUCCESSFUL)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DATAAREA_CLOSED_AUTOMATICALLY
Language        = English
A client closed the PSM handle without closing a data area. The data area (%2) was closed 
.

;
;#define PSM_ADMIN_INFO_DATAAREA_CLOSED_AUTOMATICALLY   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DATAAREA_CLOSED_AUTOMATICALLY)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_SOFT_IO_FAILURE
Language        = English
Internal information only, no action necessary.
Persistent Storage Manager encountered a SOFT IO Error: %2.
If the Error is on the Local Path, the Persistent Storage Manager
will fail over to the DSA Path.
If the Error is on the DSA Path, the Persistent Storage Manager
will fail over to the Local Path.
.

;
;#define PSM_ADMIN_INFO_SOFT_IO_FAILURE     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_SOFT_IO_FAILURE)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DATA_AREA_DELETED
Language        = English
Internal Information only. PSM File "%2" Deleted.
.

;
;#define PSM_ADMIN_INFO_DATA_AREA_DELETED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DATA_AREA_DELETED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Info
;//
;// Symptom of Problem: An PSM was asked to Expand a Data Area to its current size.
;//                     This is an idempotent operation, this status code is 
;//                     for Clients who might want to know that the Expansion
;//                     was unnecessary.
;//                     
;//
;//
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_EXPAND_TO_CURRENT_SIZE
Language        = English
PSM was asked to Expand a Data Area to its current size.
.

;
;#define PSM_ADMIN_INFO_EXPAND_TO_CURRENT_SIZE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_EXPAND_TO_CURRENT_SIZE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 24
;//
;// Usage:              Internal
;//
;// Severity:           Info
;//
;// Symptom of Problem: PSM is reinitialized, because the superblock is invalid.
;//
;//
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_CONTAINER_FORMATTED
Language        = English
Internal information only, no action needed.
The PSM Container has been formatted, as a valid PSM signature was not found in the superblock.
.

;
;#define PSM_ADMIN_INFO_CONTAINER_FORMATTED  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_CONTAINER_FORMATTED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 32
;//
;// Usage:              Internal
;//
;// Severity:           Info
;//
;// Symptom of Problem: psmDataAreaWritePrepareBuffer() dirtied the PSM Container IO Buffer.
;//                     If psmDataAreaWritePrepareBuffer() did not dirty the PSM Container,
;//                     psmDataAreaZeroFillLeaves() can call psmDataAreaClearExtents() 
;//                     - which uses the FLARE ZERO_FILL IOCTL.
;//
;//
MessageId       = +1
Severity        = Informational
Facility        = Psm
SymbolicName    = PSM_INFO_DATA_WRITE_PREPARE_BUFFER_SECTOR_LOADED
Language        = English
Internal information only, no action needed.
.

;
;#define PSM_ADMIN_INFO_DATA_WRITE_PREPARE_BUFFER_SECTOR_LOADED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_INFO_DATA_WRITE_PREPARE_BUFFER_SECTOR_LOADED)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x4000
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_GENERIC
Language        = English
Generic PSM Warning.
.

;
;#define PSM_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DATA_AREA_OPEN_DEFERRED
Language        = English
The Data Area Open is deferred, will try again...
.

;
;#define PSM_ADMIN_WARNING_DATA_AREA_OPEN_DEFERRED                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DATA_AREA_OPEN_DEFERRED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DEFAULT_CONTAINER_ENTRY_FAILURE
Language        = English
Could not read and process Persistent Container %2, see Data for Error. 
.

;
;#define PSM_ADMIN_WARNING_DEFAULT_CONTAINER_ENTRY_FAILURE                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WANRING_DEFAULT_CONTAINER_ENTRY_FAILURE)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_CONTAINER_NOT_FOUND
Language        = English
The Persistent Container for this Data Area was not found.
.

;
;#define PSM_ADMIN_WARNING_CONTAINER_NOT_FOUND                                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_CONTAINER_NOT_FOUND)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DATA_AREA_NOT_FOUND
Language        = English
The Data Area was not found.
.

;
;#define PSM_ADMIN_WARNING_DATA_AREA_DESCRIPTOR_NOT_FOUND                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DATA_AREA_NOT_FOUND)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_ILLEGAL_WRITE_TO_READ_ONLY_DATA_AREA
Language        = English
An attempt was made to write to a Data Area opened for read only.
.

;
;#define PSM_ADMIN_WARNING_ILLEGAL_WRITE_TO_READ_ONLY_DATA_AREA                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_ILLEGAL_WRITE_TO_READ_ONLY_DATA_AREA)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_ILLEGAL_READ_FROM_WRITE_ONLY_DATA_AREA
Language        = English
An attempt was made to read from a Data Area opened for write only.
.

;
;#define PSM_ADMIN_WARNING_ILLEGAL_READ_FROM_WRITE_ONLY_DATA_AREA               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_ILLEGAL_READ_FROM_WRITE_ONLY_DATA_AREA)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_CANNOT_REMOVE_CONTAINER_WITH_OPEN_DATA_AREA
Language        = English
An attempt was made to remove a Persistent Container with an open Data Area.
.

;
;#define PSM_ADMIN_WARNING_CANNOT_REMOVE_CONTAINER_WITH_OPEN_DATA_AREA         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_CANNOT_REMOVE_CONTAINER_WITH_OPEN_DATA_AREA)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DATA_AREA_ALREADY_OPEN
Language        = English
An attempt was made to open an already open Data Area.
.

;
;#define PSM_ADMIN_WARNING_DATA_AREA_ALREADY_OPEN         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DATA_AREA_ALREADY_OPEN)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_CANNOT_DELETE_OPEN_DATA_AREA
Language        = English
An attempt was made to delete an open Data Area.
.

;
;#define PSM_ADMIN_WARNING_CANNOT_DELETE_OPEN_DATA_AREA         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_CANNOT_DELETE_OPEN_DATA_AREA)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_ADDING_DUPLICATE_CONTAINER
Language        = English
An attempt was made to add a Persistent Container with the same name as an existing Persistent Container.
.

;
;#define PSM_ADMIN_WARNING_ADDING_DUPLICATE_CONTAINER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_ADDING_DUPLICATE_CONTAINER)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_ADDING_DEFAULT_CONTAINER
Language        = English
An attempt was made to add a Persistent Container with the same name as the default Persistent Container.
.

;
;#define PSM_ADMIN_WARNING_ADDING_DEFAULT_CONTAINER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_ADDING_DEFAULT_CONTAINER)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_CANNOT_DELETE_DEFAULT_CONTAINER
Language        = English
An attempt was made to delete the default Persistent Container.
.

;
;#define PSM_ADMIN_WARNING_CANNOT_DELETE_DEFAULT_CONTAINER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_CANNOT_DELETE_DEFAULT_CONTAINER)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_WRONG_API_VERSION
Language        = English
Input buffer has the wring API revision.
.

;
;#define PSM_ADMIN_WARNING_WRONG_API_VERSION         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_WRONG_API_VERSION)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_REMOVAL_OF_NON_EXISTENT_CONTAINER
Language        = English
An attempt was made to remove a Persistent Container that does not exist.
.

;
;#define PSM_ADMIN_WARNING_REMOVAL_OF_NON_EXISTENT_CONTAINER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_REMOVAL_OF_NON_EXISTENT_CONTAINER)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DSA_CANNOT_PROCESS_DSA_REQUEST
Language        = English
The Local SP does own the PSM LU, and cannot process a DSA request from the Peer.
.

;
;#define PSM_ADMIN_WARNING_DSA_CANNOT_PROCESS_DSA_REQUEST         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DSA_CANNOT_PROCESS_DSA_REQUEST)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_FLARE_TIMEOUT_RETRY_REQUEST
Language        = English
The PSM container lock was not released in specified time.
.

;
;#define PSM_ADMIN_WARNING_FLARE_TIMEOUT_RETRY_REQUEST         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_FLARE_TIMEOUT_RETRY_REQUEST)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DATAAREA_CLOSED_AUTOMATICALLY
Language        = English
A client closed the PSM handle without closing a data area. The data area (%2) was closed 
.

;
;#define PSM_ADMIN_WARNING_DATAAREA_CLOSED_AUTOMATICALLY   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DATAAREA_CLOSED_AUTOMATICALLY)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM could not find (or create) a Data Area Lock
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DATA_AREA_LOCK_NOT_FOUND
Language        = English
PSM could not find (or create) a Data Area Lock it was trying to manipulate.
.

;
;#define PSM_ADMIN_WARNING_DATA_AREA_LOCK_NOT_FOUND   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DATA_AREA_LOCK_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: A slot has been re-used ind the PSM Read Wite Annals
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DIAGNOSTIC_READ_WRITE_ANNALS_SLOT_REUSED
Language        = English
A slot has been re-used ind the PSM Read Wite Annals
.

;
;#define PSM_ADMIN_WARNING_DIAGNOSTIC_READ_WRITE_ANNALS_SLOT_REUSED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DIAGNOSTIC_READ_WRITE_ANNALS_SLOT_REUSED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM write-once data areas cannot be created
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_WRITE_ONCE_NOT_SUPPORTED
Language        = English
The write-once feature is not supported in this revision of PSM
.

;
;#define PSM_ADMIN_WARNING_WRITE_ONCE_NOT_SUPPORTED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_WRITE_ONCE_NOT_SUPPORTED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM cannot open a write-once data area
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_WRITE_ONCE_DATA_AREA_ALREADY_WRITTEN
Language        = English
The write-once data area has been opened again for a write operation
.

;
;#define PSM_ADMIN_WARNING_WRITE_ONCE_DATA_AREA_ALREADY_WRITTEN   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_WRITE_ONCE_DATA_AREA_ALREADY_WRITTEN)
;


;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM cannot process a DSA message sent by the peer
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DSA_INVALID_VERSION
Language        = English
PSM received a DSA message from the peer with an invalid version
.

;
;#define PSM_ADMIN_WARNING_DSA_INVALID_VERSION   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DSA_INVALID_VERSION)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM cannot process a DSA request message sent by the peer
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DSA_INVALID_REQUEST
Language        = English
PSM received an invalid DSA request message from the peer that cannot be decoded
.

;
;#define PSM_ADMIN_WARNING_DSA_INVALID_REQUEST   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DSA_INVALID_REQUEST)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM cannot process a DSA response message sent by the peer
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DSA_INVALID_RESPONSE
Language        = English
PSM received an invalid DSA response message from the peer that cannot be decoded
.

;
;#define PSM_ADMIN_WARNING_DSA_INVALID_RESPONSE   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DSA_INVALID_RESPONSE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM data area expand operation fails
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_EXPAND_NOT_SUPPORTED
Language        = English
The data area expand feature is not supported in this revision of PSM
.

;
;#define PSM_ADMIN_WARNING_EXPAND_NOT_SUPPORTED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_EXPAND_NOT_SUPPORTED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM data area expand operation fails
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_SHRINK_NOT_SUPPORTED
Language        = English
The data area expand feature is being asked to shrink a Data Area in a Revision
of PSM that does not support Data Area Shrinkage.
.

;
;#define PSM_ADMIN_WARNING_SHRINK_NOT_SUPPORTED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_SHRINK_NOT_SUPPORTED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM data area expand operation fails
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_CURRENT_SIZE_CHECK_FAILED
Language        = English
The current size of the data area in the input buffer is not the same as the 
actual size of the data area
.

;
;#define PSM_ADMIN_WARNING_CURRENT_SIZE_CHECK_FAILED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_CURRENT_SIZE_CHECK_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM Copy Extents runs too many times
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_COPY_EXTENTS_LOCAL_FSM_ITER_COUNT_EXCEEDED
Language        = English
psmStorageCopyExtentsFSMRun() has iterated too many times. This is an 
internal PSM error, and is re-tryable.
.

;
;#define PSM_ADMIN_WARNING_COPY_EXTENTS_LOCAL_FSM_ITER_COUNT_EXCEEDED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_COPY_EXTENTS_LOCAL_FSM_ITER_COUNT_EXCEEDED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM data area expand operation fails
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_EXPAND_WRITE_ONCE_FAILED
Language        = English
An expand operation was attempted on a write-once data area and was rejected.
.

;
;#define PSM_ADMIN_WARNING_EXPAND_WRITE_ONCE_FAILED   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_EXPAND_WRITE_ONCE_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 22
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: PSM data area expand operation fails
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_INVALID_CONSOLIDATION_SIDE
Language        = English
A valid side to consolidate all data in the data area was not found.
.

;
;#define PSM_ADMIN_WARNING_INVALID_CONSOLIDATION_SIDE   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_INVALID_CONSOLIDATION_SIDE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 32 (Inyo)
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: Used to let peer SP know it is also running Inyo,
;//                     so do not use DSA.
;//                     
;//
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_DO_NOT_USE_DSA
Language        = English
Both SPs are running Inyo, so do not use DSA.
.

;
;#define PSM_ADMIN_WARNING_DO_NOT_USE_DSA   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_DO_NOT_USE_DSA)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_WARNING_EMPTY_DATA_AREA_NAME
Language        = English
Received request to delete or open a data area with invalid (empty) name.
.

;
;#define PSM_ADMIN_WARNING_INVALID_DATA_AREA_NAME                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_EMPTY_DATA_AREA_NAME)
;
;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x8000
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_GENERIC
Language        = English
Generic PSM Error Code.
.

;
;#define PSM_ADMIN_ERROR_GENERIC                                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_GENERIC)
;                              
;//-----------------------------------------------------------------------------
;// K10 Admin Lib Error Code
;// K10_PSMADMIN_ERROR_BAD_DBID
;//-----------------------------------------------------------------------------
;#define K10_PSMADMIN_ERROR_BAD_DBID  PSM_ADMIN_ERROR_GENERIC  
;
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_ADMIN_BAD_OPC
Language        = English
.

;
;#define PSM_ADMIN_ERROR_ADMIN_BAD_OPC                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_ADMIN_BAD_OPC)
;
;//-----------------------------------------------------------------------------
;// K10 Admin Lib Error Code
;// K10_PSMADMIN_ERROR_BAD_OPC
;//-----------------------------------------------------------------------------
;#define K10_PSMADMIN_ERROR_BAD_OPC  PSM_ADMIN_ERROR_ADMIN_BAD_OPC
;
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_ADMIN_BAD_ITEMSPEC
Language        = English
.

;
;#define PSM_ADMIN_ERROR_ADMIN_BAD_ITEMSPEC                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_ADMIN_BAD_ITEMSPEC)
;
;//-----------------------------------------------------------------------------
;// K10 Admin Lib Error Code
;// K10_PSMADMIN_ERROR_BAD_ITEMSPEC
;//-----------------------------------------------------------------------------
;#define K10_PSMADMIN_ERROR_BAD_ITEMSPEC  PSM_ADMIN_ERROR_ADMIN_BAD_ITEMSPEC
;
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_ADMIN_IOCTL_TIMEOUT
Language        = English
.

;
;#define PSM_ADMIN_ERROR_ADMIN_IOCTL_TIMEOUT                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_ADMIN_IOCTL_TIMEOUT)
;
;//-----------------------------------------------------------------------------
;// K10 Admin Lib Error Code
;// K10_PSMADMIN_ERROR_IOCTL_TIMEOUT
;//-----------------------------------------------------------------------------
;#define K10_PSMADMIN_ERROR_IOCTL_TIMEOUT  PSM_ADMIN_ERROR_ADMIN_IOCTL_TIMEOUT
;
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_ADMIN_BAD_DATA
Language        = English
.

;
;#define PSM_ADMIN_ERROR_ADMIN_BAD_DATA                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_ADMIN_BAD_DATA)
;
;//-----------------------------------------------------------------------------
;// K10 Admin Lib Error Code
;// K10_PSMADMIN_ERROR_BAD_DATA
;//-----------------------------------------------------------------------------
;#define K10_PSMADMIN_ERROR_BAD_DATA  PSM_ADMIN_ERROR_ADMIN_BAD_DATA
;
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_INVALID_DISK_SECTOR_SIZE
Language        = English
Attempt to use a Persistent Container with an sector size not equal to PSM_EXTENT_SIZE .
.

;
;#define PSM_ADMIN_ERROR_INVALID_DISK_SECTOR_SIZE                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_INVALID_DISK_SECTOR_SIZE)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_INVALID_CONTAINER_SIZE
Language        = English
Attempt to validate a Persistent Container that is too small to be useful.
.

;
;#define PSM_ADMIN_ERROR_INVALID_CONTAINER_SIZE                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_INVALID_CONTAINER_SIZE)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_CONTAINER_UNEXPECTED_STATE
Language        = English
While processing an the Default Container was found to be neither INCHOATE or COMPLETE.
.

;
;#define PSM_ADMIN_BUGCHECK_CONTAINER_UNEXPECTED_STATE                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_CONTAINER_UNEXPECTED_STATE)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_UNEXPECTED_IO_FAILURE
Language        = English
PSM LU FLARE Error: %2 (PSM_ERROR_UNEXPECTED_IO_FAILURE)
.

;
;#define PSM_ADMIN_ERROR_UNEXPECTED_IO_FAILURE                                                                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_UNEXPECTED_IO_FAILURE)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DEFAULT_LU_NOT_SET_IN_REGISTRY
Language        = English
The Persistent Storage Manager Default LU key is missing from the Registry.
.

;
;#define PSM_ADMIN_BUGCHECK_DEFAULT_LU_NOT_SET_IN_REGISTRY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DEFAULT_LU_NOT_SET_IN_REGISTRY)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_OPEN_DATA_AREA_FOR_READ_WITH_NO_VALID_COPY
Language        = English
The Persistent Storage Manager opened a Data Area for Read with no valid information.
.

;
;#define PSM_ADMIN_BUGCHECK_OPEN_DATA_AREA_FOR_READ_WITH_NO_VALID_COPY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_OPEN_DATA_AREA_FOR_READ_WITH_NO_VALID_COPY)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_OPEN_DATA_AREA_FOR_WRITE_WITH_NO_VALID_COPY
Language        = English
The Persistent Storage Manager opened a Data Area for Write with no valid information.
.

;
;#define PSM_ADMIN_BUGCHECK_OPEN_DATA_AREA_FOR_WRITE_WITH_NO_VALID_COPY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_OPEN_DATA_AREA_FOR_WRITE_WITH_NO_VALID_COPY)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_CONTAINER_LOCK_TIMEOUT
Language        = English
A Flare IO request did not complete within 120 seconds.
.

;
;#define PSM_ADMIN_BUGCHECK_CONTAINER_LOCK_TIMEOUT                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_CONTAINER_LOCK_TIMEOUT)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_OPEN_DATA_AREA_FOR_RW_WITH_NO_VALID_COPY
Language        = English
The Persistent Storage Manager opened a Data Area for R/W with no valid information.
.

;
;#define PSM_ADMIN_BUGCHECK_OPEN_DATA_AREA_FOR_RW_WITH_NO_VALID_COPY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_OPEN_DATA_AREA_FOR_RW_WITH_NO_VALID_COPY)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_INVALID_DAD_ENTRY
Language        = English
Found a dad with either an invalid DadValidCopy byte or a bad offset.
.

;
;#define PSM_ADMIN_BUGCHECK_INVALID_DAD_ENTRY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_INVALID_DAD_ENTRY)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_INVALID_DATA_AREA_NAME
Language        = English
Received request to delete or open a data area with invalid name.
.

;
;#define PSM_ADMIN_BUGCHECK_INVALID_DATA_AREA_NAME                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_INVALID_DATA_AREA_NAME)
;
;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_INVALID_DATA_AREA_OPEN_PARAMETER
Language        = English
Received request to open a data area with either invalid access mode or invalid data flag.
.

;
;#define PSM_ADMIN_ERROR_INVALID_DATA_AREA_OPEN_PARAMETER                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_INVALID_DATA_AREA_OPEN_PARAMETER)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_ERROR_UNKNOWN_VERSION_DETECTED
Language        = English
Unknown version of PSM LU detected (%2). 
.

;
;#define PSM_ADMIN_ERROR_UNKNOWN_VERSION_DETECTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_ERROR_UNKNOWN_VERSION_DETECTED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: The Lock Sty's Semaphore has been released more times than
;//                     is has been acquired.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_LOCK_STY_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PSty
[2]PSty -> Semaphore.Limit
[3] __LINE__
[4] PSty -> Semaphore.Header.SignalState

.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_LOCK_STY_SEMAPHORE_LIMIT_EXCEEDED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_LOCK_STY_SEMAPHORE_LIMIT_EXCEEDED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM is trying to unlock a Data Area,  but the Lock 
;//                     does not exist
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_LOCK_UNLOCK_NOT_FOUND
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PHandle
[2] PName
[3] __LINE__
[4] PSty
.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_LOCK_UNLOCK_NOT_FOUND                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_LOCK_UNLOCK_NOT_FOUND)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM is trying to find a Data Area Lock, but did
;//                     not specify a Data Name or Handle
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_LOCK_FIND_LOCK_NO_HANDLE_OR_NAME
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PSty
[2] 0
[3] __FILE__
[4] 0

.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_LOCK_FIND_LOCK_NO_HANDLE_OR_NAME                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_LOCK_FIND_LOCK_NO_HANDLE_OR_NAME)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM is assign a Handle to a Data Area Lock, 
;//                     and the lock does not exist
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NOT_FOUND
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PName
[2] 0
[3] __LINE__
[4] PSty
.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NOT_FOUND                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NOT_FOUND)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM is assign a Handle to a Data Area Lock, 
;//                     but does not hold a Reference to the Lock
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NO_REFERENCES
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PName
[2] PLock
[3] __LINE__
[4] PSty
.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NO_REFERENCES                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_LOCK_SET_HANDLE_NO_REFERENCES)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM is assign a Handle to a Data Area Lock, 
;//                     but does not hold a Reference to the Lock
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_RELEASE_LOCK_NEGATIVE_REF_COUNT
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] PLock
[2] Lock Name
[3] __LINE__
[4] Reference Count
.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_RELEASE_LOCK_NEGATIVE_REF_COUNT                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_RELEASE_LOCK_NEGATIVE_REF_COUNT)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: PSM a Data Area has been left open longer than the Client
;//                     specified maximum timeout
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATA_AREA_ACCESS_TIMER_EXCEEDED
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] Data Area Node
[2] Timer
[3] __LINE__
[4] 
.

;
;#define PSM_ADMIN_BUGCHECK_DATA_AREA_ACCESS_TIMER_EXCEEDED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATA_AREA_ACCESS_TIMER_EXCEEDED)
;


;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: An SP has tried to invalidate the Peer SPs' Dad Cache when it
;//                     does not hold the PSM CintainerLock 
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_CONTAINER_INVALIDATE_PEER_CACHE_CONTAINER_NOT_LOCKED
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] Container
[2] Container Lock Held?
[3] __LINE__
[4] Container Lock
.

;
;#define PSM_ADMIN_BUGCHECK_CONTAINER_INVALIDATE_PEER_CACHE_CONTAINER_NOT_LOCKED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_CONTAINER_INVALIDATE_PEER_CACHE_CONTAINER_NOT_LOCKED)
;

;//------------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: An SP has tried to invalidate the Local SPs' Dad Cache 
;//                     when the Local SP holds the Container Lock
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_CONTAINER_INVALIDATE_CACHE_FOM_PEER_CONTAINER_LOCKED
Language        = English
Internal Error in the Ehanced Data Area Locking subsystem.
[1] Container
[2] Container Lock Held?
[3] __LINE__
[4] Container Lock
.

;
;#define PSM_ADMIN_BUGCHECK_CONTAINER_INVALIDATE_CACHE_FOM_PEER_CONTAINER_LOCKED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_CONTAINER_INVALIDATE_CACHE_FOM_PEER_CONTAINER_LOCKED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_NO_MORE_SECONDARY_CONTAINERS
Language        = English
There are no more Secondary LUs on the Persistent Container List. This is not an error.
.

;
;#define PSM_ADMIN_WARNING_NO_MORE_SECONDARY_CONTAINERS               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_NO_MORE_SECONDARY_CONTAINERS)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 18
;//
;// Usage:              Warning
;//
;// Severity:           Error
;//
;// Symptom of Problem: The Persisent Storage Manager cannot process IO to the
;//                     FLARE PSM LU on either the Local or the DSA Path
;//
MessageId       = +1
Severity        = Warning
Facility        = Psm
SymbolicName    = PSM_WARNING_HARD_IO_FAILURE
Language        = English
The Persistent Storage Manager encountered a HARD IO Error: %2.
The Persistent Storage Manager could not process IO to FLARE, 
it encountered an error on both the Local path and DSA path.
Please check the PSM LU.
.

;
;#define PSM_ADMIN_WARNING_HARD_IO_FAILURE               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_WARNING_HARD_IO_FAILURE)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 19
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: SP has not received a response from Flare for its IO
;//                     request
;//
;//
MessageId   = +1
Severity    = Error
Facility    = Psm
SymbolicName    = PSM_BUGCHECK_IO_REQUEST_TIMEOUT
Language    = English
Internal Error in the PSM Storage subsystem
[1] Container
[2] Device Object
[3] IRP
[4] __LINE__
.

;
;#define PSM_ADMIN_BUGCHECK_IO_REQUEST_TIMEOUT                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_IO_REQUEST_TIMEOUT)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 24
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: The semaphore limit for the IO buffer has been 
;//                     exceeded in the PSM IO subsystem.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_DATAAREA_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
Internal Error in the PSM IO subsystem.
[1] Open Data Area Node
[2] IO Buffer Semaphore
[3] Status of IO operation
[4] __LINE__
.

;
;#define PSM_ADMIN_BUGCHECK_DATAAREA_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_DATAAREA_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 24
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: The semaphore limit for the IO buffer has been 
;//                     exceeded in the PSM IO subsystem.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_STORAGE_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
Internal Error in the PSM IO subsystem.
[1] IO Buffer Semaphore
[2] __LINE__ 
.

;
;#define PSM_ADMIN_BUGCHECK_STORAGE_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_STORAGE_IOBUFFER_SEMAPHORE_LIMIT_EXCEEDED)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 24
;//
;// Usage:              BugCheck
;//
;// Severity:           Error
;//
;// Symptom of Problem: An SP has tried to perform a container operation
;//                     without holding the container lock
;//
;//
MessageId       = +1
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_BUGCHECK_CONTAINER_NOT_LOCKED
Language        = English
Internal Error in the Container/Data Area Locking subsystem.
[1] Container
[2] Container Lock Held?
[3] __LINE__
[4] Container Lock
.

;
;#define PSM_ADMIN_BUGCHECK_CONTAINER_NOT_LOCKED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_BUGCHECK_CONTAINER_NOT_LOCKED)
;

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0xC000
Severity        = Error
Facility        = Psm
SymbolicName    = PSM_CRITICAL_ERROR_GENERIC
Language        = English
Generic PSM Critical Error Code.
.

;
;#define PSM_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PSM_CRITICAL_ERROR_GENERIC)
;


;#endif
