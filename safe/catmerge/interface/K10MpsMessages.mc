;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;//++
;// File:            K10MpsMessages.h (MC)
;//
;// Description:     This file contains the definitions for MPS events that
;//                  are logged.  This file also contains UMps return values
;//                  and MPS bug check codes.
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;// The MPS standard for further classifying errors breaks down each range
;// into the following:
;//
;// Log Events:         0x0000-0x03FF "Information"             (1024 possible)
;//                     0x4000-0x43FF "Warning"                 (1024 possible)
;//                     0x8000-0x81FF "Error No Bug Check"      ( 512 possible)
;//                     0x8200-0x83FF "Error Bug Check"         ( 512 possible)
;//                     0xC000-0xC3FF "Critical"                (1024 possible)
;//
;// UMps Return Values: 0x8400-0x87FF                           (1024 possible)
;//
;// Bug Codes:          0x8800-0x8BFF                           (1024 possible)
;//
;// History:         28-Feb-00       ALT     Created
;//--
;
;#ifndef __K10_MPS_MESSAGES__
;#define __K10_MPS_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( Mps           = 0x110         : FACILITY_MPS )


SeverityNames = (
                  Success       = 0x0           : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1           : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2           : STATUS_SEVERITY_WARNING
                  Error         = 0x3           : STATUS_SEVERITY_ERROR
                )

MessageId    = 0x0000
Severity     = Informational
Facility     = Mps
SymbolicName = K10_MPS_DRIVER_LOADED
Language     = English
Driver started.  Compiled: %2
.

MessageId    = +1
Severity     = Informational
Facility     = Mps
SymbolicName = K10_MPS_LOST_CONTACT
Language     = English
Lost contact with %2:%3 on conduit %4.
.

MessageId    = +1
Severity     = Informational
Facility     = Mps
SymbolicName = K10_MPS_DRIVER_UNLOADED
Language     = English
Driver stopped.
.

MessageId    = +1
Severity     = Informational
Facility     = Mps
SymbolicName = K10_MPS_HANDSHAKE_IOCTL_RECEIVED
Language     = English
Received a handshake IOCTL from a trusted kernel mode component.
.

;//
;// MessageId: K10_UMPS_ERROR_SUCCESS
;//
;// MessageText:
;//
;//  The UMps operation completed successfully.
;//
;#define K10_UMPS_ERROR_SUCCESS           ((EMCPAL_STATUS)0x00000000L)
;

MessageId    = 0x8000
Severity     = Error
Facility     = Mps
SymbolicName = K10_MPS_DRIVER_LOAD_FAILED
Language     = English
Driver failed to load.  Compiled: %2
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_MPS_INVALID_IOCTL_SIZE
Language     = English
The %2 IOCTL contains an invalid input or output buffer size.  Might be
mismatched MPS components.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_MPS_UNSUPPORTED_IOCTL
Language     = English
IOCTL %2 is unsupported.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_MPS_LOG_LOAD_ERROR_MESSAGE
Language     = English
The following error was encountered during startup: %2  The driver
cannot start.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_MPS_CONDUIT_CONSISTENCY
Language     = English
CMI Conduit %2 is assumed open when it is not.
.

MessageId    = 0x8400
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_INVALID_PARAMETER
Language     = English
The caller submitted an invalid parameter to UMps.
.
;#define K10_UMPS_ERROR_INVALID_PARAMETER \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_INVALID_PARAMETER ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_DUPLICATE_MAILBOXNAME
Language     = English
The specified mailbox name is already in use.
.
;#define K10_UMPS_ERROR_DUPLICATE_MAILBOXNAME \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_DUPLICATE_MAILBOXNAME ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_INVALID_FILAMENT
Language     = English
The specified filament is invalid for the operation.
.
;#define K10_UMPS_ERROR_INVALID_FILAMENT \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_INVALID_FILAMENT ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_FILAMENT_NOT_OPEN
Language     = English
The filament specified for the transmission is closed on the destination SP.
.
;#define K10_UMPS_ERROR_FILAMENT_NOT_OPEN \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_FILAMENT_NOT_OPEN ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_INVALID_MESSAGE
Language     = English
The specified message does not exist.
.
;#define K10_UMPS_ERROR_INVALID_MESSAGE \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_INVALID_MESSAGE ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_NOT_ENOUGH_MEMORY
Language     = English
There were insufficient resources to complete the request.
.
;#define K10_UMPS_ERROR_NOT_ENOUGH_MEMORY \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_NOT_ENOUGH_MEMORY ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_TIMEOUT
Language     = English
The transmission timed out.
.
;#define K10_UMPS_ERROR_TIMEOUT \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_TIMEOUT ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_USER_APC
Language     = English
A user APC was queued to this thread.
.
;#define K10_UMPS_ERROR_USER_APC \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_USER_APC ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_NO_SYNCHRONOUS_MESSAGE
Language     = English
There was not a synchronous message available for delivery.
.
;#define K10_UMPS_ERROR_NO_SYNCHRONOUS_MESSAGE \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_NO_SYNCHRONOUS_MESSAGE ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_COULD_NOT_REFERENCE_MPS
Language     = English
The UMps DLL could not reference the kernel mode MPS component.
.
;#define K10_UMPS_ERROR_COULD_NOT_REFERENCE_MPS \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_COULD_NOT_REFERENCE_MPS ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_NO_CONNECTION
Language     = English
The specified destination could not be reached.
.
;#define K10_UMPS_ERROR_NO_CONNECTION \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_NO_CONNECTION ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_DESTINATION_CONDUIT_NOT_OPEN
Language     = English
The underlying CMI conduit was not open on the destination SP.
.
;#define K10_UMPS_ERROR_DESTINATION_CONDUIT_NOT_OPEN \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_DESTINATION_CONDUIT_NOT_OPEN ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_ACCESS_VIOLATION
Language     = English
A memory access violation occurred.
.
;#define K10_UMPS_ERROR_ACCESS_VIOLATION \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_ACCESS_VIOLATION ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_BUFFER_TOO_SMALL
Language     = English
Buffer too small during mps recieve .
.
;#define K10_UMPS_ERROR_BUFFER_TOO_SMALL \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_BUFFER_TOO_SMALL ) )
;

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = K10_UMPS_INTERNAL_ERROR_INTERNAL
Language     = English
An internal UMps error occurred.
.
;#define K10_UMPS_ERROR_INTERNAL \
; ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( K10_UMPS_INTERNAL_ERROR_INTERNAL ) )
;

MessageIdTypedef = EMCPAL_STATUS

MessageId    = 0x8800
Severity     = Error
Facility     = Mps
SymbolicName = MPS_DEFAULT_CONDUIT_ALREADY_OPEN
Language     = English
The default CMI conduit that MPS uses for client transmissions was already
open.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_UNDEFINED_ERROR_CODE_MAPPING
Language     = English
There is no mapping from the native EMCPAL_STATUS to a UMps status.  The first
parameter is the unmapable status.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_USER_MODE_CLOSE_NOT_COMPLETE
Language     = English
The user mode close request failed.  The first parameter indicates the close
status code.  The second parameter indicates the filament to be closed.  The
other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_RECV_HANDLE_ALLOC_FAILED
Language     = English
There were insufficient resources to allocate a synchronous receive handle.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CLOSE_CONDUIT_ALLOC_FAILED
Language     = English
There were insufficient resources to allocate a close conduit request.
The first parameter indicates the conduit that was being closed. The other
parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_TRANSMIT_MESSAGE_CONDUIT_CONSISTENCY_ERROR
Language     = English
A CMI Conduit is assumed open when it is not.  The first parameter
indicates the conduit that was assumed open.  The second parameter indicates
the transmission handle.  The third parameter is a pointer to the CMI_SP_ID
of the destination.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_BUFFER_SIZE
Language     = English
CMI reported the IOCTL buffer was an invalid size.  The first indicates the
Ioctl that was issued to CMI.  The second parameter is a pointer to the
IRP that was sent to CMI.  The other paramters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_IRP_ALLOC_FAILED
Language     = English
There were insufficient resources to allocate a IRP to send to CMI.  The
first parameter indicates the Ioctl for which the IRP allocation failed.
The second parameter indicates the amount of memory that is unused by MPS.
The third parameter indicates the maximum memory allowed for MPS.  The
fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_EVENT
Language     = English
An invalid CMI event was received in the CMI callback routine.  The first
parameter indicates the event received.  The second parameter is a pointer
to the data supplied by CMI for the event indicated.  The other parameters
are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_MPS_VERSION_FROM_PEER
Language     = English
MPS received a invalid version in the MPS level acknowledgement from our
peer.  The first parameter indicates the version number received from the
peer.  The second parameter is a pointer to the fixed data message requesting
a different message format.  The third parameter is a pointer to the
transmission handle for the original message.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_HANDLE_MPS_ACK_INVALID_ACK_STATE
Language     = English
The MPS acknowledgement state machine is in an invalid state after
receiving an MPS acknowledgment.  The first parameter is the
present acknowledgement state.  The second parameter is a pointer to the
transmission handle.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_HANDLE_CMI_ACK_INVALID_ACK_STATE
Language     = English
The MPS acknowledgement state machine is in an invalid state after
receiving an CMI transmission notification.  The first parameter is the
present acknowledgement state.  The second parameter is a pointer to the
transmission handle.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_HANDLE_MSG_RETRANSMIT_INVALID_ACK_STATE
Language     = English
The MPS acknowledgement state machine is in an invalid state after
receiving a message retransmission request.  The first parameter is the
present acknowledgement state.  The second parameter is a pointer to the
transmission handle.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_ACK_ALLOC_FAILED
Language     = English
There were insufficient resources to allocate a MPS level acknowledgement.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CLIENT_RECV_ALLOC_FAILED
Language     = English
There were insufficient resources to allocate a received message packet
for the client.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_OPEN_FILAMENT_VERSION_MISMATCH
Language     = English
A user mode open filament request contained an invalid version.  The first
parameter is the version supplied by the caller.  The second parameter is the
current MPS version identifer.  The third parameter is the input buffer.  The
fourth parameter is the output buffer.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CLOSE_FILAMENT_VERSION_MISMATCH
Language     = English
A user mode close filament request contained an invalid version.  The first
parameter is the version supplied by the caller.  The second parameter is the
current MPS version identifer.  The third parameter is the input buffer.  The
fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_SEND_VERSION_MISMATCH
Language     = English
A user mode synchronous send request contained an invalid version.  The first
parameter is the version supplied by the caller.  The second parameter is the
current MPS version identifer.  The third parameter is the input buffer.  The
fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_RECV_VERSION_MISMATCH
Language     = English
A user mode synchronous receive request contained an invalid version.  The first
parameter is the version supplied by the caller.  The second parameter is the
current MPS version identifer.  The third parameter is the input buffer.  The
fourth parameter is the output buffer.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_RELEASE_RECV_MSG_VERSION_MISMATCH
Language     = English
A user mode release received message request contained an invalid version.
The first parameter is the version supplied by the caller.  The second
parameter is the current MPS version identifer.  The third parameter is the
input buffer.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_GET_RECV_MSG_VERSION_MISMATCH
Language     = English
A user mode get received message request contained an invalid version.
The first parameter is the version supplied by the caller.  The second
parameter is the current MPS version identifer.  The third parameter is the
input buffer.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_STATUS_IOCTL
Language     = English
An invalid CMI status was received immediately in the CMI IOCTL call.  The
first parameter indicates the status returned from CMI.  The second
parameter indicates the transmission handle.  The third parameter is a
pointer to the CMI_SP_ID of the destination.  The fourth parameter is the
conduit over which the message was to be sent.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_STATUS_CALLBACK
Language     = English
An invalid CMI status was received in the CMI callback routine.  The
first parameter indicates the status returned from CMI.  The second
parameter indicates the transmission handle.  The third parameter is a
pointer to the CMI transmission information packet.  The fourth parameter
is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_STATUS_REQUEST_RETRANSMISSION
Language     = English
An invalid CMI status was received immediately in the CMI call for a
message retransmission request.  The first parameter indicates the status
returned from CMI.  The second parameter indicates the transmission handle.
The third parameter is a pointer to the CMI_SP_ID of the destination.  The
fourth parameter is the conduit over which the message was to be sent.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CMI_STATUS_MESSAGE_RETRANSMISSION
Language     = English
An invalid CMI status was received immediately in the CMI call for a
message retransmission.  The first parameter indicates the status returned
from CMI.  The second parameter indicates the transmission handle.  The
third parameter is a pointer to the CMI_SP_ID of the destination.  The
fourth parameter is the conduit over which the message was to be sent.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_TRANSMISSION_TYPE
Language     = English
The internal MPS transmission type for the message is invalid.  The first
parameter is a pointer to the received message to be released.  The second
parameter is the memory type of the received message.  The third parameter
is the actual MPS allocated memory.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CLOSE_CONDUIT_CONSISTENCY_ERROR
Language     = English
A CMI Conduit is assumed open when it is not.  The first parameter indicates
the conduit that was assumed open.  The second parameter is a pointer to
the CMI close conduit completion event data.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_INVALID_CLIENT_DISPOSITION
Language     = English
The client returned unexpected status from their MPS callback routine.  The
first parameter indicates the status that was returned.  The second parameter
is a pointer to the filament.  The third parameter is a pointer to the
transmission handle.  The fourth parameter is a pointer to the received
event data tracking structure.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_MEM_QUTOA_EXCEEDED_DURING_LOAD
Language     = English
The memory limit quota for MPS was exceeded when it was set at load
time (programatic error).  The first parameter is the amount of memory
that is unused by MPS.  The second parameter indicates the maximum memory
allowed for MPS.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_IRP_ALLOC_FAILED_AFTER_QUOTA_GUARANTEE
Language     = English
The memory limit quota for MPS was successfully checked, but the IRP
allocation failed.  The first parameter indicates the Ioctl for which the
IRP allocation failed.  The second parameter indicates the amount of memory
that is unused by MPS.  The third parameter indicates the maximum memory
allowed for MPS.  The fourth parameter is 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_SEND_MDL_ALLOC_FAILED_AFTER_QUOTA_GUARANTEE
Language     = English
The memory limit quota for MPS was successfully checked, but the MDL
allocation for a user mode synchronous send failed.  The first parameter
indicates the amount of memory that is unused by MPS.  The second parameter
indicates the maximum memory allowed for MPS.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_RECV_MDL_ALLOC_FAILED_AFTER_QUOTA_GUARANTEE
Language     = English
The memory limit quota for MPS was successfully checked, but the MDL
allocation for a user mode synchronous receive failed.  The first parameter
indicates the amount of memory that is unused by MPS.  The second parameter
indicates the maximum memory allowed for MPS.  The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_NO_PREVIOUS_PROTOCOL_ALLOWED
Language     = English
The first version of MPS received a request for a re-transmission to a previous
protocol version.  The first parameter indicates the transmission handle.
The second parameter is a pointer to the CMI_SP_ID of the destination.
The third and fourth parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CONDUIT_ALREADY_OPEN
Language     = English
The conduit that was provided to MPS on an open filament request was already
opened by another CMI client.  The first parameter indicates the conduit.  The
other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_FREE_ATTEMPT_OF_OUTSTANDING_TRANSMISSION
Language     = English
A client attempted to release a transmission that was still outstanding.  The
first parameter is a pointer to the transmission handle.  The other parameters
are 0.
.


MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_TRANSMISSION_WITHOUT_ACKNOWLEDGEMENT
Language     = English
MPS received a transmission and did not deliver an acknowledgement in an
acceptable amount of time.  The first parameter is a pointer to the DPC
object.  The second parameter is a pointer to the deferred context for the
DPC (the safety net object).  The third and fourth parameters specify the
system arguments passed to the DPC.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SAFETY_NET_ALLOC_FAILED
Language     = English
The allocation for the safety net object failed.  The first parameter specifies
the transmission header for which the object was being allocated.  The other
parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SAFETY_NET_NOT_FOUND
Language     = English
MPS could not locate the internal safety net structure for a transmission.
The first parameter specifies the opaque transmission handle from the sender.
The other parameters are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_RELEASE_RECEIVED_MESSAGE_TIMEOUT
Language     = English
An MPS component did not release a received message in a specified amount
of time.  The first parameter is a pointer to the deferred context for the 
DPC (the client received object). The other parameters are 0. Use the 
!mpsdbg.mpswatch debug extension to identify the MPS component that did not 
release the message.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_OPERATION_VERSION_MISMATCH
Language     = English
A user mode synchronous operation request contained an invalid version.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_SYNC_RESPONSE_VERSION_MISMATCH
Language     = English
A user mode synchronous response contained an invalid version.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_DELIVER_LOST_CONTACT_MEMORY_ALLOC_FAILED
Language     = English
MPS could not allocate memory to send the lost contact notification message.
The first parameter specifies the filament's mailbox name. The other parameters 
are 0.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_USER_FILAMENT_INFO_ALLOC_FAILED
Language     = English
MPS could not allocate memory to internally store the user filament info. The 
first parameter specifies the user filament handle for which the internal
allocation failed. The other parameters are 0. 
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_USER_FILAMENT_LIST_INFO_HEADER_ALLOC_FAILED
Language     = English
MPS could not allocate memory for the header used to internally store 
the user filament list info.
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_USER_MODE_CLOSE_FILAMENT_NOT_FOUND
Language     = English
MPS could not locate the filament while processing the user mode close
request. The first parameter indicates the filament to be closed. The
other parameters are 0. 
.

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_USER_MODE_CLEANUP_FILAMENT_CLOSE_FAILED
Language     = English
MPS could not close the filament during user mode cleanup. The first parameter 
indicates the close status code. The second parameter indicates the filament 
to be closed. The other parameters are 0.
.


MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_FILAMENT_CLOSE_WITH_OUTSTANDING_CLIENT_MESSAGES
Language     = English
While closing a filament, MPS discovered outstanding messages that the client
was responsible for.  The client should process all messages before attempting
to close the filament.  MPS will play mother-hen and cleanup user space messages
since it might be impossible for user space to cleanup in some cases.  The first
parameter indicates the address of the filament. The second parameter indicates
the address of the list of client responsible messages.  The third parameter is
the line number.  The fourth parameter is 0.
.

;//------------------------------------------------
;// Introduced in Release 26
;// Usage: Internal Information Only
;// Severity: Error
;// Symptom of problem: MPS is unable to locate a conduit while walking through 
;//                     the list of open conduits.
;//			

MessageId    = +1
Severity     = Error
Facility     = Mps
SymbolicName = MPS_CONDUIT_NOT_FOUND
Language     = English
MPS is unable to locate conduit. The first paramenter is a pointer to MPS device
extension. The second parameter is the conduit that MPS is unable to locate
in the MPS conduit list. The third parameter is the line number and the fourth
parameter is 0.
.

MessageIdTypedef = EMCPAL_STATUS

;
;#endif // __K10_MPS_MESSAGES__
;

