;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_DISTRIBUTED_LOCK_SERVICE_MESSAGES_H
;#define K10_DISTRIBUTED_LOCK_SERVICE_MESSAGES_H
;
;//
;//++
;// File:            K10DistributedLockServiceMessages.h (MC)
;//
;// Description:     This file defines DLS Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  DLS_xxx and DLS_ADMIN_xxx.
;//
;// History:         03-Mar-00       MWagner     Created
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( Dls= 0x111 : FACILITY_DLS )

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
Facility	= Dls
SymbolicName	= DLS_INFO_GENERIC
Language	= English
Generic Information.
.

;
;#define DLS_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOAD_VERSION
Language	= English
Compiled on %2 at %3, %4.
.

;
;#define DLS_ADMIN_INFO_LOAD_VERSION                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOAD_VERSION)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOADED
Language	= English
DriverEntry() returned %2.
.

;
;#define DLS_ADMIN_INFO_LOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOADED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_UNLOADED
Language	= English
Unloaded.
.

;
;#define DLS_ADMIN_INFO_UNLOADED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_UNLOADED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_INITIALIZED
Language	= English
Initialization completed with a status of %2.
.

;
;#define DLS_ADMIN_INFO_INITIALIZED                                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_INITIALIZED)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOCAL_SP_ID
Language	= English
Local Engine is %2 (SPID is in Data). 
.

;
;#define DLS_ADMIN_INFO_LOCAL_SP_ID                                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOCAL_SP_ID)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_PEER_SP_ID
Language	= English
Peer Engine is %2 (SPID is in Data). 
.

;
;#define DLS_ADMIN_INFO_PEER_SP_ID                                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_PEER_SP_ID)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOCAL_SP_COMPLETING_JOIN
Language	= English
The Local SP is completing a Cabal Join due to the death or absence of the Peer SP.
.

;
;#define DLS_ADMIN_INFO_LOCAL_SP_COMPLETING_JOIN                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOCAL_SP_COMPLETING_JOIN)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOCAL_SP_LIVE_BEING_SENT_TO_PEER
Language	= English
The Local SP is alive, and that information is being returned to the Peer SP.
.

;
;#define DLS_ADMIN_INFO_LOCAL_SP_LIVE_BEING_SENT_TO_PEER                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOCAL_SP_LIVE_BEING_SENT_TO_PEER)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_PEER_SP_LIVE_BEING_SENT_TO_PEER
Language	= English
The Peer SP is alive, and that information is being returned to the Peer SP.
.

;
;#define DLS_ADMIN_INFO_PEER_SP_LIVE_BEING_SENT_TO_PEER                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_PEER_SP_LIVE_BEING_SENT_TO_PEER)
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_LOCAL_SP_LIVE_BEING_RECEIVED_FROM_PEER
Language	= English
The Peer SP reply indicates that the Local SP is alive.
.

;
;#define DLS_ADMIN_INFO_LOCAL_SP_LIVE_BEING_RECEIVED_FROM_PEER                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_LOCAL_SP_LIVE_BEING_RECEIVED_FROM_PEER)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_PEER_SP_LIVE_BEING_RECEIVED_FROM_PEER
Language	= English
The Peer SP reply indicates that the Peer SP is alive.
.

;
;#define DLS_ADMIN_INFO_PEER_SP_LIVE_BEING_RECEIVED_FROM_PEER                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_PEER_SP_LIVE_BEING_RECEIVED_FROM_PEER)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_DELAYED_PEER_SP_JOIN_BEING_PROCESSED
Language	= English
The Local SP is processing previously delayed Join request from 
the Peer SP.
.

;
;#define DLS_ADMIN_INFO_DELAYED_PEER_SP_JOIN_BEING_PROCESSED                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_DELAYED_PEER_SP_JOIN_BEING_PROCESSED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_RESURRECTING_ZOMBIE_PEER
Language	= English
The Peer SP was a Zombie, but is being resurrected to Instantiation %2.
.

;
;#define DLS_ADMIN_INFO_RESURRECTING_ZOMBIE_PEER                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_RESURRECTING_ZOMBIE_PEER)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Informational
Facility	= Dls
SymbolicName	= DLS_INFO_RESURRECTING_ZOMBIE_LOCAL
Language	= English
The Local SP was a Zombie, but is being resurrected to Instantiation %2.
.

;
;#define DLS_ADMIN_INFO_RESURRECTING_ZOMBIE_LOCAL                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_INFO_RESURRECTING_ZOMBIE_LOCAL)
;


;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_GENERIC
Language	= English
Generic Warning.
.

;
;#define DLS_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCKNAME_NOT_FOUND
Language	    = English
Dls internal status that indicates that lock with the specified name has not been found.
.

;
;#define DLS_ADMIN_WARNING_LOCKNAME_NOT_FOUND                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCKNAME_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCKNAME_DUPLICATE
Language	= English
Dls internal status that prevents a lock name from being added twice.
.

;
;#define DLS_ADMIN_WARNING_LOCKNAME_NOT_FOUND                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCKNAME_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCKCABALID_NOT_FOUND
Language	= English
Dls internal status that indicates that a search of the Lock Map by Cabal Lock Id failed.
.

;
;#define DLS_ADMIN_WARNING_LOCKCABALID_NOT_FOUND                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCKCABALID_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_OPEN_PENDING
Language	= English
Dls status that indicates the a lock is being opened. The caller will receive
a call back when the open is complete.
.

;
;#define DLS_ADMIN_WARNING_LOCK_OPEN_PENDING                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_OPEN_PENDING)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_CLOSE_PENDING
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_LOCK_CLOSE_PENDING                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_CLOSE_PENDING)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_CONVERT_PENDING
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_LOCK_CONVERT_PENDING                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_CONVERT_PENDING)
;

MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_FORCEMAILBOX_NO_REMOTE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_FORCEMAILBOX_NO_REMOTE                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_FORCEMAILBOX_NO_REMOTE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_FORCEMAILBOX_PENDING
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_WARNING_FORCEMAILBOX_PENDING                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_FORCEMAILBOX_PENDING)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_ZOMBIE_LOCK_FOUND
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_WARNING_ZOMBIE_LOCK_FOUND                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_ZOMBIE_LOCK_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_CONVERT_DENIED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_WARNING_LOCK_CONVERT_DENIED                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_CONVERT_DENIED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_PEER_BEING_DISMISSED
Language	= English
The Peer SP is being dismissed. Dismissal will be complete when the Peer is a Zombie.
.

;
;#define DLS_ADMIN_WARNING_PEER_BEING_DISMISSED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_PEER_BEING_DISMISSED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_PEER_IS_NOW_A_ZOMBIE
Language	= English
The Peer SP has been dismissed. The Peer SP is now a Zombie.
.

;
;#define DLS_ADMIN_WARNING_PEER_IS_NOW_A_ZOMBIE                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_PEER_IS_NOW_A_ZOMBIE)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_MPS_SEND_RETURNED_BAD_STATUS
Language	= English
An attempt to send a message to the Peer SP returned a bad status: %2.
.

;
;#define DLS_ADMIN_WARNING_MPS_SEND_RETURNED_BAD_STATUS                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_MPS_SEND_RETURNED_BAD_STATUS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_OPEN_DENIED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_WARNING_LOCK_OPEN_DENIED                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_OPEN_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= Dls
SymbolicName	= DLS_WARNING_LOCK_CLOSE_DENIED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_WARNING_WARNING_LOCK_CLOSE_DENIED                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_WARNING_LOCK_CLOSE_DENIED)
;

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_ERROR_GENERIC
Language	= English
Generic Error Code.
.

;
;#define DLS_ADMIN_ERROR_GENERIC                                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_ERROR_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCK_BOOKINGS_LESS_THAN_ZERO
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCK_BOOKINGS_LESS_THAN_ZERO                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCK_BOOKINGS_LESS_THAN_ZERO)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCKMAP_NAME_HASH_OUT_OF_RANGE
Language	= English
Inserting a Lock into the Lock Map returned Hash out of Range Error: %2.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCKMAP_NAME_HASH_OUT_OF_RANGE                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCKMAP_NAME_HASH_OUT_OF_RANGE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCKMAP_INSERT_ERROR
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCKMAP_INSERT_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCKMAP_INSERT_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCKMAP_INCONSISTENCY
Language	= English
A new lock could not be inserted. HT_InsertBucket() returned %2.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCKMAP_INCONSISTENCY                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCKMAP_INCONSISTENCY)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCKMAP_REMOVE_LOCK_ERROR
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCKMAP_REMOVE_LOCK_ERROR                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCKMAP_REMOVE_LOCK_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CABAL_REGISTER_REINITIALIZATION
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CABAL_REGISTER_REINITIALIZATION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CABAL_REGISTER_REINITIALIZATION)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCKMAP_CABALID_HASH_OUT_OF_RANGE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCKMAP_CABALID_HASH_OUT_OF_RANGE                  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCKMAP_CABALID_HASH_OUT_OF_RANGE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_REGISTRATION_BOOKINGS_LESS_THAN_ZERO
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_REGISTRATION_BOOKINGS_LESS_THAN_ZERO                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_REGISTRATION_BOOKINGS_LESS_THAN_ZERO)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLERK_RECEIVED_UNEXPECTED_EVENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CLERK_RECEIVED_UNEXPECTED_EVENT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLERK_RECEIVED_UNEXPECTED_EVENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLERK_COULD_NOT_OPEN_FILAMENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CLERK_COULD_NOT_OPEN_FILAMENT                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLERK_COULD_NOT_OPEN_FILAMENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COULD_NOT_ALLOCATE_MESSAGE
Language	= English
Insert Message here.
.

;//-----------------------------------------------------------------------------
;
;#define DLS_ADMIN_BUGCHECK_COULD_NOT_ALLOCATE_MESSAGE                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COULD_NOT_ALLOCATE_MESSAGE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLERK_UNKNOWN_MESSAGE_TYPE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CLERK_UNKNOWN_MESSAGE_TYPE                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLERK_UNKNOWN_MESSAGE_TYPE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCK_BELL_PULLED_UNKNOWN_LOCK_OP
Language	= English
The Local SP is being asked to process an unknown lock operation.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCK_BELL_PULLED_UNKNOWN_LOCK_OP                                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCK_BELL_PULLED_UNKNOWN_LOCK_OP)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_RECEIVE_LOCK_MESSAGE_UNKNOWN_LOCK_OP
Language	= English
The Local SP is being asked to process an unknown lock operation.
.

;
;#define DLS_ADMIN_BUGCHECK_RECEIVE_LOCK_MESSAGE_UNKNOWN_LOCK_OP                                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_RECEIVE_LOCK_MESSAGE_UNKNOWN_LOCK_OP)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COULD_NOT_ALLOCATE_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_COULD_NOT_ALLOCATE_LOCK                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COULD_NOT_ALLOCATE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCK_REOPEN_WITH_NEW_MAILBOX
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCK_REOPEN_WITH_NEW_MAILBOX                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCK_REOPEN_WITH_NEW_MAILBOX)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_FOR_NONEXISTENT_LOCK                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_DIFF_MAILBOX_SIZE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_DIFF_MAILBOX_SIZE                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_DIFF_MAILBOX_SIZE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCAL_OPEN_LOCK_IN_USE
Language	= English
The Local SP has been asked to open a lock while another Lock
operation is in progress on the Local SP.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCAL_OPEN_LOCK_IN_USE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCAL_OPEN_LOCK_IN_USE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CANNOT_FIND_OR_CREATE_LOCK
Language	= English
The Local SP could not find or create the requested Lock. The
attempt to find or create this Lock failed with a status of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_CANNOT_FIND_OR_CREATE_LOCK                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CANNOT_FIND_OR_CREATE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLOSE_OF_NON_LIVE_LOCK
Language	= English
The Local SP has been asked to Close a Lock that has not been
completely opened, or is the process of being closed.
.

;
;#define DLS_ADMIN_BUGCHECK_CLOSE_OF_NON_LIVE_LOCK                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLOSE_OF_NON_LIVE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_UNEXPECTED_PHASE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_UNEXPECTED_PHASE                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_UNEXPECTED_PHASE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCAL_CLOSE_LOCK_IN_USE
Language	= English
The Local SP has been asked to Close a Lock while another Lock
Operation is in progress.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCAL_CLOSE_LOCK_IN_USE                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCAL_CLOSE_LOCK_IN_USE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLOSE_OF_NONEXISTENT_LOCK
Language	= English
The Local SP has been asked to Close a Lock that does not exist.
.

;
;#define DLS_ADMIN_BUGCHECK_CLOSE_OF_NONEXISTENT_LOCK                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLOSE_OF_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FINDLOCK_UNEXPECTED_STATUS
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_FINDLOCK_UNEXPECTED_STATUS                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FINDLOCK_UNEXPECTED_STATUS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONEXISTENT_LOCK                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONZOMBIE_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONZOMBIE_LOCK                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_FOR_NONZOMBIE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_LOCK_ILLEGAL_TRANSITION
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_LOCK_ILLEGAL_TRANSITION                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_LOCK_ILLEGAL_TRANSITION)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCAL_CONVERT_LOCK_IN_USE
Language	= English
The Local SP has been asked to Convert a Lock while another Lock
Operation is in progress.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCAL_CONVERT_LOCK_IN_USE                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCAL_CONVERT_LOCK_IN_USE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_OF_NONEXISTENT_LOCK
Language	= English
The Local SP has been asked to Convert a Lock that does not exist.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_OF_NONEXISTENT_LOCK                        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_OF_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERTREMOTE_LOCK_ILLEGAL_TRANSITION
Language	= English
The Local SP has been asked to Convert a Lock to an illegal mode
of %x (1 = Read, and 2 = Write).
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERTREMOTE_LOCK_ILLEGAL_TRANSITION              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERTREMOTE_LOCK_ILLEGAL_TRANSITION)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_REPLY_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_REPLY_FOR_NONEXISTENT_LOCK                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_REPLY_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_ANSWER_CALLWAITING_UNEXPECTED_OP
Language	= English
[1] pWorkOrder
[2] pLocalLock
[3] __LINE__
[4] Unexpected Op
.

;
;#define DLS_ADMIN_BUGCHECK_ANSWER_CALLWAITING_UNEXPECTED_OP                         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_ANSWER_CALLWAITING_UNEXPECTED_OP)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCAL_FORCE_MAILBOX_LESS_THAN_WRITE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCAL_FORCE_MAILBOX_LESS_THAN_WRITE                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCAL_FORCE_MAILBOX_LESS_THAN_WRITE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FORCE_MAILBOX_OF_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_FORCE_MAILBOX_OF_NONEXISTENT_LOCK                  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FORCE_MAILBOX_OF_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LOCAL_FORCE_MAILBOX_LOCK_IN_USE
Language	= English
The Local SP has been asked to force a lock's mail nox while another lock
operation is in progress on the Local SP.
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LOCAL_FORCE_MAILBOX_LOCK_IN_USE                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LOCAL_FORCE_MAILBOX_LOCK_IN_USE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FORCE_MAILBOX_WRONG_SIZE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_FORCE_MAILBOX_WRONG_SIZE                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FORCE_MAILBOX_WRONG_SIZE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_UPROOT_MAILBOX_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_UPROOT_MAILBOX_FOR_NONEXISTENT_LOCK                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_UPROOT_MAILBOX_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_UPROOT_MAILBOX_LESS_THAN_WRITE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_UPROOT_MAILBOX_LESS_THAN_WRITE                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_UPROOT_MAILBOX_LESS_THAN_WRITE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_REPLACE_MAILBOX_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_REPLACE_MAILBOX_FOR_NONEXISTENT_LOCK               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_REPLACE_MAILBOX_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_REPLACE_MAILBOX_LESS_THAN_WRITE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_REPLACE_MAILBOX_LESS_THAN_WRITE                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_REPLACE_MAILBOX_LESS_THAN_WRITE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DISMISS_CABAL_MEMBER_COMPLETE_PENDING_FAILED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_DISMISS_CABAL_MEMBER_COMPLETE_PENDING_FAILED       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DISMISS_CABAL_MEMBER_COMPLETE_PENDING_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COMPLETE_PENDEDOPS_UNEXPECTED_OP
Language	= English
Processing pended lock operations on behalf of failing Peer, unexpected operation %2.
.

;
;#define DLS_ADMIN_BUGCHECK_COMPLETE_PENDEDOPS_UNEXPECTED_OP                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COMPLETE_PENDEDOPS_UNEXPECTED_OP)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COMPLETE_PENDEDOPS_BAD_STATUS
Language	= English
Processing pended lock operations on behalf of failing Peer, received bad status %2.
.

;
;#define DLS_ADMIN_BUGCHECK_COMPLETE_PENDEDOPS_BAD_STATUS                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COMPLETE_PENDEDOPS_BAD_STATUS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCODE_JOIN_CABAL_LOCAL_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCODE_JOIN_CABAL_LOCAL_REGISTRATION_NONEXISTENT           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCODE_JOIN_CABAL_LOCAL_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REMOTE_UNEXPECTED_PHASE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REMOTE_UNEXPECTED_PHASE           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REMOTE_UNEXPECTED_PHASE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_NONEXISTENT           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_CLEANUP_LOCK_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_CLEANUP_LOCK_NONEXISTENT           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_CLEANUP_LOCK_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_UNKNOWN_CABAL_OP
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_UNKNOWN_CABAL_OP                                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_UNKNOWN_CABAL_OP)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_REMOTE_ON_NON_HOME_SP
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_REMOTE_ON_NON_HOME_SP                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_REMOTE_ON_NON_HOME_SP)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_INIT_GET_SP_ID_FAILED
Language	= English
The Local SP's attempt to get its SPID failed with an status of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_INIT_GET_SP_ID_FAILED                       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_INIT_GET_SP_ID_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_VALET_INIT_ALLOCATE_POOL_FAILED
Language	= English
The Local SP could not allocate memory for its lock tables.
.

;
;#define DLS_ADMIN_BUGCHECK_VALET_INIT_ALLOCATE_POOL_FAILED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_VALET_INIT_ALLOCATE_POOL_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REGISTRATION_NONEXISTENT
Language	= English
The Local SP could not find a cabal registration while processing a cabal join request.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REGISTRATION_NONEXISTENT                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REMOTE_REGISTRATION_NONEXISTENT
Language	= English
The Local SP could not find the cabal registration for a joining peer.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REMOTE_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REMOTE_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCCHECK_JOIN_CABAL_UNEXPECTED_ERROR
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCCHECK_JOIN_CABAL_UNEXPECTED_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCCHECK_JOIN_CABAL_UNEXPECTED_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COMPLETED_JOIN_CABAL_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_COMPLETED_JOIN_CABAL_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COMPLETED_JOIN_CABAL_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CLERK_COULD_NOT_ClOSE_FILAMENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CLERK_COULD_NOT_ClOSE_FILAMENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CLERK_COULD_NOT_ClOSE_FILAMENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_EXIT_VALET_EXIT_ERROR
Language	= English
The Distributed Lock Service attempt to shut down the Valet failed with a 
status of %2
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_EXIT_VALET_EXIT_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_EXIT_VALET_EXIT_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_EXIT_CLERK_EXIT_ERROR
Language	= English
The Distributed Lock Service attempt to shut down the Clerk failed with a 
status of %2
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_EXIT_CLERK_EXIT_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_EXIT_CLERK_EXIT_ERROR)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_EXIT_EXECUTIONER_EXIT_ERROR
Language	= English
The Distributed Lock Service attempt to shut down the Executioner failed with a 
status of %2
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_EXIT_EXECUTIONER_EXIT_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_EXIT_EXECUTIONER_EXIT_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SP_ID_NODES_ARE_DIFFERENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SP_ID_NODES_ARE_DIFFERENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SP_ID_NODES_ARE_DIFFERENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_INVALID
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_INVALID         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DISMISS_CABAL_MEMBER_REGISTRATION_INVALID)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CANNOT_FIND_OR_CREATE_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CANNOT_FIND_OR_CREATE_LOCK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CANNOT_FIND_OR_CREATE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_CONFLICT_FOR_NONEXISTENT_LOCK
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_CONFLICT_FOR_NONEXISTENT_LOCK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_CONFLICT_FOR_NONEXISTENT_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_COULD_NOT_ALLOCATE_WORKORDER
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_COULD_NOT_ALLOCATE_WORKORDER         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_COULD_NOT_ALLOCATE_WORKORDER)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FOOTMAN_COULD_NOT_REMOVE_GHOST_LOCK
Language	= English
Could not remove a Ghost lock from the Valet tables
 [1] the lock
 [2] unused
 [3] line number
 [4] the bad status
.

;
;#define DLS_ADMIN_BUGCHECK_FOOTMAN_COULD_NOT_REMOVE_GHOST_LOCK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FOOTMAN_COULD_NOT_REMOVE_GHOST_LOCK)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FOOTMAN_COULD_NOT_DESTROY_GHOST_LOCK
Language	= English
Could not destroy a Ghost lock.
 [1] the lock
 [2] unused
 [3] line number
 [4] the bad status
.

;
;#define DLS_ADMIN_BUGCHECK_FOOTMAN_COULD_NOT_REMOVE_GHOST_LOCK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FOOTMAN_COULD_NOT_REMOVE_GHOST_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_COULD_NOT_REMOVE_LOCK_IN_SECRETARY_CLOSE_LOCK_REPLY
Language	= English
The Distributed Lock Service Valet's attempt to Remove a Lock from internal
tables failed with a status of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_COULD_NOT_REMOVE_LOCK_IN_SECRETARY_CLOSE_LOCK_REPLY         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_COULD_NOT_REMOVE_LOCK_IN_SECRETARY_CLOSE_LOCK_REPLY)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_LOCAL_COULD_NOT_REMOVE_LOCK
Language	= English
The Distributed Lock Service Valet's attempt to Remove a Lock from internal
tables failed with a status of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_LOCAL_COULD_NOT_REMOVE_LOCK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_LOCAL_COULD_NOT_REMOVE_LOCK)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERTPEER_FIND_UNEXPECTED_STATUS
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERTPEER_FIND_UNEXPECTED_STATUS         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERTPEER_FIND_UNEXPECTED_STATUS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LEAVE_CABAL_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LEAVE_CABAL_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LEAVE_CABAL_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LEAVE_CABAL_PENDED_OPS
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_LEAVE_CABAL_PENDED_OPS         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LEAVE_CABAL_PENDED_OPS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_EXIT_LEAVE_CABAL_ERROR
Language	= English
The Distributed Lock Service's attempt to leave the Cabal failed with a status
of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_EXIT_LEAVE_CABAL_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_EXIT_LEAVE_CABAL_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_COULD_NOT_INIT_MPS
Language	= English
The Distributed Lock Service attempt to initialize the Message Passing Service failed
with an status of %2.
.

;
;#define DLS_ADMIN_BUGCHECK_BUTLER_COULD_NOT_INIT_MPS         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_BUTLER_COULD_NOT_INIT_MPS)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCCHECK_JOIN_CABAL_SEMAPHORE_WAIT_ERROR
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCCHECK_JOIN_CABAL_SEMAPHORE_WAIT_ERROR         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCCHECK_JOIN_CABAL_SEMAPHORE_WAIT_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REPLY_SEMAPHORE_LIMIT_EXCEEDED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REPLY_SEMAPHORE_LIMIT_EXCEEDED         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REPLY_SEMAPHORE_LIMIT_EXCEEDED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DISMISS_CABAL_MEMBER_LOCAL_PHASE_UNEXPECTED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_DISMISS_CABAL_MEMBER_LOCAL_PHASE_UNEXPECTED         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DISMISS_CABAL_MEMBER_LOCAL_PHASE_UNEXPECTED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_ERSATZ_JOIN_CABAL_MEMBER_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_ERSATZ_JOIN_CABAL_MEMBER_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_ERSATZ_JOIN_CABAL_MEMBER_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DISMISS_CABAL_MEMBER_PEER_PHASE_UNEXPECTED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_DISMISS_CABAL_MEMBER_PEER_PHASE_UNEXPECTED         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DISMISS_CABAL_MEMBER_PEER_PHASE_UNEXPECTED)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_LOCAL_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_AMDIN_BUGCHECK_HANDLE_UNPLEASANTRY_LOCAL_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_LOCAL_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_PEER_REGISTRATION_NONEXISTENT
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_HANDLE_UNPLEASANTRY_PEER_REGISTRATION_NONEXISTENT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_PEER_REGISTRATION_NONEXISTENT)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_INVALID_UNEXPECTED_PEER_STATE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_HANDLE_UNPLEASANTRY_INVALID_UNEXPECTED_PEER_STATE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_INVALID_UNEXPECTED_PEER_STATE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_INCHOATE_UNEXPECTED_PEER_STATE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_HANDLE_UNPLEASANTRY_INCHOATE_UNEXPECTED_PEER_STATE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_INCHOATE_UNEXPECTED_PEER_STATE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_LIVE_UNEXPECTED_PEER_STATE
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_HANDLE_UNPLEASANTRY_LIVE_UNEXPECTED_PEER_STATE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_LIVE_UNEXPECTED_PEER_STATE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_HANDLE_UNPLEASANTRY_UNEXPECTED_LOCAL_STATE
Language	= English
While dismissing the Peer SP, a registration with an unexpected state was encountered (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_HANDLE_UNPLEASANTRY_UNEXPECTED_LOCAL_STATE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_HANDLE_UNPLEASANTRY_UNEXPECTED_LOCAL_STATE)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DOORMAN_ADMIT_REPLY_REGISTRATION_NONEXISTENT
Language	= English
The Dls Doorman, processing a Lock Reply, Read Peer Cabal Registration =  %2.
.

;
;#define DLS_ADMIN_BUGCHECK_DOORMAN_ADMIT_REPLY_REGISTRATION_NONEXISTENT       \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DOORMAN_ADMIT_REPLY_REGISTRATION_NONEXISTENT)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_BUTLER_USURP_ALL_LOCKS_BAD_STATUS
Language	= English
Closing all locks on behalf of failing Peer, received bad status %2.
.

;
;#define DLS_ADMIN_BUGCHECK_DOORMAN_USURP_ALL_LOCKS_BAD_STATUS                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DOORMAN_USURP_ALL_LOCKS_BAD_STATUS)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER
Language	= English
Could not allocate memory to send a Join Request to Peer SP.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER);
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_LEAVE_COULD_NOT_ALLOCATE_CABAL_WORKORDER
Language	= English
Could not allocate memory to send a Leave Notification to Peer SP.
.

;
;#define DLS_ADMIN_BUGCHECK_LEAVE_COULD_NOT_ALLOCATE_CABAL_WORKORDER            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_LEAVE_COULD_NOT_ALLOCATE_CABAL_WORKORDER);
;



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DELAYED_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER
Language	= English
Could not allocate memory to send a Join Request to Peer SP.
.

;
;#define DLS_ADMIN_BUGCHECK_DELAYED_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DELAYED_JOIN_COULD_NOT_ALLOCATE_CABAL_WORKORDER);
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FOOTMAN_COULD_NOT_ADD_LOCK
Language	= English
Could not add a lock to the Distributed Lock Service lock map, status = %2.
.

;
;#define DLS_ADMIN_BUGCHECK_FOOTMAN_COULD_NOT_ADD_LOCK            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FOOTMAN_COULD_NOT_ADD_LOCK);
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_FOOTMAN_COULD_NOT_FIND_LOCK_AFTER_ADDING
Language	= English
Could not find a lock immediately after creation, status = %2.
.

;
;#define DLS_ADMIN_BUGCHECK_FOOTMAN_COULD_NOT_FIND_LOCK_AFTER_ADDING            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_FOOTMAN_COULD_NOT_FIND_LOCK_AFTER_ADDING);
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_INIT_ALLOCATE_POOL_FAILED
Language	= English
Insert Message here.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_INIT_ALLOCATE_POOL_FAILED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_INIT_ALLOCATE_POOL_FAILED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_NAME_EXPIRED
Language	= English
A Lock Operation requested by the Peer SP was not completed in the required time.
The Lock's Name appears as Data
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_NAME_EXPIRED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_NAME_EXPIRED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_CABALID_EXPIRED
Language	= English
A Lock Operation requested by the Peer SP was not completed in the required time.
The Lock's Identifier appears as Data
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_CABALID_EXPIRED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_LOCK_REQUEST_BY_CABALID_EXPIRED)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_ALLOCATION_FAILED
Language	= English
A allocation to add a Lock Request to the list of Requests to be times out failed.
The Lock's Name is in the Data field.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_ALLOCATION_FAILED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_ALLOCATION_FAILED)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_COULD_NOT_ADD_WRIT
Language	= English
A Lock Request could not be added to the list of Requests to be timed out (2x).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_COULD_NOT_ADD_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_NAME_COULD_NOT_ADD_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_ALLOCATION_FAILED
Language	= English
A allocation to add a Lock Request to the list of Requests to be times out failed.
The Lock's Identifier is in the Data field.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_ALLOCATION_FAILED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_ALLOCATION_FAILED)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_COULD_NOT_ADD_WRIT
Language	= English
A Lock Request could not be added to the list of Requests to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_COULD_NOT_ADD_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_COULD_NOT_ADD_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_LOOKUP_FAILED
Language	= English
A Lock Request could not be found on a list of Request to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_LOOKUP_FAILED                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_BY_CABALID_LOOKUP_FAILED)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_FIND_WRIT
Language	= English
A Lock Request could not be found on a list of Request to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_FIND_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_FIND_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_FIND_WRIT
Language	= English
A Lock Request could not be found on a list of Request to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_FIND_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_FIND_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_REMOVE_WRIT
Language	= English
A Lock Request could not be removed from a list of Request to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_REMOVE_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_BY_NAME_COULD_NOT_REMOVE_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_REMOVE_WRIT
Language	= English
A Lock Request could not be removed from a list of Request to be timed out (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_REMOVE_WRIT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_COULD_NOT_REMOVE_WRIT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_CONDEMN_UNEXPECTED_LOCK_OP
Language	= English
Insert message here %2.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_CONDEMN_UNEXPECTED_LOCK_OP                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_CONDEMN_UNEXPECTED_LOCK_OP)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_UNEXPECTED_LOCK_OP
Language	= English
Insert message here %2.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_UNEXPECTED_LOCK_OP                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_UNEXPECTED_LOCK_OP)



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_EXIT_WITH_NAME_WRITS
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_EXIT_WITH_NAME_WRITS                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_EXIT_WITH_NAME_WRITS)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_EXIT_WITH_CABALID_WRITS
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_EXIT_WITH_CABALID_WRITS                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_EXIT_WITH_CABALID_WRITS)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_INVALID_VERSION
Language	= English
The Peer sent an Open Lock Request using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_INVALID_VERSION
Language	= English
The Peer sent an Local Open Reply using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_REPLY_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_INVALID_VERSION
Language	= English
The Peer sent a Close Lock Request using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_LOCK_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_INVALID_VERSION
Language	= English
The Peer sent a Close Lock Reply using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_REPLY_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CONVERT_LOCK_INVALID_VERSION
Language	= English
The Peer sent a Convert Request using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CONVERT_LOCK_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CONVERT_LOCK_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CONVERT_LOCK_REPLY_INVALID_VERSION
Language	= English
The Peer sent a Convert Reply message using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CONVERT_LOCK_REPLY_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CONVERT_LOCK_REPLY_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CONVERT_CONFLICT_INVALID_VERSION
Language	= English
The Peer sent a Convert Conflict using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_CONVERT_CONFLICT_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CONVERT_CONFLICT_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_FORCE_MAILBOX_INVALID_VERSION
Language	= English
The Peer sent a Force Mailbox using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_FORCE_MAILBOX_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_FORCE_MAILBOX_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_PEER_INVALID_VERSION
Language	= English
The Peer sent a Join Request using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_PEER_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_PEER_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REPLY_INVALID_VERSION
Language	= English
The Peer sent a Join Reply message using an invalid Protocol Version (%2).
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REPLY_INVALID_VERSION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REPLY_INVALID_VERSION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_CABAL_NO_LONGER_REQUIRES_YOUR_SERVICES
Language	= English
The Peer SP has asked the Local SP to shut down. The Peer will have logged
the reason for this request.
.

;
;#define DLS_ADMIN_BUGCHECK_CABAL_NO_LONGER_REQUIRES_YOUR_SERVICES                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_CABAL_NO_LONGER_REQUIRES_YOUR_SERVICES)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE
Language	= English
DriverEntry() could not allocate memory for a loggable copy of the compile date.
.

;
;#define DLS_ADMIN_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_DRIVER_ENTRY_COULD_NOT_ALLOCATE_COMPILE_DATE)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REQUEST_PEER_REGISTRATION_NONEXISTENT
Language	= English
The Local SP cannot find the Peer SP's Cabal Registration while processing a Join Cabal Request.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REQUEST_PEER_REGISTRATION_NONEXISTENT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REQUEST_PEER_REGISTRATION_NONEXISTENT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_JOIN_CABAL_REQUEST_REGISTRATION_NONEXISTENT
Language	= English
The Local SP cannot find a Cabal Registration while processing a Join Cabal Request.
.

;
;#define DLS_ADMIN_BUGCHECK_JOIN_CABAL_REQUEST_REGISTRATION_NONEXISTENT                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_JOIN_CABAL_REQUEST_REGISTRATION_NONEXISTENT)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_UNEXPECTED_OPERATION
Language	= English
The Local SP is attemtping to satisfy a request from the Peer SP, but the Local
SP operation is illegal.
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_UNEXPECTED_OPERATION                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_BY_CABALID_UNEXPECTED_OPERATION)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_OPEN_LOCK_UNEXPECTED_PHASE
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_OPEN_LOCK_UNEXPECTED_PHASE  \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_OPEN_LOCK_UNEXPECTED_PHASE)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_UNEXPECTED_PHASE
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_LOCK_UNEXPECTED_PHASE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_UNEXPECTED_PHASE)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_COULD_NOT_REMOVE
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CLOSE_COULD_NOT_REMOVE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CLOSE_LOCK_COULD_NOT_REMOVE)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_SECRETARY_CONVERT_COULD_NOT_FIND_LOCK
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_SECRETARY_CONVERT_COULD_NOT_FIND_LOCK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_SECRETARY_CONVERT_COULD_NOT_FIND_LOCK)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_EXECUTIONER_QUASH_SPURIOUS_WRIT_UNEXPECTED_LOCK_OP
Language	= English
.

;
;#define DLS_ADMIN_BUGCHECK_EXECUTIONER_QUASH_SPURIOUS_WRIT_UNEXPECTED_LOCK_OP                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_EXECUTIONER_QUASH_SPURIOUS_WRIT_UNEXPECTED_LOCK_OP)

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_PETITION_MODE_ASSIGNMENT_ILLEGAL
Language	= English
Illegal Petition Mode assigned to Lock.
[1] pLock
[2] Collective
[3] __LINE__
[4] Mode
.

;
;#define DLS_ADMIN_BUGCHECK_PETITION_MODE_ASSIGNMENT_ILLEGAL                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_PETITION_MODE_ASSIGNMENT_ILLEGAL)


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_AWARDED_MODE_ASSIGNMENT_ILLEGAL
Language	= English
Illegal Awarded Mode assigned to Lock.
[1] pLock
[2] Collective
[3] __LINE__
[4] Mode
.

;
;#define DLS_ADMIN_BUGCHECK_AWARDED_MODE_ASSIGNMENT_ILLEGAL                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_AWARDED_MODE_ASSIGNMENT_ILLEGAL)



;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_WORKORDER_MODE_ASSIGNMENT_ILLEGAL
Language	= English
Illegal Awarded Mode assigned to Lock.
[1] pLock
[2] Collective
[3] __LINE__
[4] Mode
.

;
;#define DLS_ADMIN_BUGCHECK_WORKORDER_MODE_ASSIGNMENT_ILLEGAL                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_WORKORDER_MODE_ASSIGNMENT_ILLEGAL)

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_BUGCHECK_INVALID_DLS_PROTOCOL_VERSION
Language	= English
Insert message here.
.

;
;#define DLS_ADMIN_BUGCHECK_INVALID_DLS_PROTOCOL_VERSION \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_BUGCHECK_INVALID_DLS_PROTOCOL_VERSION)
;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= Dls
SymbolicName	= DLS_CRITICAL_ERROR_GENERIC
Language	= English
Generic Distributed Lock Service Critical Error Code.
.

;
;#define DLS_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(DLS_CRITICAL_ERROR_GENERIC)
;

;#endif
