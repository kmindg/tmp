;//++
;// Copyright (C) EMC Corporation, 2007-2013
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10MluUDIMessages.mc
;//
;// Description:     This file contains the message catalogue for UDI messages.
;//
;// History:
;//
;//--
;//
;// The CLARiiON standard for how to classify errors according to their range is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;
;#ifndef __K10_MLU_UDI_MESSAGES__
;#define __K10_MLU_UDI_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  MLUUDI        = 0x16F : FACILITY_UDI
                )

SeverityNames = ( Success       = 0x0   : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1   : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2   : STATUS_SEVERITY_WARNING
                  Error         = 0x3   : STATUS_SEVERITY_ERROR
                )
 
;
;
;
;//========================
;//======= INFO MSGS ======
;//========================
;
;
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Creation of NAS Server initiated.
;//
;// Generated value should be: 0x616F0000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_SFS_RECEIVED
Language     = English
Request received to create a NAS Server named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: NAS Server creation initiated successfully.
;//
;// Generated value should be: 0x616F0001
;//
MessageId    = 0x0001
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_SFS_COMPLETED
Language     = English
NAS Server %2 creation is initiated successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destruction of NAS Server initiated.
;//
;// Generated value should be: 0x616F0002
;//
MessageId    = 0x0002
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_SFS_RECEIVED
Language     = English
Request received to destroy a NAS Server named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: NAS Server destruction initiated successfully.
;//
;// Generated value should be: 0x616F0003
;//
MessageId    = 0x0003
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_SFS_COMPLETED
Language     = English
NAS Server %2 destruction is initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Creation of File System initiated.
;//
;// Generated value should be: 0x616F0004
;//
MessageId    = 0x0004
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_SF_RECEIVED
Language     = English
Request received to create a File System named %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File System creation initiated successfully.
;//
;// Generated value should be: 0x616F0005
;//
MessageId    = 0x0005
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_SF_COMPLETED
Language     = English
File System %2 creation is initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destruction of File System initiated.
;//
;// Generated value should be: 0x616F0006
;//
MessageId    = 0x0006
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_SF_RECEIVED
Language     = English
Request received to destroy a File System named %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File System destruction initiated successfully.
;//
;// Generated value should be: 0x616F0007
;//
MessageId    = 0x0007
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_SF_COMPLETED
Language     = English
File System %2 destruction was initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0008
;//
MessageId    = 0x0008
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NET_IF_RECEIVED
Language     = English
Request received to create a network interface named %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0009
;//
MessageId    = 0x0009
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NET_IF_COMPLETED
Language     = English
Request to create network interface %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000A
;//
MessageId    = 0x000A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NET_IF_RECEIVED
Language     = English
Request received to destroy network interface %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000B
;//
MessageId    = 0x000B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NET_IF_COMPLETED
Language     = English
Request to destroy network interface %2 completed successfully.
.

;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000C
;//
MessageId    = 0x000C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NET_IF_RECEIVED
Language     = English
Request received to modify network interface %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000D
;//
MessageId    = 0x000D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NET_IF_COMPLETED
Language     = English
Request to modify network interface %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000E
;//
MessageId    = 0x000E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_SET_SFS_PROPERTIES_RECEIVED
Language     = English
Request received to set properties for NAS Server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F000F
;//
MessageId    = 0x000F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_SET_SFS_PROPERTIES_COMPLETED
Language     = English
Request to set properties for NAS Server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0010
;//
MessageId    = 0x0010
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_DNS_RECEIVED
Language     = English
Request received to create dns properties named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0011
;//
MessageId    = 0x0011
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_DNS_COMPLETED
Language     = English
Request to create dns %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0012
;//
MessageId    = 0x0012
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_DNS_RECEIVED
Language     = English
Request received to destroy dns %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0013
;//
MessageId    = 0x0013
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_DNS_COMPLETED
Language     = English
Request to destroy dns %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0014
;//
MessageId    = 0x0014
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_DNS_RECEIVED
Language     = English
Request received to modify DNS properties %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0015
;//
MessageId    = 0x0015
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_DNS_COMPLETED
Language     = English
Request to modify DNS properties %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0016
;//
MessageId    = 0x0016
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NFS_RECEIVED
Language     = English
Request received to create a NFS for NAS Server %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0017
;//
MessageId    = 0x0017
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NFS_COMPLETED
Language     = English
Request to create NFS %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0018
;//
MessageId    = 0x0018
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NFS_RECEIVED
Language     = English
Request received to destroy NFS %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0019
;//
MessageId    = 0x0019
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NFS_COMPLETED
Language     = English
Request to destroy NFS %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001A
;//
MessageId    = 0x001A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NFS_RECEIVED
Language     = English
Request received to modify NFS %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001B
;//
MessageId    = 0x001B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NFS_COMPLETED
Language     = English
Request to modify NFS %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001C
;//
MessageId    = 0x001C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CIFS_RECEIVED
Language     = English
Request received to create a CIFS server named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001D
;//
MessageId    = 0x001D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CIFS_COMPLETED
Language     = English
Request to create CIFS server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001E
;//
MessageId    = 0x001E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CIFS_RECEIVED
Language     = English
Request received to destroy CIFS server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F001F
;//
MessageId    = 0x001F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CIFS_COMPLETED
Language     = English
Request to destroy cifs server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0020
;//
MessageId    = 0x0020
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_CIFS_RECEIVED
Language     = English
Request received to modify CIFS server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0021
;//
MessageId    = 0x0021
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_CIFS_COMPLETED
Language     = English
Request to modify CIFS server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0022
;//
MessageId    = 0x0022
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_JOIN_VDM_CIFS_RECEIVED
Language     = English
Request received to join CIFS server %2 to domain.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0023
;//
MessageId    = 0x0023
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_JOIN_VDM_CIFS_COMPLETED
Language     = English
Request to join CIFS server %2 to domain completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0024
;//
MessageId    = 0x0024
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_UNJOIN_VDM_CIFS_RECEIVED
Language     = English
Request received to unjoin CIFS server %2 from domain.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0025
;//
MessageId    = 0x0025
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_UNJOIN_VDM_CIFS_COMPLETED
Language     = English
Request to unjoin CIFS server %2 from domain completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0026
;//
MessageId    = 0x0026
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ADD_IF_VDM_CIFS_RECEIVED
Language     = English
Request received to add network interface to CIFS server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0027
;//
MessageId    = 0x0027
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ADD_IF_VDM_CIFS_COMPLETED
Language     = English
Request to add network interface to CIFS server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0028
;//
MessageId    = 0x0028
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_REMOVE_IF_VDM_CIFS_RECEIVED
Language     = English
Request received to remove network interface from CIFS server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0029
;//
MessageId    = 0x0029
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_REMOVE_IF_VDM_CIFS_COMPLETED
Language     = English
Request to remove network interface from CIFS server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002A
;//
MessageId    = 0x002A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NFS_EXPORT_RECEIVED
Language     = English
Request received to create NFS export %2 associated with File System %3.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002B
;//
MessageId    = 0x002B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NFS_EXPORT_COMPLETED
Language     = English
Request to create NFS export %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002C
;//
MessageId    = 0x002C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NFS_EXPORT_RECEIVED
Language     = English
Request received to destroy NFS export %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002D
;//
MessageId    = 0x002D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NFS_EXPORT_COMPLETED
Language     = English
Request to destroy NFS export %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002E
;//
MessageId    = 0x002E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NFS_EXPORT_RECEIVED
Language     = English
Request received to modify NFS export %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F002F
;//
MessageId    = 0x002F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NFS_EXPORT_COMPLETED
Language     = English
Request to modify NFS export %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0030
;//
MessageId    = 0x0030
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CIFS_SHARE_RECEIVED
Language     = English
Request received to create CIFS share %2 associated with File System %3.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0031
;//
MessageId    = 0x0031
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CIFS_SHARE_COMPLETED
Language     = English
Request to create CIFS share %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0032
;//
MessageId    = 0x0032
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CIFS_SHARE_RECEIVED
Language     = English
Request received to destroy CIFS share %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0033
;//
MessageId    = 0x0033
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CIFS_SHARE_COMPLETED
Language     = English
Request to destroy CIFS share %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0034
;//
MessageId    = 0x0034
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_CIFS_SHARE_RECEIVED
Language     = English
Request received to modify CIFS share %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0035
;//
MessageId    = 0x0035
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_CIFS_SHARE_COMPLETED
Language     = English
Request to modify CIFS share %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0036
;//
MessageId    = 0x0036
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NDMP_RECEIVED
Language     = English
Request received to create a NDMP for NAS Server %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0037
;//
MessageId    = 0x0037
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NDMP_COMPLETED
Language     = English
Request to create NDMP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0038
;//
MessageId    = 0x0038
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NDMP_RECEIVED
Language     = English
Request received to destroy NDMP %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0039
;//
MessageId    = 0x0039
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NDMP_COMPLETED
Language     = English
Request to destroy NDMP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003A
;//
MessageId    = 0x003A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NDMP_RECEIVED
Language     = English
Request received to modify NDMP %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003B
;//
MessageId    = 0x003B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_NDMP_COMPLETED
Language     = English
Request to modify NDMP %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003C
;//
MessageId    = 0x003C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CAVA_RECEIVED
Language     = English
Request received to create CAVA for NAS Server %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003D
;//
MessageId    = 0x003D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_CAVA_COMPLETED
Language     = English
Request to create CAVA %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003E
;//
MessageId    = 0x003E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CAVA_RECEIVED
Language     = English
Request received to destroy CAVA %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F003F
;//
MessageId    = 0x003F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_CAVA_COMPLETED
Language     = English
Request to destroy CAVA %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0040
;//
MessageId    = 0x0040
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_ASA_RECEIVED
Language     = English
Request received to destroy ASA %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0041
;//
MessageId    = 0x0041
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_ASA_COMPLETED
Language     = English
Request to destroy ASA %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0042
;//
MessageId    = 0x0042
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_ASA_RECEIVED
Language     = English
Request received to create a ASA for NAS Server %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0043
;//
MessageId    = 0x0043
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_ASA_COMPLETED
Language     = English
Request to create ASA %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0044
;//
MessageId    = 0x0044
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_ASA_RECEIVED
Language     = English
Request received to modify an ASA %2 for NAS Server %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0045
;//
MessageId    = 0x0045
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_ASA_COMPLETED
Language     = English
Request to modify ASA %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0046
;//
MessageId    = 0x0046
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_NIS_RECEIVED
Language     = English
Request received to create NIS properties named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0047
;//
MessageId    = 0x0047
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_NIS_COMPLETED
Language     = English
Request to create NIS %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0048
;//
MessageId    = 0x0048
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_NIS_RECEIVED
Language     = English
Request received to destroy NIS %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0049
;//
MessageId    = 0x0049
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_NIS_COMPLETED
Language     = English
Request to destroy NIS %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F04A
;//
MessageId    = 0x004A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_NIS_RECEIVED
Language     = English
Request received to modify NIS properties %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F004B
;//
MessageId    = 0x004B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_NIS_COMPLETED
Language     = English
Request to modify NIS properties %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F004C
;//
MessageId    = 0x004C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_UFS_RECEIVED
Language     = English
Request received to modify File System %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F004D
;//
MessageId    = 0x004D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_UFS_COMPLETED
Language     = English
Request to modify File System %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F004E
;//
MessageId    = 0x004E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_UFS_RECEIVED
Language     = English
Internal information only. Delete recovery process %2 received.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F004F
;//
MessageId    = 0x004F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_UFS_COMPLETED
Language     = English
Internal information only. Delete recovery process %3 after recovery of File System %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort the File System recovery request received.
;//
;// Generated value should be: 0x616F0050
;//
MessageId    = 0x0050
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_UFS_RECEIVED
Language     = English
Request received to cancel recovery for File System %2.
Internal information only. Aborting File System recovery process object %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort the File System recovery completed.
;//
;// Generated value should be: 0x616F0051
;//
MessageId    = 0x0051
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_UFS_COMPLETED
Language     = English
Recovery of File System %2 was successfullly cancelled.
Internal information only. File System Recovery process object %3 was aborted.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616F0052
;//
MessageId    = 0x0052
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UNMOUNT_UFS_FOR_PANIC_AVOIDANCE_REUSE_ME
Language     = English
This code can be reused.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband CIFS Share create request completed
;//
;// Generated value should be: 0x616F0053
;//
MessageId    = 0x0053
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UAPI_CREATE_CIFS_SHARE_COMPLETED
Language     = English
An in-band request to create CIFS Share %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband CIFS Share destroy request completed
;//
;// Generated value should be: 0x616F0054
;//
MessageId    = 0x0054
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UAPI_DESTROY_CIFS_SHARE_COMPLETED
Language     = English
An in-band request to destroy CIFS Share %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband CIFS Share modify request completed
;//
;// Generated value should be: 0x616F0055
;//
MessageId    = 0x0055
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UAPI_MODIFY_CIFS_SHARE_COMPLETED
Language     = English
An in-band request to modify the properties of CIFS Share %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband enable CAVA request completed
;//
;// Generated value should be: 0x616F0056
;//
MessageId    = 0x0056
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UAPI_ENABLE_CAVA_COMPLETED
Language     = English
An in-band request to enable CAVA for NAS Server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband disable CAVA request completed
;//
;// Generated value should be: 0x616F0057
;//
MessageId    = 0x0057
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_UAPI_DISABLE_CAVA_COMPLETED
Language     = English
An in-band request to disable CAVA for NAS Server %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0058
;//
MessageId    = 0x0058
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_LDAP_RECEIVED
Language     = English
Request received to create LDAP named %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0059
;//
MessageId    = 0x0059
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_LDAP_COMPLETED
Language     = English
Request to create LDAP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F005A
;//
MessageId    = 0x005A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_LDAP_RECEIVED
Language     = English
Request received to destroy LDAP %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F005B
;//
MessageId    = 0x005B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_LDAP_COMPLETED
Language     = English
Request to destroy LDAP %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F05C
;//
MessageId    = 0x005C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_LDAP_RECEIVED
Language     = English
Request received to modify properties of LDAP %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F005D
;//
MessageId    = 0x005D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_LDAP_COMPLETED
Language     = English
Request to modify properties of LDAP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F005E
;//
MessageId    = 0x005E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_FTP_RECEIVED
Language     = English
Request received to create an FTP for NAS Server %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F005F
;//
MessageId    = 0x005F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_FTP_COMPLETED
Language     = English
Request to create FTP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0060
;//
MessageId    = 0x0060
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_FTP_RECEIVED
Language     = English
Request received to modify FTP %2 for NAS Server %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0061
;//
MessageId    = 0x0061
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_VDM_FTP_COMPLETED
Language     = English
Request to modify FTP %2 completed successfully.
.

;// ----------------------------------------------------------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0062
;//
MessageId    = 0x0062
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_FTP_RECEIVED
Language     = English
Request received to destroy FTP %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0063
;//
MessageId    = 0x0063
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_FTP_COMPLETED
Language     = English
Request to destroy FTP %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0064
;//
MessageId    = 0x0064
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_SFS_MOVE_START_RECEIVED
Language     = English
Request recieved to move NAS Server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0065
;//
MessageId    = 0x0065
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_SFS_MOVE_COMPLETE
Language     = English
Moving NAS Server %2 to peer completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0066
;//
MessageId    = 0x0066
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_SFS_MOVE_ONGOING
Language     = English
A move operation for NAS Server %2 is still outstanding.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort the File System (UFS64 Only) shrink request received.
;//
;// Generated value should be: 0x616F0067
;//
MessageId    = 0x0067
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_UFS64_SHRINK_RECEIVED
Language     = English
Request received to cancel shrink for File System %2.
Internal information only. Aborting File System shrink process object %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort the File System (UFS64 Only) shrink completed.
;//
;// Generated value should be: 0x616F0068
;//
MessageId    = 0x0068
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_UFS64_SHRINK_COMPLETED
Language     = English
Shrink of File System %2 was successfullly cancelled.
Internal information only. File System Shrink process object %3 was aborted.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Creation of VDM Migration initiated.
;//
;// Generated value should be: 0x616F0069
;//
MessageId    = 0x0069
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_MIGRATION_RECEIVED
Language     = English
Request received to create a VDM Migration named %2.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: VDM Migration creation initiated successfully.
;//
;// Generated value should be: 0x616F006A
;//
MessageId    = 0x006A
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_MIGRATION_COMPLETED
Language     = English
Request to create a VDM Migration %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destruction of VDM Migration initiated.
;//
;// Generated value should be: 0x616F006B
;//
MessageId    = 0x006B
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_MIGRATION_RECEIVED
Language     = English
Request received to destroy a VDM Migration named %2.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem:  VDM Migration destruction initiated successfully.
;//
;// Generated value should be: 0x616F006C
;//
MessageId    = 0x006C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_MIGRATION_COMPLETED
Language     = English
Destruction of VDM migration %2 has successfully initiated.
.

;// --------------------------------------------------
;// Introduced In: Bald Eagle (PSI7)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destruction of VDM Route initiated.
;//
;// Generated value should be: 0x616F006D
;//
MessageId    = 0x006D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NET_ROUTE_RECEIVED
Language     = English
Request received to destroy VDM Route %2.
.
    
;// --------------------------------------------------
;// Introduced In: Bald Eagle (PSI7)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem:  VDM Route destruction initiated successfully.
;//
;// Generated value should be: 0x616F006E
;//
MessageId    = 0x006E
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_VDM_NET_ROUTE_COMPLETED
Language     = English
Destruction of VDM route %2 has successfully initiated.
.

;// --------------------------------------------------
;// Introduced In: Bald Eagle (PSI7)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem: Creation of VDM Route initiated.
;//
;// Generated value should be: 0x616F006F
;//
MessageId    = 0x006F
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NET_ROUTE_RECEIVED
Language     = English
Request received to creatie a VDM Route.
.
    
;// --------------------------------------------------
;// Introduced In: Bald Eagle (PSI7)
;//
;// Usage: For Event Log Only.
;//
;// Severity: Informational
;//
;// Symptom of Problem:  VDM Route creation initiated successfully.
;//
;// Generated value should be: 0x616F0070
;//
MessageId    = 0x0070
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_VDM_NET_ROUTE_COMPLETED
Language     = English
Creation of VDM route %2 has successfully initiated.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0071
;//
MessageId    = 0x0071
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_KERBEROS_RECEIVED
Language     = English
Request received to create KERBEROS properties named %2.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0072
;//
MessageId    = 0x0072
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_KERBEROS_COMPLETED
Language     = English
Request to create KERBEROS %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0073
;//
MessageId    = 0x0073
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_KERBEROS_RECEIVED
Language     = English
Request received to destroy KERBEROS %2.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0074
;//
MessageId    = 0x0074
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_KERBEROS_COMPLETED
Language     = English
Request to destroy KERBEROS %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0075
;//
MessageId    = 0x0075
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_KERBEROS_RECEIVED
Language     = English
Request received to modify KERBEROS properties %2.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616F0076
;//
MessageId    = 0x0076
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_INFO_MODIFY_KERBEROS_COMPLETED
Language     = English
Request to modify KERBEROS properties %2 completed successfully.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: User selected to fail writes and not invalidate snaps on the File-System. Free space in the pool went below threshold
;//                     and file system was mounted read only. But now free space went above threshold and file system is mounted back as read write.
;//
;// Generated value should be: 0x616F0077
;//
MessageId    = 0x0077
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_ALERT_ABOUT_READ_WRITE_MOUNT_UFS64_SNAPINVALIDATION
Language     = English
File system %2 is now read-write, because free space in its pool has increased more than %3 GB.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: User selected to fail writes and not invalidate snaps on the File-System. Free space in the pool went below threshold
;//                     and file system was mounted read only. But now free space went above threshold and file system is mounted back as read write.
;//
;// Generated value should be: 0x616F0078
;//
MessageId    = 0x0078
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_ALERT_ABOUT_NOT_INVALIDATING_SNAPS_FOR_NOW
Language     = English
File system %2 is no longer at risk of losing its snapshots.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: User selected to fail writes and not invalidate snaps on the File-System. Free space in the pool went below threshold
;//                     and file system was mounted read only. But now free space went above threshold and file system is mounted back as read write.
;//
;// Generated value should be: 0x616F0079
;//
MessageId    = 0x0079
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_UDI_INFO_ALERT_ABOUT_FS_REPL_SESSION_NOT_NEEDING_FULL_SYNC
Language     = English
File system %2 no longer needs a full synchronization for the associated replication session.
.

;
;
;
;//===========================
;//======= WARNING MSGS ======
;//===========================
;
;
;


;// --------------------------------------------------
;// Introduced In: R__
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x616F4000
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_TEMPLATE
Language     = English
Only for template
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Warning
;//
;// Symptom of Problem: File System object needs recovery
;//
;// Generated value should be: 0x616F4001
;//
MessageId    = 0x4001
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_UFS_NEEDS_RECOVERY
Language     = English
The File System cannot be used until it is recovered. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Warning
;//
;// Symptom of Problem: File System is faulted because its VU is faulted.
;//
;// Generated value should be: 0x616F4002
;//
MessageId    = 0x4002
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_UFS_IS_FAULTED_DUE_TO_VU_IS_FAULTED
Language     = English
The File System is faulted because its volume is faulted. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Warning
;//
;// Symptom of Problem: NAS Server is faulted because its config File System is offline.
;//
;// Generated value should be: 0x616F4003
;//
MessageId    = 0x4003
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_VDM_WARNING_VDM_IS_FAULTED_DUE_TO_CONFIG_UFS_IS_OFFLINE
Language     = English
The NAS Server is faulted because its configuration File System is offline. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Warning
;//
;// Symptom of Problem: NAS Server is faulted because its root File System is faulted.
;//
;// Generated value should be: 0x616F4004
;//
MessageId    = 0x4004
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_VDM_WARNING_VDM_IS_FAULTED_DUE_TO_ROOT_UFS_IS_FAULTED
Language     = English
The NAS Server is faulted because its root File System is faulted. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: For Audit Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The VDM Override configuration is in use, and a network interface
;//                     has been created in the Global configuration without a corresponding
;//                     entry in the Override configuration.
;//
;// Generated value should be: 0x616F4005
;//
MessageId    = 0x4005
;//SSPG C4type=Audit
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_OVERRIDE_NETWORK_INTERFACE_MISSING
Language     = English
NAS Server %2 is missing a network interface to override %3.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                      
;//


;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the CIFS shares and NFS exports high watermark for the array
;//
;// Generated value should be: 0x616F4006
;//
MessageId    = 0x4006
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_ARRAY_HAS_REACHED_SHARES_COUNT_THRESHOLD
Language     = English
The array has reached the warning threshold for the number of Shares associated with it.
Internal Information Only: Creation of %2 leaves the array at the warning threshold of %3 Shares.
.

;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: A file system is approaching the maximum number of CIFS shares allowed.
;//
;// Generated value should be: 0x616F4007
;//
MessageId    = 0x4007
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_SF_HAS_REACHED_CIFS_SHARE_COUNT_THRESHOLD
Language     = English
The file system has reached the warning threshold for the number of CIFS Shares associated with it.
Internal Information Only: Creation of %2 leaves the file system at the warning threshold of %3 CIFS Shares.
.

;// Usage: Logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: A file system is approaching the maximum number of NFS export allowed
;//
;// Generated value should be: 0x616F4008
;//
MessageId    = 0x4008
Severity     = Warning
Facility     = MLUUDI
SymbolicName =  MLU_UDI_WARNING_SF_HAS_REACHED_NFS_EXPORT_COUNT_THRESHOLD
Language     = English
The file system has reached the warning threshold for the number of NFS Exports associated with it.
Internal Information Only: Creation of %2 leaves the file system at the warning threshold of %3 NFS Exports.
.

;// ----------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Audit and Event Log 
;//
;// Severity: Warning
;//
;// Symptom of Problem: User selected to invalidate snaps on the File-System and now the threshold
;//                     has breached to alert the user about possibly losing the associated snaps
;//
;// Generated value should be: 0xc16F4009
MessageId    = 0x4009
;//SSPG C4type=Audit
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_UFS64_ALERT_ABOUT_SNAP_INVALIDATION
Language     = English
Pool containing file system %2 is low on free space. The file system will lose all of its snapshots, unless more storage space is added to the pool or the file system's pool full policy is changed to failWrites.
.

;// ----------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Audit and Event Log 
;//
;// Severity: Warning
;//
;// Symptom of Problem: User selected to invalidate snaps on the File-System and now the threshold
;//                     has breached to alert the user about possibly needing a full-sync as system snaps
;//                     associated in a replication session might get invalidated
;//
;// Generated value should be: 0xc16F400A
MessageId    = 0x400A
;//SSPG C4type=Audit
Severity     = Warning
Facility     = MLUUDI
SymbolicName = MLU_UDI_WARNING_UFS64_ALERT_ABOUT_NEEDING_FULL_SYNC
Language     = English
Pool containing file system %2 is low on free space. The file system is in a replication session and will require a full synchronization, unless more space is added to the pool or the file system's pool full policy is changed to failWrites.
.

;
;
;
;//=========================
;//======= ERROR MSGS ======
;//=========================
;
;
;


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Servers
;//
;// Generated value should be: 0xE16F8000
;//
MessageId    = 0x8000
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server because the maximum number of NAS Servers already exists on the array
Internal Information Only.Could not create %2 because maximum number of NAS Servers %3 already exists on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of file systems
;//
;// Generated value should be: 0xE16F8001
;//
MessageId    = 0x8001
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_SF_LIMIT_EXCEEDED
Language     = English
Unable to create the File System because the maximum number of file systems and File System snaps already exist on the array.
Internal Information Only. Could not create %2 because the maximum number of file systems and File System snaps %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Duplicate name for NAS Server
;//
;// Generated value should be: 0xE16F8002
;//
MessageId    = 0x8002
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_NAME_ALREADY_EXISTS
Language     = English
Unable to create the NAS Server because the specified name is already in use. Please retry the operation using a unique name.
Internal Information Only. NAS Server %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Duplicate name for file system
;//
;// Generated value should be: 0xE16F8003
;//
MessageId    = 0x8003
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_NAME_ALREADY_EXISTS
Language     = English
Unable to create the File System because the specified name is already in use. Please retry the operation using a unique name.
Internal Information Only. File System %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Error returned when trying to create an NAS Server while Pool is offline
;//
;// Generated value should be: 0xE16F8004
;//
MessageId    = 0x8004
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CREATE_POOL_OFFLINE
Language     = English
Unable to create a NAS Server because the pool is offline. Please try again after bringing the pool back online.
Internal Information Only.Could not create NAS Server because pool %2 is offline.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Error returned when trying to create an NAS Server while Pool is Destroying
;//
;// Generated value should be: 0xE16F8005
;//
MessageId    = 0x8005
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CREATE_POOL_DESTROYING
Language     = English
Unable to create a NAS Server because the pool is being destroyed.
Internal Information Only.Could not create NAS Server because pool %2 is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Unused
;//
;// Severity: Error
;//
;// Symptom of Problem: Unmounting File System to avoid panicking since free pool space has dropped below threshold.
;//
;// Generated value should be: 0xE16F8006
;//
MessageId    = 0x8006
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UNMOUNT_UFS_FOR_PANIC_AVOIDANCE
Language     = English
Unmounting File System %2 because free pool space has dropped below threshold.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F8007
;//
MessageId    = 0x8007
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INSTANTIATE_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 instantiation failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F8008
;//
MessageId    = 0x8008
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_START_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 start failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F8009
;//
MessageId    = 0x8009
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_STOP_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 stop failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F800A
;//
MessageId    = 0x800A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_COMPLETE_INIT_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 init completion failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified File System object does not exist
;//
;// Generated value should be: 0xE16F800B
;//
MessageId    = 0x800B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_DOES_NOT_EXIST
Language     = English
Unable to destroy the File System because the specified File System does not exist. 
Internal Information Only. Could not destroy the File System because File System object %2 does not exist.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere.
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified File System object already exists in the NAS Server
;//
;// Generated value should be: 0xE16F800C
;//
MessageId    = 0x800C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_ALREADY_EXISTS_IN_VDM
Language     = English
Unable to add the File System to NAS Server because it is already added. 
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere.
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified File System object is not a member of this NAS Server
;//
;// Generated value should be: 0xE16F800D
;//
MessageId    = 0x800D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_NOT_A_MEMBER_OF_THIS_VDM
Language     = English
Unable to remove the File System from the NAS Server because the specified File System does not exist in the server. 
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified NAS Server object does not exist
;//
;// Generated value should be: 0xE16F800E
;//
MessageId    = 0x800E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_DOES_NOT_EXIST
Language     = English
Unable proceeed with the requested operation because the specified NAS Server does not exist. 
Internal Information Only. Could not %2 because NAS Server object %3 does not exist.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a non USER File System directly. If a ROOT/CONFIG File System needs to be destroyed
;//                     it can only be initiated through the destroy of a NAS Server.
;//
;// Generated value should be: 0xE16F800F
;//
MessageId    = 0x800F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_TYPE_DOES_NOT_SUPPORT_DESTROY
Language     = English
Unable to complete the destroy operation. The destroy operation on the specified File System is not supported.
Internal Information Only. Could not destroy the File System because the File System object %2 associated with it is not of type USER.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of file systems per NAS Server
;//
;// Generated value should be: 0xE16F8010
;//
MessageId    = 0x8010
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_MAX_SF_PER_SFS_LIMIT_EXCEEDED
Language     = English
Unable to create the file system/file system snap because the maximum number of file systems and File System snaps already exist on the NAS Server.
Internal Information Only. Could not create file system/file system snap because the maximum number of file systems and File System snaps %2 already exist on the NAS Server %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a system NAS Server. 
;//
;// Generated value should be: 0xE16F8011
;//
MessageId    = 0x8011
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SYSTEM_VDM_DESTROY_NOT_SUPPORTED
Language     = English
The destroy operation on the specified NAS Server not supported since this is not created by the user. 
Internal Information Only. Could not destroy the NAS Server %2 because it is a System NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F8012
;//
MessageId    = 0x8012
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_END_VDM_INSTANTIATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 end instantiation failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F8014
;//
MessageId    = 0x8014
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_SFS_PROPERTIES_SFS_DESTROYING
Language     = English
Unable to set NAS Server properties while the NAS Server is being destroyed.
Internal Information Only. Could not set NAS Server properties because the NAS Server %2 is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API CREATE- NAS Server -INTERFACE request failed.
;//
;// Generated value should be: 0xE16F8015
;//
MessageId    = 0x8015
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_VDM_INTERFACE_FAILED
Language     = English
NAS Server network interface %2 failed while being created.
Internal Information Only.  NAS Server NetIF %3 failed create interface request with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API DESTROY- NAS Server -INTERFACE request failed.
;//
;// Generated value should be: 0xE16F8016
;//
MessageId    = 0x8016
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_VDM_INTERFACE_FAILED
Language     = English
NAS Server network interface %2 failed while being destroyed.
Internal Information Only. NAS Server NetIF %3 failed destroy interface request with status %4
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Server network interfaces
;//
;// Generated value should be: 0xE16F8017
;//
MessageId    = 0x8017
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_NET_IF_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server network interface because the maximum number of NAS Server network interfaces already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of network interface per NAS Server
;//
;// Generated value should be: 0xE16F8018
;//
MessageId    = 0x8018
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_NET_IF_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server network interface because the maximum number of network interfaces already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of NAS Servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET- NAS Server -DNS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F8019
;//
MessageId    = 0x8019
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_VDM_DNS_PROPERTIES_FAILED
Language     = English
NAS Server DNS %2 failed while being created.
Internal Information Only. NAS Server DNS %3 failed setting dns property with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET- NAS Server -DNS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F801A
;//
MessageId    = 0x801A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLEAR_VDM_DNS_PROPERTIES_FAILED
Language     = English
NAS Server %2 failed while destroying DNS.
Internal Information Only. NAS Server DNS %3 failed clearing DNS property with status %4
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Server dns
;//
;// Generated value should be: 0xE16F801B
;//
MessageId    = 0x801B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_DNS_COUNT_EXCEEDED
Language     = English
Unable to create NAS Server dns because the maximum number of NAS Server dns already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of dns per NAS Server
;//
;// Generated value should be: 0xE16F801C
;//
MessageId    = 0x801C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_DNS_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server dns property because the maximum number of dns already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of dns %3 already exist on the array.
.



;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API ADD- NAS Server -CIFS-SERVER request failed.
;//
;// Generated value should be: 0xE16F801D
;//
MessageId    = 0x801D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_VDM_CIFS_FAILED 
Language     = English
NAS Server CIFS server failed while being created.
Internal Information Only. CIFS %2 failed add CIFS request with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API REMOVE-VDM-CIFS-SERVER request failed.
;//
;// Generated value should be: 0xE16F801E
;//
MessageId    = 0x801E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_VDM_CIFS_FAILED 
Language     = English
NAS Server CIFS server failed while being destroyed.
Internal Information Only. CIFS server %2 failed remove CIFS request with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Server CIFS servers
;//
;// Generated value should be: 0xE16F801F
;//
MessageId    = 0x801F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_CIFS_COUNT_EXCEEDED 
Language     = English
Unable to create the NAS Server CIFS server because the maximum number of NAS Server CIFS servers already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of CIFS servers NAS Server
;//
;// Generated value should be: 0xE16F8020
;//
MessageId    = 0x8020
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_CIFS_PER_SFS_COUNT_EXCEEDED 
Language     = English
Unable to create the NAS Server CIFS server because the maximum number of CIFS servers already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of NAS Servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS join error
;//
;// Generated value should be: 0xE16F8021
;//
MessageId    = 0x8021
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_JOIN_DOMAIN_FAILED 
Language     = English
NAS Server CIFS server failed to join CIFS domain.
Internal Information Only. CIFS %2 failed to join CIFS domain with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS unjoin error
;//
;// Generated value should be: 0xE16F8022
;//
MessageId    = 0x8022
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_UNJOIN_DOMAIN_FAILED 
Language     = English
NAS Server CIFS server failed to unjoin CIFS domain.
Internal Information Only. CIFS %2 failed to unjoin CIFS domain with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS add net IF error
;//
;// Generated value should be: 0xE16F8023
;//
MessageId    = 0x8023
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_ADD_IF_FAILED 
Language     = English
Unable to add network interface to NAS Server CIFS server because of error.
Internal Information Only. CIFS %2 failed to add network interface with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS remove net IF error
;//
;// Generated value should be: 0xE16F8024
;//
MessageId    = 0x8024
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_REMOVE_IF_FAILED
Language     = English
Unable to remove network interface from NAS Server CIFS server because of error.
Internal Information Only. CIFS %2 failed to remove network interface with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS set properties error
;//
;// Generated value should be: 0xE16F8025
;//
MessageId    = 0x8025
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_SET_PROPS_FAILED
Language     = English
Unable to modify properties of NAS Server CIFS server because of error.
Internal Information Only. CIFS %2 failed to set properties with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F8026
;//
MessageId    = 0x8026
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_GET_PROPS_FAILED
Language     = English
Unable to get properties of NAS Server CIFS server because of error.
Internal Information Only. CIFS %2 failed to get properties with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F8027
;//
MessageId    = 0x8027
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED1_FAILED 
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F8028
;//
MessageId    = 0x8028
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED2_FAILED
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F8029
;//
MessageId    = 0x8029
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED3_FAILED
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F802A
;//
MessageId    = 0x802A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED4_FAILED
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F802B
;//
MessageId    = 0x802B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED5_FAILED
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: CIFS get properties error
;//
;// Generated value should be: 0xE16F802C
;//
MessageId    = 0x802C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SERVER_RESERVED6_FAILED
Language     = English
NAS Server CIFS server RESERVED error message.
Internal Information Only. CIFS %2 RESERVED error message with status %3
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-nfs-properties request failed for NFS enable.
;//
;// Generated value should be: 0xE16F802D
;//
MessageId    = 0x802D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ENABLE_NFS_FAILED
Language     = English
Failed to enable NFS for NAS Server %2.
Internal Information Only. NFS %3 failed to enable NFS with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-nfs-properties request failed for NFS disable.
;//
;// Generated value should be: 0xE16F802E
;//
MessageId    = 0x802E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DISABLE_NFS_FAILED
Language     = English
Failed to disable NFS for NAS Server %2.
Internal Information Only. NFS %3 failed to disable NFS with status %4

.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NFS servers
;//
;// Generated value should be: 0xE16F802F
;//
MessageId    = 0x802F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_NFS_COUNT_EXCEEDED
Language     = English
Cannot enable NFS because the maximum number of NFS servers already exist on the array.
Internal Information Only. Could not enable NFS for NAS Server %2 because maximum number of NFS servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NFS per NAS Server
;//
;// Generated value should be: 0xE16F8030
;//
MessageId    = 0x8030
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_NFS_PER_SFS_COUNT_EXCEEDED
Language     = English
NFS is already enabled for this NAS Server. 
Internal Information Only. Could not enable NFS for NAS Server %2 because maximum number of NFS servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System size is smaller than the minimum required.
;//
;// Generated value should be: 0xE16F8031
;//
MessageId    = 0x8031
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_SF_MINIMUM_SIZE_FAILED
Language     = English
Unable to create the File System because the File System minimum size requirement is not met.
Internal Information Only. Could not create %2 because the File System minimum size %3 is not met.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NFS exports on the array
;//
;// Generated value should be: 0xE16F8032
;//
MessageId    = 0x8032
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SF_NFS_EXPORT_COUNT_EXCEEDED
Language     = English
Unable to create the NFS export because the maximum number of CIFS Shares and NFS exports already exist on the array
Internal Information Only. Could not create %2 because maximum number of CIFS Shares and NFS Exports %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NFS exports per file system
;//
;// Generated value should be: 0xE16F8033
;//
MessageId    = 0x8033
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_NFS_EXPORT_PER_SF_COUNT_EXCEEDED
Language     = English
Unable to create the NFS export because the maximum number of NFS exports already exists for the specified file system.
Internal Information Only. Could not create %2 because maximum number of NFS Exports %3 already exists for the specified file system.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Server CIFS shares
;//
;// Generated value should be: 0xE16F8034
;//
MessageId    = 0x8034
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SF_CIFS_SHARE_COUNT_EXCEEDED
Language     = English
Unable to create the CIFS share because the maximum number of CIFS shares and NFS Exports already exist on the array
Internal Information Only. Could not create %2 because maximum number of CIFS Shares and NFS Exports %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of CIFS shares per file system
;//
;// Generated value should be: 0xE16F8035
;//
MessageId    = 0x8035
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_CIFS_SHARE_PER_SF_COUNT_EXCEEDED
Language     = English
Unable to create the CIFS share because the maximum number of CIFS shares already exists for the specified file system.
Internal Information Only. Could not create %2 because maximum number of CIFS Share %3 already exists for the specified file system.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API CREATE-VDM-NFS-EXPORT request failed.
;//
;// Generated value should be: 0xE16F8036
;//
MessageId    = 0x8036
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_VDM_NFS_EXPORT_FAILED
Language     = English
NAS Server NFS Export %2 failed while being created.
Internal Information Only. NAS Server NFSExport %3 failed create NFS Export request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API DESTROY-VDM-NFS-EXPORT request failed.
;//
;// Generated value should be: 0xE16F8037
;//
MessageId    = 0x8037
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_VDM_NFS_EXPORT_FAILED
Language     = English
NAS Server NFS Export %2 failed while being destroyed.
Internal Information Only. NAS Server NFSExport %3 failed destroy NFS Export request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API CREATE-VDM-CIFS-SHARE request failed.
;//
;// Generated value should be: 0xE16F8038
;//
MessageId    = 0x8038
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_VDM_CIFS_SHARE_FAILED
Language     = English
NAS Server CIFS Share %2 failed while being created.
Internal Information Only. NAS Server CIFSShare %3 failed create CIFS Share request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API DESTROY-VDM-CIFS-SHARE request failed.
;//
;// Generated value should be: 0xE16F8039
;//
MessageId    = 0x8039
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_VDM_CIFS_SHARE_FAILED
Language     = English
NAS Server CIFS Share %2 failed while being destroyed.
Internal Information Only. NAS Server CIFSShare %3 failed destroy CIFS Share request with status %4.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-ndmp-properties request failed for NDMP enable.
;//
;// Generated value should be: 0xE16F803A
;//
MessageId    = 0x803A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ENABLE_NDMP_FAILED
Language     = English
Failed to enable NDMP for NAS Server %2.
Internal Information Only. NDMP %3 failed to enable NDMP with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-ndmp-properties request failed for NDMP disable.
;//
;// Generated value should be: 0xE16F803B
;//
MessageId    = 0x803B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DISABLE_NDMP_FAILED
Language     = English
Failed to disable NDMP for NAS Server %2.
Internal Information Only. NDMP %3 failed to disable NDMP with status %4.

.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NDMP servers on the array
;//
;// Generated value should be: 0xE16F803C
;//
MessageId    = 0x803C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_NDMP_COUNT_EXCEEDED
Language     = English
Cannot enable NDMP because the maximum number of NDMP servers already exist on the array.
Internal Information Only. Could not enable NDMP for NAS Server %2 because maximum number of NDMP servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NDMP per NAS Server
;//
;// Generated value should be: 0xE16F803D
;//
MessageId    = 0x803D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_NDMP_PER_SFS_COUNT_EXCEEDED
Language     = English
NDMP is already enabled for this NAS Server. 
Internal Information Only. Could not enable NDMP for NAS Server %2 because maximum number of NDMP servers %3 already exist on the NAS Server.
.



;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API request to create File System failed
;//
;// Generated value should be: 0xE16F803E
;//
MessageId    = 0x803E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_CREATE_UFS_FAILED
Language     = English
File System %2 failed while being created.
Internal Information Only. File System %3 failed creation with status %4
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API unmount_ufs returned an error status 
;//
;// Generated value should be: 0xE16F803F
;//
MessageId    = 0x803F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_UFS_UNMOUNT_FAILED
Language     = English
File System %2 failed while unmounting.
Internal Information Only. File System %3 failed unmounting with status %4
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API mkdir returned an error status  
;//
;// Generated value should be: 0xE16F8040
;//
MessageId    = 0x8040
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_MOUNT_POINT_CREATE_FAILED
Language     = English
File System %2 failed while creating mount point.
Internal Information Only. File System %3 failed creation mount point with status %4
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API mount_ufs returned an error status 
;//
;// Generated value should be: 0xE16F8041
;//
MessageId    = 0x8041
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_MOUNT_UFS_FAILED
Language     = English
File System %2 failed while mounting.
Internal Information Only. File System %3 failed mounting with status %4
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API rmdir returned an error status 
;//
;// Generated value should be: 0xE16F8042
;//
MessageId    = 0x8042
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_REMOVING_MOUNT_POINT_FAILED
Language     = English
File System %2 failed while removing mount point.
Internal Information Only. File System %3 failed removing mount point with status %4
.



;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API 
;//
;// Generated value should be: 0xE16F8043
;//
MessageId    = 0x8043
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_CANCEL_CREATE_UFS32_FAILED
Language     = English
File System %2 failed while cancelling creation.
Internal Information Only. File System %3 failed cancelling creation process with status %4
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits APO-driven operations.
;//
;// Generated value should be: 0xE16F8044
;//
MessageId    = 0x8044
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_APO_COUNT_EXCEEDED
Language     = English
Unable to perform the operation because the maximum number of ongoing operations has been reached.
Internal Information Only. Could not create APO because maximum number of APOs %2 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NetIF Modify operation.
;//
;// Generated value should be: 0xE16F8045
;//
MessageId    = 0x8045
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_INTERFACE_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server interface %2 failed.  Insufficient system resources were available.
Internal Information Only. NetIF %3: Buffer needed %4 bytes, got %5.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-INTERFACE-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8046
;//
MessageId    = 0x8046
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_INTERFACE_MODIFY_FAILED
Language     = English
Modify operation for NAS Server interface %2 failed.
Internal Information Only. NAS Server NetIF %3 failed Modify request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The target of an APO driven operation is being destroyed.
;//
;// Generated value should be: 0xE16F8047
;//
MessageId    = 0x8047
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_APO_OPERATION_FAILED_REQUISITE_BEING_DESTROYED
Language     = English
Unable to perform the specified operation because the target is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for CIFS Modify operation.
;//
;// Generated value should be: 0xE16F8048
;//
MessageId    = 0x8048
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server CIFS failed.  Insufficient system resources were available.
Internal Information Only. CIFS %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-CIFS-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8049
;//
MessageId    = 0x8049
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_MODIFY_FAILED
Language     = English
Modify operation for NAS Server CIFS failed.
Internal Information Only. NAS Server CIFS %2 failed Modify request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of CAVA per NAS Server
;//
;// Generated value should be: 0xE16F804A
;//
MessageId    = 0x804A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_CAVA_PER_SFS_COUNT_EXCEEDED
Language     = English
CAVA is already enabled for this NAS Server. 
Internal Information Only. Could not enable CAVA for NAS Server %2 because maximum number of CAVA servers %3 already exist on the NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of NAS Server CAVA servers
;//
;// Generated value should be: 0xE16F804B
;//
MessageId    = 0x804B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_CAVA_COUNT_EXCEEDED
Language     = English
Cannot enable CAVA because the maximum number of CAVA servers already exist on the array.
Internal Information Only. Could not enable CAVA for NAS Server %2 because maximum number of CAVA servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-cava-properties request failed for CAVA enable.
;//
;// Generated value should be: 0xE16F804C
;//
MessageId    = 0x804C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ENABLE_CAVA_FAILED
Language     = English
Failed to enable CAVA for NAS Server %2.
Internal Information Only. CAVA %3 failed to enable CAVA with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-cava-properties request failed for CAVA disable.
;//
;// Generated value should be: 0xE16F804D
;//
MessageId    = 0x804D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DISABLE_CAVA_FAILED
Language     = English
Failed to disable CAVA for NAS Server %2.
Internal Information Only. CAVA %3 failed to disable CAVA with status %4
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NFS Export Modify operation.
;//
;// Generated value should be: 0xE16F804E
;//
MessageId    = 0x804E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NFS_EXPORT_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server NFS Export %2 failed.  Insufficient system resources were available.
Internal Information Only. NAS Server NFSExport %3: Buffer needed %4 bytes, got %5.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NFS-EXPORT-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F804F
;//
MessageId    = 0x804F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NFS_EXPORT_MODIFY_FAILED
Language     = English
Modify operation for NAS Server NFS Export %2 failed.
Internal Information Only. NAS Server NFSExport %3 failed Modify request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for CIFS Share Modify operation.
;//
;// Generated value should be: 0xE16F8050
;//
MessageId    = 0x8050
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SHARE_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server CIFS Share %2 failed.  Insufficient system resources were available.
Internal Information Only. NAS Server CIFSShare %3: Buffer needed %4 bytes, got %5.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-CIFS-SHARE-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8051
;//
MessageId    = 0x8051
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_CIFS_SHARE_MODIFY_FAILED
Language     = English
Modify operation for NAS Server CIFS Share %2 failed.
Internal Information Only. NAS Server CIFSShare %3 failed Modify request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified NAS Server object does not have a configured CIFS server
;//
;// Generated value should be: 0xE16F8052
;//
MessageId    = 0x8052
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFS_SERVER_DOES_NOT_EXIST
Language     = English
Unable proceeed with the requested operation because the specified NAS Server does not have a configured CIFS server. 
Internal Information Only. Could not perform the operation because the CIFS server does not exist on NAS Server %2.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NDMP Modify operation.
;//
;// Generated value should be: 0xE16F8053
;//
MessageId    = 0x8053
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NDMP_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server NDMP failed.  Insufficient system resources were available.
Internal Information Only. NDMP %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NDMP-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8054
;//
MessageId    = 0x8054
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NDMP_MODIFY_FAILED
Language     = English
Modify operation for NDMP interface failed.
Internal Information Only. NDMP %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-asa-properties request failed for ASA enable.
;//
;// Generated value should be: 0xE16F8055
;//
MessageId    = 0x8055
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ENABLE_ASA_FAILED
Language     = English
Failed to enable ASA for NAS Server %2.
Internal Information Only. ASA %3 failed to enable ASA with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-asa-properties request failed for ASA disable.
;//
;// Generated value should be: 0xE16F8056
;//
MessageId    = 0x8056
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DISABLE_ASA_FAILED
Language     = English
Failed to disable ASA for NAS Server %2.
Internal Information Only. ASA %3 failed to disable ASA with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of ASA servers on the array
;//
;// Generated value should be: 0xE16F8057
;//
MessageId    = 0x8057
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_ASA_COUNT_EXCEEDED
Language     = English
Cannot enable ASA because the maximum number of ASA servers already exist on the array.
Internal Information Only. Could not enable ASA for NAS Server %2 because maximum number of ASA servers %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of ASA per NAS Server
;//
;// Generated value should be: 0xE16F8058
;//
MessageId    = 0x8058
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_ASA_PER_SFS_COUNT_EXCEEDED
Language     = English
ASA is already enabled for this NAS Server. 
Internal Information Only. Could not enable ASA for NAS Server %2 because maximum number of ASA servers %3 already exist on the share folder server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NAS Server Modify operation.
;//
;// Generated value should be: 0xE16F8059
;//
MessageId    = 0x8059
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server failed.  Insufficient system resources were available.
Internal Information Only. NAS Server %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F805A
;//
MessageId    = 0x805A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_MODIFY_FAILED
Language     = English
Modify operation for NAS Server failed.
Internal Information Only. NAS Server %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for DNS Modify operation.
;//
;// Generated value should be: 0xE16F805B
;//
MessageId    = 0x805B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_DNS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server DNS failed.  Insufficient system resources were available.
Internal Information Only. NAS Server DNS %2, %3: Buffer needed %4 bytes, got %5.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-DNS-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F805C
;//
MessageId    = 0x805C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_DNS_MODIFY_FAILED
Language     = English
Modify operation for NAS Server DNS failed.
Internal Information Only. NAS Server DNS %2, %3 failed Modify request with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for ASA Modify operation.
;//
;// Generated value should be: 0xE16F805D
;//
MessageId    = 0x805D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_ASA_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server ASA failed.  Insufficient system resources were available.
Internal Information Only. ASA %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-ASA-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F805E
;//
MessageId    = 0x805E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_ASA_MODIFY_FAILED
Language     = English
Modify operation for ASA failed.
Internal Information Only. ASA %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for File System Modify operation.
;//
;// Generated value should be: 0xE16F805F
;//
MessageId    = 0x805F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_MODIFY_NO_MEMORY
Language     = English
Modify operation for File System failed.  Insufficient system resources were available.
Internal Information Only. File System %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-FILE-SYSTEM-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8060
;//
MessageId    = 0x8060
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_MODIFY_FAILED
Language     = English
Modify operation for File System failed.
Internal Information Only. File System %2 failed Modify request with status %3.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NIS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F8061
;//
MessageId    = 0x8061
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_VDM_NIS_PROPERTIES_FAILED
Language     = English
NAS Server NIS failed while attempting to set NIS object properties.
Internal Information Only. NAS Server DNS %2 failed setting NIS property with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NIS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F8062
;//
MessageId    = 0x8062
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLEAR_VDM_NIS_PROPERTIES_FAILED
Language     = English
NAS Server failed while attempting to clear NIS object properties.
Internal Information Only. NAS Server NIS %2 failed clearing NIS property with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceeded the maximum allowed limits of NAS Server NIS
;//
;// Generated value should be: 0xE16F8063
;//
MessageId    = 0x8063
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_NIS_COUNT_EXCEEDED
Language     = English
Unable to create NAS Server NIS because the maximum number of NAS Server NIS objects already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers NIS objects (%3) already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceed the maximum allowed limits of NIS per NAS Server
;//
;// Generated value should be: 0xE16F8064
;//
MessageId    = 0x8064
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_NIS_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server NIS property because the maximum number of NIS already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of NIS %3 already exist for the specified NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System prepare snap phase 2 event returned an error status  
;//
;// Generated value should be: 0xE16F8065
;//
MessageId    = 0x8065
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_PREPARE_SNAP_PHASE_2_FAILED
Language     = English
Snapshot File System %2 failed while preparing for snap phase 2 creation.
Internal Information Only. Snapshot File System %3 failed preparing snap phase 2 with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API resume File System returned an error status  
;//
;// Generated value should be: 0xE16F8066
;//
MessageId    = 0x8066
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RESUME_FILE_SYSTEM_FAILED
Language     = English
Snapshot File System %2 failed while resuming source file system.
Internal Information Only. Snapshot File System %3 failed resuming source File System with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System delegate snap creation event returned an error status  
;//
;// Generated value should be: 0xE16F8067
;//
MessageId    = 0x8067
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_DELEGATE_SNAP_CREATION_FAILED
Language     = English
Snapshot File System %2 failed while delegating snap creation.
Internal Information Only. Snapshot File System %3 failed to delegate snap creation with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API pause File System returned an error status  
;//
;// Generated value should be: 0xE16F8068
;//
MessageId    = 0x8068
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_PAUSE_FILE_SYSTEM_FAILED
Language     = English
Snapshot File System %2 failed while pausing source file system.
Internal Information Only. File System %3 failed pausing source File System with status %4
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a non USER File System snap directly. A ROOT/CONFIG File System cannot be snapped.
;//                     
;//
;// Generated value should be: 0xE16F8069
;//
MessageId    = 0x8069
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_TYPE_DOES_NOT_SUPPORT_SNAP
Language     = English
Unable to complete the create snap operation. The snap create operation on the specified File System is not supported.
Internal Information Only. Could not create snap on the File System because the File System object %2 associated with it is not of type USER.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NIS Modify operation.
;//
;// Generated value should be: 0xE16F806A
;//
MessageId    = 0x806A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NIS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server NIS failed.  Insufficient system resources were available.
Internal Information Only. NIS %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NIS-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F806B
;//
MessageId    = 0x806B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NIS_MODIFY_FAILED
Language     = English
Modify operation for NIS interface failed.
Internal Information Only. NIS %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API extend_ufs returned an error status
;//
;// Generated value should be: 0xE16F806C
;//
MessageId    = 0x806C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_EXTEND_UFS_FAILED
Language     = English
File System %2 failed while extending.
Internal Information Only. File System %3 failed extending with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Multiple groups of properties specified for modify File System.
;//
;// Generated value should be: 0xE16F806D
;//
MessageId    = 0x806D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INCORRECT_PROPERTY_MIX_SPECIFIED
Language     = English
An attempt to modify the properties of the File System failed because it included incompatible properties.
Internal Information Only. File System %2 received a modification request that included both deduplication properties and other File System properties.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log.
;//
;// Severity: Error
;//
;// Symptom of Problem: File System encountered IO errors and notified pending panic.
;//
;// Generated value should be: 0xE16F806E
;//
MessageId    = 0x806E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_PANIC_NOTIFICATION
Language     = English
File System encountered IO errors and would cause a system panic.
Internal Information Only. File System %2 has encountered IO errors and notified pending panic %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to create object while the NAS Server is being destroyed.
;//
;// Generated value should be: 0xE16F806F
;//
MessageId    = 0x806F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_DESTROYING
Language     = English
Unable to create object while the NAS Server is being destroyed.
Internal Information Only. Could not create object because the NAS Server is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to create object while the NAS Server is offline.
;//
;// Generated value should be: 0xE16F8070
;//
MessageId    = 0x8070
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_OFFLINE
Language     = English
Unable to create object while the NAS Server is offline.
Internal Information Only. Could not create object because the NAS Server is offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to notify the File System thin flag when doing a thin to thick conversion. Refer to the event log for detailed error info.
;//
;// Generated value should be: 0xE16F8071
;//
MessageId    = 0x8071
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_THIN_TO_THICK_CONVERSION_FAILED
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Attempt to modify File System thin flag failed with error %2.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to modify snap properties for a non-snap File System object.
;//
;// Generated value should be: 0xE16F8072
;//
MessageId    = 0x8072
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SNAP_PROPERTIES_SET_ON_NON_SNAP_UFS
Language     = English
Unable to complete the modify snap properties operation. The modify snap properties operation on the specified File System is not supported because it is not a snap.
Internal Information Only. Could not modify snap properties because the File System object %2 associated with it is not a snap.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File System recovery failed because recovery encountered an error waiting for File System to disable.
;//
;// Generated value should be: 0xE16F8073
;//
MessageId    = 0x8073
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_PREPARE_UFS_WAIT_FOR_DISABLE_UFS
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Recovery failed for File System %2 because recovery encountered error %3 while waiting for File System to be disabled.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File System recovery failed due to unexpected object state while disabling file system.
;//
;// Generated value should be: 0xE16F8074
;//
MessageId    = 0x8074
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_PREPARE_UFS_UNEXPECTED_STATE
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Recovery failed for File System %2 because of unexpected state %3 while disabling file system.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File System recovery failed due to unexpected object state.
;//
;// Generated value should be: 0xE16F8075
;//
MessageId    = 0x8075
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_RECOVER_UFS_UNEXPECTED_STATE
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Recovery failed for File System %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File System recovery failed due to error accessing File System object.
;//
;// Generated value should be: 0xE16F8076
;//
MessageId    = 0x8076
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_PREPARE_UFS_ACCESS_OBJECT
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Recovery failed for File System %2 because of error %3 accessing the File System object.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File System recovery failed.
;//
;// Generated value should be: 0xE16F8077
;//
MessageId    = 0x8077
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_RECOVER_UFS_ERROR
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Recovery failed for File System %2 with error %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Delete File System recovery PO failed.
;//
;// Generated value should be: 0xE16F8078
;//
MessageId    = 0x8078
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_RM_ERROR_DELETE_RECOVER_UFS_FAILED
Language     = English
Internal information only. Deletion of File System %2's recovery Object %3 failed with error %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere only
;//
;// Severity: Error
;//
;// Symptom of Problem: File System Requires Recovery Now
;//
;// Generated value should be: 0xE16F8079
;//
MessageId    = 0x8079
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFSMGR_ERROR_UFS_REQUIRES_RECOVERY_NOW
Language     = English
File System requires recovery. Please contact your service provider.
Internal information only. File System %2 requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F807A
;//
MessageId    = 0x807A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REG_IND_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 Indication Handler Registration failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F807B
;//
MessageId    = 0x807B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_PAUSE_VDM_FAILED_ASYNC
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 Failed to pause NAS Server with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to set properties on a system NAS Server.
;//
;// Generated value should be: 0xE16F807C
;//
MessageId    = 0x807C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SYSTEM_VDM_SET_PROPERTIES_NOT_SUPPORTED
Language     = English
The set properties operation on the specified NAS Server not supported since this is not created by the user. 
Internal Information Only. Could not set properties on NAS Server %2 because it is a System NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create ASA on a system NAS Server.
;//
;// Generated value should be: 0xE16F807D
;//
MessageId    = 0x807D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ASA_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create ASA on system NAS Server.
Internal Information Only. Could not create ASA on NAS Server %2 because it is a System NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create NDMP on a system NAS Server.
;//
;// Generated value should be: 0xE16F807E
;//
MessageId    = 0x807E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NDMP_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create NDMP on system NAS Server.
Internal Information Only. Could not create NDMP on NAS Server %2 because it is a System NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create CAVA on a system  NAS Server .
;//
;// Generated value should be: 0xE16F807F
;//
MessageId    = 0x807F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CAVA_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create CAVA on system  NAS Server .
Internal Information Only. Could not create CAVA on  NAS Server  %2 because it is a System  NAS Server .
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create DNS on a system  NAS Server .
;//
;// Generated value should be: 0xE16F8080
;//
MessageId    = 0x8080
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DNS_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create DNS on system  NAS Server .
Internal Information Only. Could not create DNS on  NAS Server  %2 because it is a System  NAS Server .
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create NFS on a system  NAS Server .
;//
;// Generated value should be: 0xE16F8081
;//
MessageId    = 0x8081
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NFS_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create NFS on system  NAS Server .
Internal Information Only. Could not create NFS on  NAS Server  %2 because it is a System  NAS Server .
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create CIFS on a system  NAS Server .
;//
;// Generated value should be: 0xE16F8082
;//
MessageId    = 0x8082
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFS_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create CIFS on system  NAS Server .
Internal Information Only. Could not create CIFS on  NAS Server  %2 because it is a System  NAS Server .
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The network route could not be added because the same route already exists.
;//
;// Generated value should be: 0xE16F8083
;//
MessageId    = 0x8083
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NET_ROUTE_CREATE_WITH_DUPLICATE_ROUTE
Language     = English
The network route could not be added because the same route already exists.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem:  NAS Server network route failed while being created.
;//
;// Generated value should be: 0xE16F8084
;//
MessageId    = 0x8084
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_VDM_ROUTE_FAILED
Language     = English
NAS Server network route failed while being created.
Internal Information Only.  NAS Server NetRoute %2 failed create route request with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: NAS Server network route failed while being destroyed.
;//
;// Generated value should be: 0xE16F8085
;//
MessageId    = 0x8085
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_VDM_ROUTE_FAILED
Language     = English
NAS Server network route failed while being destroyed.
Internal Information Only. NAS Server NetRoute %2 failed destroy interface request with status %3
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create NFS export on a File System in a system NAS Server.
;//
;// Generated value should be: 0xE16F8086
;//
MessageId    = 0x8086
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NFSEXPORT_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create NFS export on File System of system NAS Server.
Internal Information Only. Cannot create NFS export on File System %2 because it is on System NAS Server %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create File System on a File System of system NAS Server.
;//
;// Generated value should be: 0xE16F8087
;//
MessageId    = 0x8087
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFSSHARE_CREATE_ON_SYSTEM_VDM_NOT_SUPPORTED
Language     = English
Internal Error. Cannot create File System on File System of a system NAS Server.
Internal Information Only. Could not create File System on File System %2 because it is on System NAS Server %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create File System on a File System of system  NAS Server .
;//
;// Generated value should be: 0xE16F8088
;//
MessageId    = 0x8088
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVERY_VDM_DESTROYING
Language     = English
Internal Error. Cannot run recovery on File System of a destroying NAS Server. 
Internal Information Only. Could not run recovery on File System %2 because its NAS Server %3 is destroying.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to shrink File System smaller than the minimum size.
;//
;// Generated value should be: 0xE16F8089
;//
MessageId    = 0x8089
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_EXECUTIVE_ERROR_SHRINK_UFS_FAILED_INVALID_SIZE
Language     = English
Unable to shrink the File System because the specified size is smaller than the minimum.
Internal Information Only. Could not shrink the File System because the modify size of File System object %2 is smaller that the minimum size.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to modify the thin flag which is already thin or thick.
;//
;// Generated value should be: 0xE16F808A
;//
MessageId    = 0x808A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_THIN_FLAG_MODIFY_FAILED
Language     = English
Unable to modify the thin flag because it is already a thin or thick lun.
Internal Information Only. Could not modify the thin flag because it is already a thin or thick lun.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Duplicate name for CIFS share
;//
;// Generated value should be: 0xE16F808B
;//
MessageId    = 0x808B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFS_SHARE_NAME_ALREADY_EXISTS
Language     = English
Unable to create the CIFS share because the specified name is already in use. Please retry the operation using a unique name.
Internal Information Only. CIFS share %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The CIFS sever name already exits
;//
;// Generated value should be: 0xE16F808C
;//
MessageId    = 0x808C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFS_SERVER_NAME_ALREADY_EXISTS
Language     = English
Unable proceeed with the requested operation because the specified CIFS server name already exists. 
Internal Information Only. Could not perform the operation because the CIFS server name already exists on NAS Server %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to notify the File System thin flag when doing a thin to thick conversion. Refer to the event log for detailed error info.
;//
;// Generated value should be: 0xE16F808D
;//
MessageId    = 0x808D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_THIN_FS_INVALID_PARAMETERS
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. Attempt to auto increase thin File System size failed due to invalid parameters, error %2.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create network interface with duplicate IP address
;//
;// Generated value should be: 0xE16F808E
;//
MessageId    = 0x808E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NETIF_CREATE_WITH_DUPLICATE_IP_ADDRESS
Language     = English
The network interface could not be added because the IP address is already used by an existing network interface.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System when the File System is being destroyed.
;//
;// Generated value should be: 0xE16F808F
;//
MessageId    = 0x808F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVER_FAILED_WHEN_UFS_DESTROYING
Language     = English
The File System could not be recovered because it is being destroyed.
Internal Information Only. Could not ack recovering the File System when File System is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to abort recovering the File System when the File System is not offline.
;//
;// Generated value should be: 0xE16F8090
;//
MessageId    = 0x8090
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_ABORT_RECOVER_FAILED_UFS_NOT_OFFLINE
Language     = English
The File System's recovery could not be aborted because the File System is not in offline state.
Internal Information Only. Could not abort recovery of File System because the File System is not offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System when the File System is already recovering and is offline.
;//
;// Generated value should be: 0xE16F8091
;//
MessageId    = 0x8091
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVER_FAILED_WHEN_UFS_IS_BEING_RECOVERED
Language     = English
The File System could not be recovered because it is already recovering and is offline.
Internal Information Only. Could not recover the File System when its state is being recovered and is offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System which is being resized.
;//
;// Generated value should be: 0xE16F8092
;//
MessageId    = 0x8092
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_RESIZING_UFS_RECOVER_FAILED
Language     = English
The File System could not be recovered because it is being resized.
Internal Information Only. Could not recover the File System %2 because it is being resized.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System which is preparing or optimizing.
;//
;// Generated value should be: 0xE16F8093
;//
MessageId    = 0x8093
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_PREPARING_OR_OPTIMIZING_UFS_RECOVER_FAILED
Language     = English
The File System could not be recovered because it is preparing or optimizing.
Internal Information Only. Could not recover the File System because it is preparing or optimizing.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to resize the File System which is preparing or optimizing.
;//
;// Generated value should be: 0xE16F8094
;//
MessageId    = 0x8094
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_PREPARING_OR_OPTIMIZING_UFS_RESIZE_FAILED
Language     = English
The File System could not be resized because it is preparing or optimizing.
Internal Information Only. Could not resize the File System because it is preparing or optimizing.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System which is initializing.
;//
;// Generated value should be: 0xE16F8095
;//
MessageId    = 0x8095
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVER_FAILED_WHEN_UFS_INITIALIZING
Language     = English
The File System could not be recovered because it is still initializing.
Internal Information Only. Could not recover the File System when the File System state is initializing.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to abort recovering the File System when the File System is offline.
;//
;// Generated value should be: 0xE16F8096
;//
MessageId    = 0x8096
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_ABORT_RECOVER_FAILED
Language     = English
Recovery of the File System could not be aborted because the File System is offline.
Internal Information Only. Could not abort recovering the File System because it is offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to ack recovering the File System when the File System is offline.
;//
;// Generated value should be: 0xE16F8097
;//
MessageId    = 0x8097
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_ACK_RECOVER_FAILED
Language     = English
The file system's recovery could not be acknowledged because the File System is offline.
Internal Information Only. Could not ack recovering the File System because it is offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a File System with snaps, without specifying the PowerDelete flag.
;//
;// Generated value should be: 0xE16F8098
;//
MessageId    = 0x8098
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_SNAPSHOTS_EXIST
Language     = English
The File System cannot be destroyed because it still has snapshots. 
Destroy the snapshots then retry the File System destroy command.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Logged in Event Log and returned to Unisphere.
;//
;// Severity: Error
;//
;// Symptom of Problem: FS unmounted because it failed to go read-only or read-write after the threshold for snap invalidation was reached.
;//
;// Generated value should be: 0xE16F8099
;//
MessageId    = 0x8099
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_MARK_FOR_INVALIDATION_FAILED
Language     = English
The File System is unmounted because it failed to go read-only or read-write after the threshold for snap invalidation was reached. 
Internal Information Only. File System %3 failed to go read-only or read-write while snapshots were being invalidated in order to free space in the pool. CBFS status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The CIFS server could not be destroyed because it is joined to a domain.
;//
;// Generated value should be: 0xE16F809A
;//
MessageId    = 0x809A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTROY_CIFS_WITH_DOMAIN_JOINED
Language     = English
The CIFS server could not be destroyed because it is joined to a domain. Remove it from the domain and retry the operation. 
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System size is greater than the maximum allowed.
;//
;// Generated value should be: 0xE16F809B
;//
MessageId    = 0x809B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_SF_MAXIMUM_SIZE_FAILED
Language     = English
Unable to create the File System because the File System size is greater than the maximum size allowed.
Internal Information Only. Could not create %2 because the File System size %3 is greater than the maximum size allowed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An error returned from mount-fs-family-complete API call
;//
;// Generated value should be: 0xE16F809C
;//
MessageId    = 0x809C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MOUNT_FS_FAMILY_COMPLETE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Mount FS Family Complete for File System family %2 failed with status %3.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot could not be created because another snapshot creation is still in progress.
;//
;// Generated value should be: 0xE16F809D
;//
MessageId    = 0x809D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VUMGMT_ERROR_SNAPSHOT_CREATION_PROHIBITED_UNTIL_PREVIOUS_SNAPSHOT_CREATION_COMPLETES
Language     = English
The snapshot could not be created because another snapshot creation is still in progress. Wait for the first creation to complete and then retry the operation.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN could not be destroyed because a snapshot of it is being created.
;//
;// Generated value should be: 0xE16F809E
;//
MessageId    = 0x809E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VUMGMT_ERROR_LU_DESTRUCTION_PROHIBITED_UNTIL_SNAPSHOT_CREATION_COMPLETES
Language     = English
The LUN could not be destroyed because a snapshot of it is being created. Wait for the snapshot creation to complete, destroy all snapshots of the LUN and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN could not be restored because a new snapshot of it is being created.
;//
;// Generated value should be: 0xE16F809F
;//
MessageId    = 0x809F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VUMGMT_ERROR_ROLLBACK_PROHIBITED_UNTIL_SNAPSHOT_CREATION_COMPLETES
Language     = English
The LUN could not be restored because a new snapshot of it is being created. Wait for the snapshot creation to complete and then retry the operation.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: LO could not start the pause  NAS Server  operation
;//
;// Generated value should be: 0xE16F80A0
;//
MessageId    = 0x80A0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_PAUSE_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only.  NAS Server  %2 pause failed with status %3
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere only
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation is not allowed because the File System or File System snap is being created, destroyed or is offline.
;//
;// Generated value should be: 0xE16F80A1
;//
MessageId    = 0x80A1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_NOT_READY
Language     = English
The Operation could not be performed because the File System or File System snap is being created, destroyed or is offline. Wait for the File System to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere only
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation is not allowed because the File System is shrinking or expanding.
;//
;// Generated value should be: 0xE16F80A2
;//
MessageId    = 0x80A2
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_IS_SHRINKING_OR_EXPANDING
Language     = English
Operation could not be performed because the File System is shrinking or expanding. Wait for size change to complete and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The  NAS Server  couldn't be destroyed because some components in it need to be destroyed first.
;//
;// Generated value should be: 0xE16F80A3
;//
MessageId    = 0x80A3
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VDM_ERROR_NAS_SERVER_FILE_SYSTEMS_STILL_EXIST
Language     = English
The NAS Server could not be destroyed because it still contains at least one File System. Destroy all such File Systems and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The name of the File System cannot be modified because it is bound to its mount path.
;//
;// Generated value should be: 0xE16F80A4
;//
MessageId    = 0x80A4
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_FS_NAME_MODIFICATION_RESTRICTED
Language     = English
The name of the File System cannot be modified because it is bound to its mount path.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System requires recovery now.
;//
;// Generated value should be: 0xE16F80A5
;//
MessageId    = 0x80A5
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_ERROR_REQUIRES_RECOVERY_NOW
Language     = English
The File System requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System requires recovery as soon as possible.
;//
;// Generated value should be: 0xE16F80A6
;//
MessageId    = 0x80A6
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_ERROR_REQUIRES_RECOVERY_ASAP
Language     = English
The File System requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System requires recovery later.
;//
;// Generated value should be: 0xE16F80A7
;//
MessageId    = 0x80A7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_ERROR_REQUIRES_RECOVERY_LATER
Language     = English
The File System requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System is recommended to run recovery.
;//
;// Generated value should be: 0xE16F80A8
;//
MessageId    = 0x80A8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_ERROR_REQUIRES_RECOVERY_RECOMMENDED
Language     = English
The File System requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The file system's recovery has completed and must be acknowledged by the user.
;//
;// Generated value should be: 0xE16F80A9
;//
MessageId    = 0x80A9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_ERROR_REQUIRES_RECOVERY_ACK
Language     = English
The File System recovery must be acknowledged before it can be brought online. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The properties of NAS Server could not be changed because it is faulted.
;//
;// Generated value should be: 0xE16F80AA
;//
MessageId    = 0x80AA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_VDM_PROPERTIES_VDM_FAULTED
Language     = English
The properties of NAS Server could not be changed because it is faulted.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The object could not be created because the NAS Server is faulted.
;//
;// Generated value should be: 0xE16F80AB
;//
MessageId    = 0x80AB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_FAULTED
Language     = English
The object could not be created because the NAS Server is faulted.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The object could not be modified because the NAS Server is faulted.
;//
;// Generated value should be: 0xE16F80AC
;//
MessageId    = 0x80AC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_MODIFY_VDM_FAULTED
Language     = English
The object could not be modified because the NAS Server is faulted.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The object could not be destroyed because the NAS Server is faulted.
;//
;// Generated value should be: 0xE16F80AD
;//
MessageId    = 0x80AD
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_DESTROY_VDM_FAULTED
Language     = English
The object could not be destroyed because the NAS Server is faulted.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Duplicate path for NFS Exports on a specific file system
;//
;// Generated value should be: 0xE16F80AE
;//
MessageId    = 0x80AE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NFS_EXPORT_PATH_ALREADY_EXISTS
Language     = English
Unable to create the NFS Export because the specified path is already in use on this file system. Please retry the operation using a unique path.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a File System with system snaps.
;//
;// Generated value should be: 0xE16F80AF
;//
MessageId    = 0x80AF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_SYSTEM_SNAPSHOTS_EXIST
Language     = English
The File System cannot be destroyed because it still has system snapshots. 
Destroy the system snapshots then retry the File System destroy command.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to recover the File System when the snap is being created, restored or destroyed.
;//
;// Generated value should be: 0xE16F80B0
;//
MessageId    = 0x80B0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVERY_PROHIBITED_DURING_SNAP_CREATION_OR_RESTORE_OR_DESTRUCTION
Language     = English
The File System could not be recovered because a snap in it is being created, restored or destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Internal Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Object cannot be activated on SP because it is not done booting.
;//
;// Generated value should be: 0xE16F80B1
;//
MessageId    = 0x80B1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SP_NOT_READY_FOR_ACTIVATION
Language     = English
The Object cannot be activated at this time because the Storage Processor startup is still in progress.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to create a CIFS share failed
;//
;// Generated value should be: 0xE16F80B2
;//
MessageId    = 0x80B2
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INBAND_CREATE_CIFS_FAILED
Language     = English
An in-band request to create CIFS Share %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to destroy a CIFS share failed
;//
;// Generated value should be: 0xE16F80B3
;//
MessageId    = 0x80B3
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INBAND_DESTROY_CIFS_FAILED
Language     = English
An in-band request to destroy CIFS Share %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to modify a CIFS share failed
;//
;// Generated value should be: 0xE16F80B4
;//
MessageId    = 0x80B4
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INBAND_MODIFY_CIFS_FAILED
Language     = English
An in-band request to modify CIFS Share %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to enable CAVA failed
;//
;// Generated value should be: 0xE16F80B5
;//
MessageId    = 0x80B5
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INBAND_ENABLE_CAVA_FAILED
Language     = English
An in-band request to enable CAVA for NAS Server %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to disable CAVA failed
;//
;// Generated value should be: 0xE16F80B6
;//
MessageId    = 0x80B6
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INBAND_DISABLE_CAVA_FAILED
Language     = English
An in-band request to disable CAVA for NAS Server %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API freeze File System returned an error status.  
;//
;// Generated value should be: 0xE16F80B7
;//
MessageId    = 0x80B7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_FREEZE_FILE_SYSTEM_FAILED
Language     = English
Snapshot File System restore %2 failed while freezing source file system.
Internal Information Only. Freezing File System %3 during restore failed with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API thaw File System returned an error status.  
;//
;// Generated value should be: 0xE16F80B8
;//
MessageId    = 0x80B8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_THAW_FILE_SYSTEM_FAILED
Language     = English
Snapshot File System restore %2 failed while thawing source file system.
Internal Information Only. Thawing File System %3 during restore failed with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if an operation is
;// requested on a File system undergoing recovery. The operation cannot
;// be allowed until File system recovery completes. 
;//
;// Generated value should be: 0xE16F80B9
;//
MessageId    = 0x80B9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_RECOVERY_EXISTS
Language     = English
File system recovery is already in progress. This operation is not allowed until recovery completes.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if a call to function 
;// MluRmResetVDMAndRelatedUFS().
;//
;// Generated value should be: 0xE16F80BA
;//
MessageId    = 0x80BA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_COULD_NOT_RESET_VDM_AND_RELATED_UFS
Language     = English
Could not reset VDM and its dependent objects and related UFSes and their dependent objects.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: Root File System is offline because its config File System is offline.
;//
;// Generated value should be: 0xE16F80BB
;//
MessageId    = 0x80BB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ROOT_UFS_OFFLINE_DUE_TO_OFFLINE_CONFIG_UFS
Language     = English
The root File System is offline because its configuration File System is offline. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: This operation is not allowed because it has 
;// shares or exports initializing or destroying.
;//
;// Generated value should be: 0xE16F80BC
;//
MessageId    = 0x80BC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NFS_EXPORT_OR_CIFS_SHARE_INITIALIZING_OR_DESTROYING
Language     = English
This operation is not allowed because the NFS export or CIFS share is initializing or destroying.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: This operation is not allowed because
;// the domain user password and local admin password in CIFS Server are both modified.
;//
;// Generated value should be: 0xE16F80BD
;//
MessageId    = 0x80BD
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CIFS_SERVER_MODIFY_PARAMS_CONFLICT
Language     = English
This operation is not allowed because the domain user password and local admin password in CIFS Server are both modified.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The VDM network interface cannot be destroyed because it is associated
;// with a VDM CIFS server.
;//
;// Generated value should be: 0xE16F80BE
;//
MessageId    = 0x80BE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_NET_IF_DESTROY_RESTRICTED_WHILE_ASSOCIATED_WITH_CIFS_SERVER
Language     = English
This operation is not allowed because the network interface is associated with a CIFS server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: VDM cannot activate its Root UFS.
;//
;// Generated value should be: 0xE16F80BF
;//
MessageId    = 0x80BF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_ROOT_ACTIVATION_FAILED
Language     = English
Activation of VDM's Root UFS failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: VDM cannot activate its Config UFS.
;//
;// Generated value should be: 0xE16F80C0
;//
MessageId    = 0x80C0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_CONFIG_ACTIVATION_FAILED
Language     = English
Activation of VDM's Config UFS failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: VDM cannot activate its Root and Config UFS.
;//
;// Generated value should be: 0xE16F80C1
;//
MessageId    = 0x80C1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_ROOT_AND_CONFIG_ACTIVATION_FAILED
Language     = English
Activation of VDM's Root and Config UFSes failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System is offline because its NAS Server is offline.
;//
;// Generated value should be: 0xE16F80C2
;//
MessageId    = 0x80C2
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_OFFLINE_DUE_TO_VDM_FAILURE
Language     = English
The File System is offline because its NAS Server is offline. 
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The NAS Server recovery has completed and must be acknowledged by the user.
;//
;// Generated value should be: 0xE16F80C3
;//
MessageId    = 0x80C3
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VDM_ERROR_REQUIRES_RECOVERY_ACK
Language     = English
The NAS Server recovery must be acknowledged before it can be brought online.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt was made to destroy a failed or running expansion object.
;//
;// Generated value should be: 0xE16F80C4
;//
MessageId    = 0x80C4
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_CANNOT_DESTROY_EXPAND_PO
Language     = English
The expansion object cannot be destroyed because it is running or it has failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to cancel the extend_ufs32 process failed.
;//
;// Generated value should be: 0xE16F80C5
;//
MessageId    = 0x80C5
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_CANCEL_EXTEND_UFS32_FAILED
Language     = English
The expansion object %2 failed cancelling the extend_ufs32 process. CBFS status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-LDAP-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F80C6
;//
MessageId    = 0x80C6
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_VDM_LDAP_PROPERTIES_FAILED
Language     = English
NAS Server LDAP failed while attempting to set LDAP object properties.
Internal Information Only. NAS Server DNS %2 failed setting LDAP property with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-LDAP-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F80C7
;//
MessageId    = 0x80C7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLEAR_VDM_LDAP_PROPERTIES_FAILED
Language     = English
NAS Server failed while attempting to clear LDAP object properties.
Internal Information Only. NAS Server LDAP %2 failed clearing LDAP property with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceeded the maximum allowed limits of NAS Server LDAP
;//
;// Generated value should be: 0xE16F80C8
;//
MessageId    = 0x80C8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_LDAP_COUNT_EXCEEDED
Language     = English
Unable to create NAS Server LDAP because the maximum number of NAS Server LDAP objects already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers LDAP objects (%3) already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for LDAP Modify operation.
;//
;// Generated value should be: 0xE16F80C9
;//
MessageId    = 0x80C9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_LDAP_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server LDAP failed.  Insufficient system resources were available.
Internal Information Only. LDAP %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-LDAP-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F80CA
;//
MessageId    = 0x80CA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_LDAP_MODIFY_FAILED
Language     = English
Modify operation for LDAP interface failed.
Internal Information Only. LDAP %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceed the maximum allowed limits of LDAP per NAS Server
;//
;// Generated value should be: 0xE16F80CB
;//
MessageId    = 0x80CB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_LDAP_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server LDAP property because the maximum number of LDAP already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of LDAP %3 already exist for the specified NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem:Unable to start moving NAS server to peer SP 
;//
;// Generated value should be: 0xE16F80CC
;//
MessageId    = 0x80CC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_MOVE_START_FAILED
Language     = English
Unable to start moving NAS server to peer SP.
Internal Information Only. Request to start moving NAS Server %2 failed with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem:Unable to move NAS server to peer SP 
;//
;// Generated value should be: 0xE16F80CD
;//
MessageId    = 0x80CD
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_MOVE_FAILED
Language     = English
Unable to move NAS server to peer SP.
Internal Information Only. Moving NAS Server %2 to peer failed with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-ftp-properties request failed to configure FTP.
;//
;// Generated value should be: 0xE16F80CE
;//
MessageId    = 0x80CE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_FTP_PROP_FAILED
Language     = English
Failed to enable FTP for NAS Server %2.
Internal Information Only. Set FTP %3 properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set-vdm-ftp-properties request failed to disable FTP.
;//
;// Generated value should be: 0xE16F80CF
;//
MessageId    = 0x80CF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLEAR_FTP_PROP_FAILED
Language     = English
Failed to disable FTP for NAS Server %2.
Internal Information Only. Clear FTP %3 properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of FTP servers on the array
;//
;// Generated value should be: 0xE16F80D0
;//
MessageId    = 0x80D0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_FTP_COUNT_EXCEEDED
Language     = English
Cannot enable FTP because the maximum number of FTP servers already exist on the array.
Internal Information Only. Could not create FTP for NAS Server %2 because maximum number of FTP %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of FTP per NAS Server
;//
;// Generated value should be: 0xE16F80D1
;//
MessageId    = 0x80D1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_FTP_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server FTP because the maximum number of FTP already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of FTP %3 already exist for the specified NAS Server.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for FTP Modify operation.
;//
;// Generated value should be: 0xE16F80D2
;//
MessageId    = 0x80D2
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_FTP_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server FTP failed.  Insufficient system resources were available.
Internal Information Only. FTP %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-FTP-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F80D3
;//
MessageId    = 0x80D3
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_FTP_MODIFY_FAILED
Language     = English
Modify operation for FTP failed.
Internal Information Only. FTP %2 failed Modify request with status %3.
.


;// --------------------------------------------------
;// Introduced In: R35                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: An internal error occurred on UFS64
;//
;// Generated value should be: 0xE16F80D4
;//
MessageId    = 0x80D4
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64MGR_ERROR_UFS64_NOTIFIED
Language     = English
An internal error occurred on UFS64. Please contact your service provider.
Internal information only. UFS64 %2 encountered error %3.
.


;// --------------------------------------------------
;// Introduced In: R35                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 was Frozen
;//
;// Generated value should be: 0xE16F80D5
;//
MessageId    = 0x80D5
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64MGR_ERROR_UFS64_FROZEN
Language     = English
An error occurred accessing the storage volume. Please contact your service provider. Internal information only. UFS64 %2 was Frozen.
.

;// --------------------------------------------------
;// Introduced In: R35                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS32 ckpt get properties failed. 
;//
;// Generated value should be: 0xE16F80D6
;//
MessageId    = 0x80D6
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_QUERY_PROPERTIES_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. File System %2 failed with status %3 while querying properties.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System size is smaller than the minimum required.
;//
;// Generated value should be: 0xE16F80D7
;//
MessageId    = 0x80D7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_RESIZE_SF_MINIMUM_SIZE_FAILED
Language     = English
Could not resize the file system because the new size is smaller than the minimum.
Internal Information Only. Could not shrink file system %2 because the new size of %3 sectors is smaller than the minimum (%4).
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System size is greater than the maximum allowed.
;//
;// Generated value should be: 0xE16F80D8
;//
MessageId    = 0x80D8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_RESIZE_SF_MAXIMUM_SIZE_FAILED
Language     = English
Unable to resize the File System because the File System size is greater than the maximum size allowed.
Internal Information Only. Could not resize %2 because the File System size %3 is greater than the maximum size allowed.
.

;// --------------------------------------------------
;// Introduced In: R35                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS32 Snap cannot be internally mounted.
;//
;// Generated value should be: 0xE16F80D9
;//
MessageId    = 0x80D9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFSMGR_ERROR_INTERNAL_MOUNT_FAILED
Language     = English
An error occurred accessing the storage volume. Please contact your service provider.
Internal information only. UFS32 %2 was not internally mounted.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System process ID specified is invalid.
;//
;// Generated value should be: 0xE16F80DA
;//
MessageId    = 0x80DA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_PROCESS_OBJECT_ID_INVALID
Language     = English
The specified process object ID is invalid.
Internal Information Only. The specified process object ID %2 of the File System %3 is invalid.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 error occurred allowing slice request
;//
MessageId    = 0x80DB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64_ERROR_ALLOW_SLICE_REQUEST_ERROR_FROM_CBFS
Language     = English
An error occurred while UFS64 is processing allow_slice_request API.
Internal Information only: CBFS error %2 while allowing slice request for %3.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred setting FS props
;//
MessageId    = 0x80DC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64_ERROR_SET_FS_PROPS_ERROR_FROM_CBFS
Language     = English
An error occurred while setting properties to an UFS64 filesystem
Internal Information only: CBFS error %2 while setting FS props for %3.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred getting FS props
;//
MessageId    = 0x80DD
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64_ERROR_GET_FS_PROPS_ERROR_FROM_CBFS
Language     = English
An error occurred while getting properties from an UFS64 filesystem
Internal Information only: CBFS error %2 while getting FS props %3.
.

;//
;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encountered while replaying slices for File Systems
;//
MessageId    = 0x80DE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64_ERROR_REPLAYING_SLICES
Language     = English
An internal error occurred while adding interrupted add slices. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get_file_system_properties2() request returned an error status,
;//                     causing a RepUpdate PO to fail.
;//
;// Generated value should be: 0xE16F80DF
;//
MessageId    = 0x80DF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_UFS_GET_FS_PROPERTIES_FAILED
Language     = English
Replication update failed while getting the updated file system properties.
Internal Information Only. RepUpdate %2 failed get_file_system_properties2 for %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The RepUpdate PO was unable to update the target object to match
;//                     the new properties obtained from the container file, causing the PO to fail.
;//
;// Generated value should be: 0xE16F80E0
;//
MessageId    = 0x80E0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_SYNC_PROPERTIES_FAILED
Language     = English
Replication update failed while updating the LO objects.
Internal Information Only. RepUpdate %2 failed to sync the properties of %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The RepUpdate PO could not freeze the target UFS, causing the PO to fail.
;//
;// Generated value should be: 0xE16F80E1
;//
MessageId    = 0x80E1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_UFS_FREEZE_FAILED
Language     = English
Replication update failed while attempting to freeze the file system.
Internal Information Only. RepUpdate %2 failed to freeze UFSObj %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: get_file_system_properties2() failed synchronously.  This error is just
;//                     returned to the caller, who morphs it to another status value before logging
;//                     or failing an object.
;//
;// Generated value should be: 0xE16F80E2
;//
MessageId    = 0x80E2
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_GET_FS_PROPERTIES_FAILED
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The RepUpdate PO could not thaw the target UFS, causing the PO to fail.
;//
;// Generated value should be: 0xE16F80E3
;//
MessageId    = 0x80E3
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_UFS_THAW_FAILED
Language     = English
Replication update failed while attempting to thaw the file system.
Internal Information Only. RepUpdate %2 failed to thaw UFSObj %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Replication session update target already has ongoing start/end
;//                     update processing (RepUpdate PO exists for target).
;//
;// Generated value should be: 0xE16F80E4
;//
MessageId    = 0x80E4
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_EXISTS_FOR_TARGET
Language     = English
The replication session resource already has update start/end processing ongoing.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set_file_system_properties2() request returned an error status,
;//                     causing a RepUpdate PO to fail.
;//
;// Generated value should be: 0xE16F80E5
;//
MessageId    = 0x80E5
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_UFS_SET_FS_PROPERTIES_FAILED
Language     = English
Replication update failed while setting the file system properties.
Internal Information Only. RepUpdate %2 failed set_file_system_properties2 for %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Cancel outstanding async process failed during UFS64 shrink abort
;//
;// Generated value should be: 0xE16F80E6
;//
MessageId    = 0x80E6
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS64_SHRINK_CANCEL_ASYNC_PROCESS_FAILED
Language     = English
Failed to cancel outstanding async process during shrink abort.
Internal Information Only. Failed to cancel outstanding async process for File System %2 during shrink abort.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for NFS Modify operation.
;//
;// Generated value should be: 0xE16F80E7
;//
MessageId    = 0x80E7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NFS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server NFS failed.  Insufficient system resources were available.
Internal Information Only. NFS %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-NFS-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F80E8
;//
MessageId    = 0x80E8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_NFS_MODIFY_FAILED
Language     = English
Modify operation for NFS failed.
Internal Information Only. NFS %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: 
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16F80E9
;//
MessageId    = 0x80E9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CONFIGURE_VDM_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. NAS Server %2 configure failed with status %3
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to update the VDM and component objects returned
;//                     an error status, causing a RepUpdate PO to fail.
;//
;// Generated value should be: 0xE16F80EA
;//
MessageId    = 0x80EA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REP_UPDATE_VDM_OBJECT_UPDATE_FAILED
Language     = English
Replication update failed while updating the NAS Server.
Internal Information Only. RepUpdate %2 failed VDM update for %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The VDM-API list_vdm_objects_close() request returned an error status,
;//                     causing the object that had the list session open to fail.
;//
;// Generated value should be: 0xE16F80EB
;//
MessageId    = 0x80EB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_LIST_VDM_OBJECTS_CLOSE_FAILED
Language     = English
Internal Information Only. %2 failed list_vdm_objects_close for %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI5)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified NAS Server object does not exist
;//
;// Generated value should be: 0xE16F80EC
;//
MessageId    = 0x80EC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INVALID_UFS64_ON_VDM32
Language     = English
Cannot create a 64 bit file system on a 32 bit VDM.
Internal Information Only. Could not create %2 because NAS Server object %3 is 32 bit.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI5)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified NAS Server object does not exist
;//
;// Generated value should be: 0xE16F80ED
;//
MessageId    = 0x80ED
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_INVALID_UFS32_ON_VDM64
Language     = English
Cannot create a 32 bit file system on a 64 bit VDM.
Internal Information Only. Could not create %2 because NAS Server object %3 is 64 bit.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event log only
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS Failed to reach ready state 
;//
;// Generated value should be: 0xE16F80EE
;//
MessageId    = 0x80EE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_IMT_MIG_MGR_ERROR_UFS_NOT_READY
Language     = English
An error occurred waiting the UFS to be ready before migrating. Please contact your service provider. Internal information only. UFS Oid %2.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event log only
;//
;// Severity: Error
;//
;// Symptom of Problem: Create migration from UFS32 to UFS64  failed 
;//
;// Generated value should be: 0xE16F80EF
;//
MessageId    = 0x80EF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CREATE_MIGRATION_FAILED
Language     = English
An error occurred creating migration from %3 to %4 on NAS Server %2. Please contact your service provider. Internal information only. VDMStatus %5.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event log only
;//
;// Severity: Error
;//
;// Symptom of Problem: Delete migration failed
;//
;// Generated value should be: 0xE16F80F0
;//
MessageId    = 0x80F0
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DELETE_MIGRATION_FAILED
Language     = English
An error occurred deleting migration %3 on  NAS Server %2. Please contact your service provider. Internal information only. VDMStatus %4.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event log only
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS Failed to reach ready state
;//
;// Generated value should be: 0xE16F80F1
;//
MessageId    = 0x80F1
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_IMT_MIG_MGR_ERROR_VDM_NOT_READY
Language     = English
An error occurred waiting the VDM to be ready. Please contact your service provider. Internal information only. VDM Oid %2.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-asa-properties request failed.
;//
;// Generated value should be: 0xE16F80F2
;//
MessageId    = 0x80F2
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_ASA_PROPERTIES_FAILED
Language     = English
Failed to get the ASA properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get ASA properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API list-vdm-objects-open request failed while
;//                     gathering CIFS Server properties.
;//
;// Generated value should be: 0xE16F80F3
;//
MessageId    = 0x80F3
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_CIFS_SERVER_PROPERTIES_FAILED
Language     = English
Failed to get the CIFS Server properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get CIFS Server properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API list_vdm_objects_close request failed.
;//
;// Generated value should be: 0xE16F80F4
;//
MessageId    = 0x80F4
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLOSE_VDM_LIST_SESSION_FAILED
Language     = English
Failed to close a list session for NAS Server %2.
Internal Information Only. Attempt by %3 to close the list session failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-cava-properties request failed.
;//
;// Generated value should be: 0xE16F80F5
;//
MessageId    = 0x80F5
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_CAVA_PROPERTIES_FAILED
Language     = English
Failed to get the CAVA properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get CAVA properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-nfs-properties request failed.
;//
;// Generated value should be: 0xE16F80F6
;//
MessageId    = 0x80F6
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_NFS_PROPERTIES_FAILED
Language     = English
Failed to get the NFS properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get NFS properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-ftp-properties request failed.
;//
;// Generated value should be: 0xE16F80F7
;//
MessageId    = 0x80F7
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_FTP_PROPERTIES_FAILED
Language     = English
Failed to get the FTP properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get FTP properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API list-vdm-objects-open request failed while
;//                     gathering network interface properties.
;//
;// Generated value should be: 0xE16F80F8
;//
MessageId    = 0x80F8
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_NETWORK_INTERFACE_PROPERTIES_FAILED
Language     = English
Failed to get the network interface properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get network interface properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API set_file_system_properties2() request returned an error status,
;//                     causing a RefreshSnap PO to fail.
;//
;// Generated value should be: 0xE16F80F9
;//
MessageId    = 0x80F9
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REFRESH_SNAP_UFS_SET_FS_PROPERTIES_FAILED
Language     = English
Refresh Snap failed while setting the snap file system properties.
Internal Information Only. RefreshSnap %2 failed set_file_system_properties2 for %3. Status %4
.

;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Refresh operation is defined only for snaps. But user supplied an oid which is not a snap.
;//
;// Generated value should be: 0xE16F80FA
;//
MessageId    = 0x80FA
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_REFRESH_SNAP_INVALID_RESOURCE_SUPPLIED
Language     = English
Refresh Snap operation can be be performed only on snaps. OID is not a snap.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-ldap-properties request failed.
;//
;// Generated value should be: 0xE16F80FB
;//
MessageId    = 0x80FB
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_LDAP_PROPERTIES_FAILED
Language     = English
Failed to get the LDAP properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get LDAP properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-dns-properties request failed.
;//
;// Generated value should be: 0xE16F80FC
;//
MessageId    = 0x80FC
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_DNS_PROPERTIES_FAILED
Language     = English
Failed to get the DNS properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get DNS properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-nis-properties request failed.
;//
;// Generated value should be: 0xE16F80FD
;//
MessageId    = 0x80FD
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_NIS_PROPERTIES_FAILED
Language     = English
Failed to get the NIS properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get NIS properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API list-vdm-objects-open request failed while
;//                     gathering CIFS share properties.
;//
;// Generated value should be: 0xE16F80FE
;//
MessageId    = 0x80FE
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_CIFS_SHARE_PROPERTIES_FAILED
Language     = English
Failed to get the CIFS Share properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get CIFS Share properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API list-vdm-objects-open request failed while
;//                     gathering NFS export properties.
;//
;// Generated value should be: 0xE16F80FF
;//
MessageId    = 0x80FF
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_NFS_EXPORT_PROPERTIES_FAILED
Language     = English
Failed to get the NFS Export properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get NFS Export properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to set destination mode on the VDM which is not ready is not supported.
;//
;// Generated value should be: 0xE16F8100
;//
MessageId    = 0x8100
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_NOT_READY_SET_DESTINATION_NOT_SUPPORTED
Language     = English
Change destination mode of 32 format NAS Server isn't allowed.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to set destination mode on the VDM which has PO running is not supported.
;//
;// Generated value should be: 0xE16F8101
;//
MessageId    = 0x8101
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_HAS_APO_SET_DESTINATION_NOT_SUPPORTED
Language     = English
Change destination mode of NAS Server which has PO running on it isn't allowed.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to set local only properties to the destination VDM is not supported.
;//
;// Generated value should be: 0xE16F8102
;//
MessageId    = 0x8102
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DESTINATION_VDM_SET_GLOBALONLY_PROPERTY_NOT_SUPPORTED
Language     = English
Set properties ( mpSharingEnabled, unixDirectoryService, defaultWindowsUser, defaultUnixUser, credCacheRetention and preferredInterfaces ) on NAS Server in destination mode isn't allowed.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to set destination mode on the VDM32 is not supported.
;//
;// Generated value should be: 0xE16F8103
;//
MessageId    = 0x8103
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM32_SET_DESTIANTION_NOT_SUPPORTED
Language     = English
Change destination mode of 32 format NAS Server isn't allowed.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to set destination mode and other async properties is not supported.
;//
;// Generated value should be: 0xE16F8104
;//
MessageId    = 0x8104
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM_SET_DESTINATION_AND_ASYNCPROP_NOT_SUPPORTED
Language     = English
It isn't allowed to change destination mode and other properties of NAS Server in one command.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-properties request failed.
;//
;// Generated value should be: 0xE16F8105
;//
MessageId    = 0x8105
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_VDM_PROPERTIES_FAILED
Language     = English
Failed to get the properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get NAS Server properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI7)
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to set a VDM32 as migration destination.
;//
;// Generated value should be: 0xE16F8106
;//
MessageId    = 0x8106
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VDM32_SET_MIGRATION_DESTINATION_NOT_SUPPORTED
Language     = English
Unable to set a VDM32 as migration destination.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to start SID mapping operation when the File System is preparing or optimizing.
;//
;// Generated value should be: 0xE16F8200
;//
MessageId    = 0x8200
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_FAILED_WHEN_UFS_PREPARING_OR_OPTIMIZING
Language     = English
The File System could not support SID mapping operation because it is preparing or optimizing.
.

;// --------------------------------------------------
;// Introduced In:  KH+ SP2 (PSI6)
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to start a SID mapping operation when the File System is initializing.
;//
;// Generated value should be: 0xE16F8201
;//
MessageId    = 0x8201
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_FAILED_WHEN_UFS_INITIALIZING
Language     = English
The File System could not support SID mapping operation because it is still initializing.
.

;// --------------------------------------------------
;// Introduced In:  KH+ SP2 (PSI6)
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to start SID mapping operation when the File System is being destroyed.
;//
;// Generated value should be: 0xE16F8202
;//
MessageId    = 0x8202
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_FAILED_WHEN_UFS_DESTROYING
Language     = English
The File System could not support SID mapping operation because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In:  KH+ SP2 (PSI6)
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to abort SID mapping operation the File System when the process is not in expected state.
;//
;// Generated value should be: 0xE16F8203
;//
MessageId    = 0x8203
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ABORT_SID_MAPPING_FAILED_PO_NOT_EXPECTED_STATE
Language     = English
The File System's sis mapping could not be aborted because the process is not in ready state.
.

;// --------------------------------------------------
;// Introduced In:  KH+ SP2 (PSI6)
;//
;// Usage: Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to start SID mapping operation when File System is not mounted.
;//
;// Generated value should be: 0xE16F8204
;//
MessageId    = 0x8204
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_FAILED_UFS_NOT_MOUNTED
Language     = English
The File System could not support SID mapping operation because it is not mounted.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to start a SID mapping operation when vdm is destroying.
;//
;// Generated value should be: 0xE16F8205
;//
MessageId    = 0x8205
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_VDM_DESTROYING
Language     = English
Internal Error. Cannot run SID mapping on File System of a destroying NAS Server. 
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if an operation is
;// requested on a File system undergoing SID mapping. The operation cannot
;// be allowed until File system SID mapping completes. 
;//
;// Generated value should be: 0xE16F8206
;//
MessageId    = 0x8206
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_EXISTS
Language     = English
SID mapping is already in progress. This operation is not allowed until SID mapping completes.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: SID Mapping process ID specified is invalid.
;//
;// Generated value should be: 0xE16F8207
;//
MessageId    = 0x8207
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_PROCESS_OBJECT_ID_INVALID
Language     = English
The specified process object ID is invalid.
Internal Information Only. The specified process object ID %2 of the File System %3 is invalid.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: SID mapping failed.
;//
;// Generated value should be: 0xE16F8208
;//
MessageId    = 0x8208
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_ERROR
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. SID mapping failed for File System %2 with error %3.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: SID mapping failed due to unexpected object state.
;//
;// Generated value should be: 0xE16F8209
;//
MessageId    = 0x8209
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_UNEXPECTED_STATE
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. SID mapping failed for File System %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: SID mapping abort failed due to unexpected object state.
;//
;// Generated value should be: 0xE16F820A
;//
MessageId    = 0x820A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_ABORT_FAILED
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. SID mapping abort failed for File System %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: SID mapping object deletion failed due to unexpected object state.
;//
;// Generated value should be: 0xE16F820B
;//
MessageId    = 0x820B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SID_MAPPING_STILL_RUNNING
Language     = English
Internal error. Please contact your service provider.
Internal Information Only. SID mapping object deletion failed for File System %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Starting SID mapping operation.
;//
;// Generated value should be: 0x416F820C
;//
MessageId    = 0x820C
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_INFO_SID_MAPPING_STARTED
Language     = English
A manual SID mapping of file system %2 has been initiated and is currently running in the background.
.

;// --------------------------------------------------
;// Introduced In: KH+ SP2 (PSI6)
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: SID mapping operation ended.
;//
;// Generated value should be: 0x416F820D
;//
MessageId    = 0x820D
Severity     = Informational
Facility     = MLUUDI
SymbolicName = MLU_INFO_SID_MAPPING_ENDED
Language     = English
A manual SID mapping of file system %2 is ended with status %3.
.


;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: FLR is not supported for UFS64
;//
;// Generated value should be: 0xE16F820E
;//
MessageId    = 0x820E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_FLR_IS_NOT_SUPPORTED_ON_UFS64
Language     = English
Creating a UFS64 filesystem with FLR enabled is not supported
.


;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 FS can not be used as ReplicationDestination
;//
;// Generated value should be: 0xE16F820F
;//
MessageId    = 0x820F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS64_FS_AS_REPLICATION_DESTINATION_IS_NOT_SUPPORTED
Language     = English
Creating a UFS64 Filesyatem as a replication destination is not supported
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI6)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: DEDUP is not supported for UFS64
;//
;// Generated value should be: 0xE16F8210
;//
MessageId    = 0x8210
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_DEDUP_IS_NOT_SUPPORTED_ON_UFS64
Language     = English
Modifying  DEDUP properties on a UFS64 FS is not supported
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI7)
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to do VDM Switchover operation, fs is not in syncing state.
;//
;// Generated value should be: 0xE16F8211
;//
MessageId    = 0x8211
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_VDM_SWITCHOVER_FS_NOT_SYNCH
Language     = English
Unable to do VDM Switchover operation, fs is not in syncing state.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI7)
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The overall VDM switchover operation failed, though the Migration switchover completed successfully.
;//
;// Generated value should be: 0xE16F8212
;//
MessageId    = 0x8212
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_VDM_SWITCHOVER_HAS_BEEN_DONE
Language     = English
The overall VDM switchover operation failed, though the Migration switchover completed successfully.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI7)
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 size change is restricted
;//
;// Generated value should be: 0xE16F8213
;//
MessageId    = 0x8213
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_FS_SIZE_CHANGE_RESTRICTED
Language     = English
The size property change of target file system is restricted because it is readonly or it is a snap or it's under snap restore
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 Snap cannot be internally mounted.
;//
;// Generated value should be: 0xE16F8214
;//
MessageId    = 0x8214
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64MGR_ERROR_INTERNAL_MOUNT_FAILED
Language     = English
An error occurred accessing the storage volume. Please contact your service provider.
Internal information only. UFS64 %2 was not internally mounted.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Can't create UFS64 on VDM32 
;//
;// Generated value should be: 0xE16F8215
;//
MessageId    = 0x8215
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS64_CREATE_ERROR_INCOMPATIBLE_VDM
Language     = English
Unable to create a 64 bit file system on a 32 bit NAS server.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Can't create UFS32
;//
;// Generated value should be: 0xE16F8216
;//
MessageId    = 0x8216
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFS_CREATE_ERROR_DISALLOWED
Language     = English
Create a 32 bit file system is not allowed on this system.
.


;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 Snap cannot be internally mounted.
;//
;// Generated value should be: 0xE16F8217
;//
MessageId    = 0x8217
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_VDM_ERROR_DUPLICATE_EXPORTED_FSID
Language     = English
The export FSID already exists.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on MLU_VVOL_OBJECT_CID (VVolMaxVVols) reached, cannot create more
;//
;// Generated value should be: 0xE16F8301
;//
MessageId    = 0x8301
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_VVOL_COUNT_EXCEEDED
Language     = English
Unable to create the VVol because the maximum number of VVols already exist.
Could not create %2 because there are already %3 VVols on the array.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on VVol LDFSes (VVolMaxLDFS) reached, cannot create more
;//
;// Generated value should be: 0xE16F8302
;//
MessageId    = 0x8302
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_LDFS_COUNT_EXCEEDED
Language     = English
Unable to create the VVol because the maximum number of Lower Deck File Systems already exist.
Could not create %2 because there are already %3 LDFSes on the array.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on Container Allocation Objects reached, cannot create more
;//
;// Generated value should be: 0xE16F8303
;//
MessageId    = 0x8303
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_POOL_ALLOC_COUNT_EXCEEDED
Language     = English
Unable to create the Pool Allocation Object because the maximum number already exist.
Could not create %2 because there are already %3 Pool Allocation Trackers on the array.
.

;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on bound vvols reached
;//
;// Generated value should be: 0xE16F8304
;//
MessageId    = 0x8304
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_BOUND_VVOL_EXCEEDED
Language     = English
Unable to bind VVol because the maximum number of concurrently bound objects already exist.
Could not bind %2 because there are already %3 bound vvols on the array.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of VDM network routes
;//
;// Generated value should be: 0xE16F8305
;//
MessageId    = 0x8305
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_NET_ROUTE_COUNT_EXCEEDED
Language     = English
Unable to create the VDM network route because the maximum number of VDM network routes already exist on the array.
Internal Information Only. Could not create %2 because maximum number of VDM network routes %3 already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to create object while the NAS network interface is being destroyed.
;//
;// Generated value should be: 0xE16F8306
;//
MessageId    = 0x8306
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_NET_IF_DESTROYING
Language     = English
Unable to create object while the NAS Network interface is being destroyed.
Internal Information Only. Could not create object because the NAS Network interface is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to create object while the NAS network interface is disabled.
;//
;// Generated value should be: 0xE16F8307
;//
MessageId    = 0x8307
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_NET_IF_OFFLINE
Language     = English
Unable to create object while the NAS Network interface is disabled.
Internal Information Only. Could not create object because the NAS Network interface is disabled.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: New route has invalid CIDR.
;//
;// Generated value should be: 0xE16F8308
;//
MessageId    = 0x8308
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_NET_ROUTE_INVALID_CIDR
Language     = English
Unable to create the VDM network route because the specified destination prefix length (CIDR) is invalid.
Internal Information Only. Could not create object because the specified destination prefix length (CIDR) is invalid.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: New route has invalid IP protocol type.
;//
;// Generated value should be: 0xE16F8309
;//
MessageId    = 0x8309
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VDM_NET_ROUTE_INVALID_IP_PROTO_TYPE
Language     = English
Unable to create the VDM network route because the specified IP protocol is invalid.
Internal Information Only. Could not create object because the specified IP protocol is invalid.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere only
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS64 shrink failed
;//
;// Generated value should be: 0xE16F830A
;//
MessageId    = 0x830A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CBFS_SHRINK_UFS_FAILED
Language     = English
Shrink fails on the target UFS64 file system due to file system not in mounted state
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on NAS Config vVols reached
;//
;// Generated value should be: 0xE16F830B
;//
MessageId    = 0x830B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_NAS_CONFIG_VVOL_COUNT_EXCEEDED
Language     = English
Unable to create NAS Config vVol because the maximum number of NAS Config vVols already created.
Could not create NAS Config vVol %2 because there are already %3 NAS Config vVols created on the array.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on snapshots/fastclones in VVol family reached
;//
;// Generated value should be: 0xE16F830C
;//
MessageId    = 0x830C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_VVOL_DERIVATIVES_COUNT_EXCEEDED
Language     = English
Unable to create the VVol snapshot or VVol fastclone because the maximum number of derivatives is already reached.
Could not create a snapshot or fastclone of %2 because there are already %3 derivatives exists
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on base vvols reached
;//
;// Generated value should be: 0xE16F830D
;//
MessageId    = 0x830D
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_BASE_VVOL_COUNT_EXCEEDED
Language     = English
Unable to create the VVol because the maximum number of base vvols is already reached.
Could not create a VVol because there are already %2 base vvol exists
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to create object while the VVOLPEFS is offline.
;//
;// Generated value should be: 0xE16F830E
;//
MessageId    = 0x830E
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_CREATE_VVOLPEFS_OFFLINE
Language     = English
Unable to create object while the VVOLPEFS is offline.
Internal Information Only. Could not create object because the VVOLPEFS is offline.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The VVOLPEFS-API mkdir returned an error status
;//
;// Generated value should be: 0xE16F830F
;//
MessageId    = 0x830F
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_MKDIR_VVOLPEFS_FAILED
Language     = English
Internal Information Only. VVOLPE file system %2 failed creation %3 directory with status %4
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The VVOLPEFS-API rmdir returned an error status
;//
;// Generated value should be: 0xE16F8310
;//
MessageId    = 0x8310
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_OBJECT_RMDIR_VVOLPEFS_FAILED
Language     = English
Internal Information Only. File System %2 failed removing %3 directory with status %4
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-KERBEROS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F8311
;//
MessageId    = 0x8311
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SET_VDM_KERBEROS_PROPERTIES_FAILED
Language     = English
NAS Server failed while attempting to set KERBEROS object properties.
Internal Information Only. NAS Server %2 failed setting KERBEROS property with status %3.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-KERBEROS-PROPERTIES request failed.
;//
;// Generated value should be: 0xE16F8312
;//
MessageId    = 0x8312
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_CLEAR_VDM_KERBEROS_PROPERTIES_FAILED
Language     = English
NAS Server KERBEROS failed while attempting to clear KERBEROS object properties.
Internal Information Only. NAS Server %2 failed clearing KERBEROS property with status %3.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceeded the maximum allowed limits of NAS Server KERBEROS
;//
;// Generated value should be: 0xE16F8313
;//
MessageId    = 0x8313
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_SFS_KERBEROS_COUNT_EXCEEDED
Language     = English
Unable to create NAS Server KERBEROS because the maximum number of NAS Server KERBEROS objects already exist on the array.
Internal Information Only. Could not create %2 because maximum number of NAS Servers KERBEROS objects (%3) already exist on the array.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted to exceed the maximum allowed limits of KERBEROS per NAS Server
;//
;// Generated value should be: 0xE16F8314
;//
MessageId    = 0x8314
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_MAX_KERBEROS_PER_SFS_COUNT_EXCEEDED
Language     = English
Unable to create the NAS Server KERBEROS property because the maximum number of KERBEROS already exists for the specified NAS Server.
Internal Information Only. Could not create %2 because maximum number of KERBEROS %3 already exist for the specified NAS Server.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Scratch buffer was too small for KERBEROS Modify operation.
;//
;// Generated value should be: 0xE16F8315
;//
MessageId    = 0x8315
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_KERBEROS_MODIFY_NO_MEMORY
Language     = English
Modify operation for NAS Server KERBEROS failed.  Insufficient system resources were available.
Internal Information Only. KERBEROS %2: Buffer needed %3 bytes, got %4.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API SET-VDM-KERBEROS-PROPERTIES call failed, or could not be called.
;//
;// Generated value should be: 0xE16F8316
;//
MessageId    = 0x8316
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_SFS_KERBEROS_MODIFY_FAILED
Language     = English
Modify operation for KERBEROS interface failed.
Internal Information Only. KERBEROS %2 failed Modify request with status %3.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File-API get-vdm-kerberos-properties request failed.
;//
;// Generated value should be: 0xE16F8317
;//
MessageId    = 0x8317
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_GET_KERBEROS_PROPERTIES_FAILED
Language     = English
Failed to get the KERBEROS properties for NAS Server %2.
Internal Information Only. Attempt by %3 to get KERBEROS properties failed with status %4.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Logged in Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to send message to peer for verifying the pool full policy
;//
;// Generated value should be: 0xE16F8318
;//
MessageId    = 0x8318
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_UFS_FAILED_TO_VERIFY_POOL_FULL_POLICY
Language     = English
Failed to check thresholds for applying pool-full policy on User File System %2.
Internal Information Only. Failed to send message to peer for verifying pool-full policy for File System %2. Status %3.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: For Audit and Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: User selected to fail writes and not invalidate snaps on the File-System. Free space in the pool went below threshold
;//                     and file system is now mounted as read only 
;//
;// Generated value should be: 0xE16F8319
;//
MessageId    = 0x8319
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_ALERT_ABOUT_READ_ONLY_MOUNT_UFS64_SNAPINVALIDATION
Language     = English
The file system %2 has been mounted read-only because the amount of free space in its pool has dropped below %3 GB and its pool-full policy is that writes will be failed in favor of preserving snapshots.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: soft limit on Bound NAS Config vVols reached
;//
;// Generated value should be: 0xE16F831A
;//
MessageId    = 0x831A
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_VVOL_ERROR_MAX_BOUND_NAS_CONFIG_VVOL_COUNT_EXCEEDED
Language     = English
Unable to bind NAS Config vVol because the maximum number of NAS Config vVols already bound.
Could not bind NAS Config vVol %2 because there are already %3 NAS Config vVols bound on the array.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to update profile on a VVOL that is already being migrated to another pool.
;//
;// Generated value should be: 0xE16F831B
;//
MessageId    = 0x831B
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_ERROR_VVOL_MIGRATION_IN_PROGRESS
Language     = English
The VVOL %2 is being migrated to another pool. Wait for migration to complete before attempting again.
.

;// --------------------------------------------------
;// Introduced In: PSI10
;//
;// Usage: Logged in ktraces only
;//
;// Severity: Error
;//
;// Symptom of Problem: UFS LowerDeck VU cannot be determined.
;//
;// Generated value should be: 0xE16F831C
;//
MessageId    = 0x831C
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UFSMGR_ERROR_COULD_NOT_FIND_BASE
Language     = English
Could not determine the base VU associated with the UFS %2.
.

;
;
;
;
;//============================
;//======= CRITICAL MSGS ======
;//============================
;
;
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event log and returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A System NAS Server has gone offline.
;//
;// Generated value should be: 0xE16FC000
;//
MessageId    = 0xC000
;//SSPG C4type=User
Severity     = Error
Facility     = MLUUDI
SymbolicName = MLU_UDI_CRITICAL_SYSTEM_VDM_OFFLINE
Language     = English
An internal system service (VDM %2) is offline. Replication is not available, and other system services may not be available.
.


;#endif //__K10_MLU_UDI_MESSAGES__
