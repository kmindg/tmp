;//++
;// Copyright (C) EMC Corporation, 2007-2013
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10MluSnapshotMessages.mc
;//
;// Description:     This file contains the message catalogue for Snapshot messages
;//                  in the MLU and SnapBack drivers.
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
;#ifndef __K10_MLU_SNAPSHOT_MESSAGES__
;#define __K10_MLU_SNAPSHOT_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  MLUSnapshot   = 0x16D : FACILITY_SNAPSHOT
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
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_SNAP_RECEIVED
Language     = English
Request received to create snapshot %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0001
;//
MessageId    = 0x0001
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_SNAP_COMPLETED
Language     = English
Request to create snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0002
;//
MessageId    = 0x0002
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_SNAP_RECEIVED
Language     = English
Request received to destroy snapshot %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0003
;//
MessageId    = 0x0003
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_SNAP_COMPLETED
Language     = English
Request to destroy snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0004
;//
MessageId    = 0x0004
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_SNAP_RECEIVED
Language     = English
Request received to attach snapshot %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0005
;//
MessageId    = 0x0005
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_SNAP_COMPLETED
Language     = English
Request to attach snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0006
;//
MessageId    = 0x0006
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DETACH_SNAP_RECEIVED
Language     = English
Request received to detach snapshot %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0007
;//
MessageId    = 0x0007
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DETACH_SNAP_COMPLETED
Language     = English
Request to detach snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0008
;//
MessageId    = 0x0008
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SET_SNAP_PROPS_RECEIVED
Language     = English
Request received to set properties for snapshot %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0009
;//
MessageId    = 0x0009
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SET_SNAP_PROPS_COMPLETED
Language     = English
Request to set properties for snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Create Snap LUN operation initiated
;//
;// Generated value should be: 0x616D000A
;//
MessageId    = 0x000A
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_SNAPLU_RECEIVED
Language     = English
Request received to create snapshot mount point %2 using WWN %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destroy snapshot LUN operation initiated
;//
;// Generated value should be: 0x616D000B
;//
MessageId    = 0x000B
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_SNAPLU_RECEIVED
Language     = English
Request received to destroy snapshot mount point %2.
Internal Information Only. VU object %3, Pool object %4.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D000C
;//
MessageId    = 0x000C
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_CG_COMPLETED
Language     = English
Request to create consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D000D
;//
MessageId    = 0x000D
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_CG_COMPLETED
Language     = English
Request to destroy consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D000E
;//
MessageId    = 0x000E
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_CG_MEMBERS_COMPLETED
Language     = English
Request to add members to consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D000F
;//
MessageId    = 0x000F
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REMOVE_CG_MEMBERS_COMPLETED
Language     = English
Request to remove members from consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0010
;//
MessageId    = 0x0010
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REPLACE_CG_MEMBERS_COMPLETED
Language     = English
Request to replace members from consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0011
;//
MessageId    = 0x0011
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAP_RECEIVED
Language     = English
Request received to restore snapshot %2.
Internal Information Only. Snapshot is a %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0012
;//
MessageId    = 0x0012
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAP_COMPLETED
Language     = English
Request to restore snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0013
;//
MessageId    = 0x0013
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_CG_RECEIVED
Language     = English
Request received to create consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0014
;//
MessageId    = 0x0014
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_CG_RECEIVED
Language     = English
Request received to destroy consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0015
;//
MessageId    = 0x0015
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_CG_MEMBERS_RECEIVED
Language     = English
Request received to add members to consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0016
;//
MessageId    = 0x0016
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REMOVE_CG_MEMBERS_RECEIVED
Language     = English
Request received to remove members from consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0017
;//
MessageId    = 0x0017
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REPLACE_CG_MEMBERS_RECEIVED
Language     = English
Request received to replace members in consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0018
;//
MessageId    = 0x0018
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SET_CG_PROPS_RECEIVED
Language     = English
Request received to set properties for consistency group %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0019
;//
MessageId    = 0x0019
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SET_CG_PROPS_COMPLETED
Language     = English
Request to set properties for consistency group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001A
;//
MessageId    = 0x001A
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_MEMBER_ADDED
Language     = English
LUN %2 has been added to consistency group %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001B
;//
MessageId    = 0x001B
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_MEMBER_REMOVED
Language     = English
LUN %2 has been removed from consistency group %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001C
;//
MessageId    = 0x001C
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MEMBER_CREATED
Language     = English
Snapshot %2 has been created for LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001D
;//
MessageId    = 0x001D
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MEMBER_ATTACHED
Language     = English
Snapshot %2 has been attached to snapshot mount point %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001E
;//
MessageId    = 0x001E
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MEMBER_DETACHED
Language     = English
Snapshot %2 has been detached from snapshot mount point %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D001F
;//
MessageId    = 0x001F
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTRUCTION_INITIATED
Language     = English
%2 destroy of snapshot %3 initiated.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0x616D0020
;//
MessageId    = 0x0020
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTRUCTION_COMPLETED
Language     = English
Destroy of snapshot %2 completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0021
;//
MessageId    = 0x0021
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_INFO_OPERATION_WAS_ABORTED
Language     = English
The snapback operation was terminated prematurely.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0022
;//
MessageId    = 0x0022
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAP_PROCESS_INITIATED
Language     = English
Rollback object %2 has been created for restoring snapshot %3 to destination LUN %4.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0023
;//
MessageId    = 0x0023
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAP_PROCESS_COMPLETED
Language     = English
Rollback object %2 completed successfully for a(n) %3 operation to LUN %4.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0024
;//
MessageId    = 0x0024
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_HARVEST_INITIATED
Language     = English
Auto-delete initiated. Pool %2 usable size %3 consumed %4 secondary %5 pool full breached %6 snap space breached %7.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0025
;//
MessageId    = 0x0025
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_HARVEST_CONTINUING
Language     = English
Internal Information Only. Auto-delete continuing. Pool %2 usable size %3 consumed %4 secondary %5 pool full breached %6 snap space breached %7.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0x616D0026
;//
MessageId    = 0x0026
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_HARVEST_COMPLETED
Language     = English
Auto-delete completed. Pool %2 usable size %3 consumed %4 secondary %5 pool full breached %6 snap space breached %7.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;//             When destroying snapshots we rename them so the old name can immediately be reused.
;//             We log the new name for diagnostics should something go wrong during the destruction.
;//
;// Generated value should be: 0x616D0027
;//
MessageId    = 0x0027
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_NEW_NAME
Language     = English
The new name for snapshot being destroyed is %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;//             When creating a snapshot we need to log information if the creation exceeds 5 seconds.
;//             Because this is internal information we log the OID of the object we are creating the
;//             snapshot for, the total time, time to commit the initial transaction and the time we
;//             waited for FMC's to finish. 
;//
;// Generated value should be: 0x616D0028
;//
MessageId    = 0x0028
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_TIME_THRESHOLD_EXCEEDED
Language     = English
Internal Information Only. Create snapshot for %2 exceeded 5 seconds, total %3 ms, initial commit %4 ms, FMC wait %5 ms.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Create Snap request completed
;//
;// Generated value should be: 0x616D0029
;//
MessageId    = 0x0029
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_CREATE_SNAP_COMPLETED
Language     = English
An in-band request to create snapshot %2 completed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Destroy Snap request completed
;//
;// Generated value should be: 0x616D002A
;//
MessageId    = 0x002A
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_DESTROY_SNAP_COMPLETED
Language     = English
An in-band request to destroy snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Attach Snap request completed
;//
;// Generated value should be: 0x616D002B
;//
MessageId    = 0x002B
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_ATTACH_SNAP_COMPLETED
Language     = English
An in-band request to attach snapshot %2 to Mount Point %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Detach Snap request completed
;//
;// Generated value should be: 0x616D002C
;//
MessageId    = 0x002C
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_DETACH_SNAP_COMPLETED
Language     = English
An in-band request to detach snapshot %2 from Mount Point %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Copy Snap request completed
;//
;// Generated value should be: 0x616D002D
;//
MessageId    = 0x002D
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_COPY_SNAP_COMPLETED
Language     = English
An in-band request to create a copy of snapshot %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Create CG request completed
;//
;// Generated value should be: 0x616D002E
;//
MessageId    = 0x002E
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_CREATE_CG_COMPLETED
Language     = English
An in-band request to create LUN group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Inband Replacement of CG members request completed
;//
;// Generated value should be: 0x616D002F
;//
MessageId    = 0x002F
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_REPLACE_CG_COMPLETED
Language     = English
An in-band request to replace the members of LUN group %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Snapshot Destroy by Auto-Delete completed
;//
;// Generated value should be: 0x616D0030
;//
MessageId    = 0x0030
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_BY_AUTO_DELETE_COMPLETED
Language     = English
Snapshot %2 was destroyed as part of auto-delete.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Snapshot Destroy by Expiration completed
;//
;// Generated value should be: 0x616D0031
;//
MessageId    = 0x0031
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_BY_EXPIRATION_COMPLETED
Language     = English
Snapshot %2 has expired and was automatically destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Protective Snapshot for Rollback has been created.
;//
;// Generated value should be: 0x616D0032
;//
MessageId    = 0x0032
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_PROTECTIVE_SNAP_CREATED
Language     = English
Backup snapshot %2 was automatically created when restoring snapshot %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request to prepare for snap received.
;//
;// Generated value should be: 0x616D0033
;//
MessageId    = 0x0033
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_PREPARE_FOR_SNAP_RECEIVED
Language     = English
Request received to prepare %2 to be snapped.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request to prepare for snap completed successfully.
;//
;// Generated value should be: 0x616D0034
;//
MessageId    = 0x0034
;//SSPG C4type=Audit
Severity     = Informational
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_PREPARE_FOR_SNAP_COMPLETED
Language     = English
Request to prepare %2 to be snapped completed successfully.
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
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_WARNING_TEMPLATE
Language     = English
Only for template
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
;// Introduced In: R32                                      
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Internal bu where MLU ran out of manifests but our counter indicates we
;//                     should still have some left.
;//
;// Generated value should be: 0xE16D8000
;//
MessageId    = 0x8000
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_NO_FREE_MANIFESTS
Language     = English
An internal error occurred. Gather SP collects and contact your service provider. 
Internal Information Only. There are no free manifests available.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to create more than the maximum number of snapshots. 
;//
;// Generated value should be: 0xE16D8001
;//
MessageId    = 0x8001
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_SNAPS_EXCEEDED
Language     = English
The maximum number of snapshots have already been created on this array.
Internal Information Only. Could not create snapshot for snap source %3 because maximum number of snapshots %2 already exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Internal error where we somehow have a duplicate family ID. 
;//
;// Generated value should be: 0xE16D8002
;//
MessageId    = 0x8002
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DUPLICATE_FAMILY_ID
Language     = English
An internal error occurred. Gather SP collects and contact your service provider. 
Internal Information Only. A duplicate family identifier %2 was found for object ID %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//                                                                     
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a snapshot that is attached. 
;//
;// Generated value should be: 0xE16D8003
;//
MessageId    = 0x8003
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAP_IS_ATTACHED
Language     = English
The snapshot cannot be destroyed because it is attached to a snapshot mount point.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a snapshot of an unattached snapshot mount point. 
;//
;// Generated value should be: 0xE16D8004
;//
MessageId    = 0x8004
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPLU_IS_UNATTACHED
Language     = English
The snapshot cannot be created because the snapshot mount point is not attached.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Specified snapshot name is already in use. 
;//
;// Generated value should be: 0xE16D8005
;//
MessageId    = 0x8005
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAP_NAME_EXISTS
Language     = English
The specified snapshot name is already in use.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot property does not allow it to be attached. 
;//
;// Generated value should be: 0xE16D8006
;//
MessageId    = 0x8006
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_NOT_ALLOWED
Language     = English
The specified snapshot cannot be attached because it was created with its allow attach flag set to FALSE.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot mount point property does not allow it to be attached.
;//
;// Generated value should be: 0xE16D8007
;//
MessageId    = 0x8007
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPLUN_UNATTACHABLE
Language     = English
The properties of this snapshot mount point do not allow it to be attached.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to attach or restore a snapshot to a snapshot mount point from another family. 
;//
;// Generated value should be: 0xE16D8008
;//
MessageId    = 0x8008
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FAMILY_IDS_DIFFER
Language     = English
The attach or restore operation is disallowed because the snapshot and snapshot mount point are associated with different primary LUNs. 
Use LUNs and snapshots from correct primary LUN for attach and restore.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to use an attached snapshot in a copy command.
;//
;// Generated value should be: 0xE16D8009
;//
MessageId    = 0x8009
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_IS_ATTACHED_SNAPSHOT
Language     = English
An attached snapshot cannot be used as the source of a copy operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to create a snapshot when the FS is not ready. 
;//
;// Generated value should be: 0xE16D800A
;//
MessageId    = 0x800A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FS_NOT_READY_OBSOLETE
Language     = English
The file system to create the snapshot in is not in the ready state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a snapshot when the source file is not ready.
;//
;// Generated value should be: 0xE16D800B
;//
MessageId    = 0x800B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FILE_NOT_READY
Language     = English
The snapshot cannot be created because the source is not in the ready state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D800C
;//
MessageId    = 0x800C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_INVALID_OP
Language     = English
The specified operation is not supported.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D800D
;//
MessageId    = 0x800D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_STATUS_NOT_FOUND
Language     = English
Cannot find the operation status.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D800E
;//
MessageId    = 0x800E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_DUPLICATE_COOKIE
Language     = English
The specified LUN now requires a different cookie value than the one that was used by the caller.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D800F
;//
MessageId    = 0x800F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_MISSING_SNAP_NAME
Language     = English
The snapshot name was not specified.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8010
;//
MessageId    = 0x8010
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_MISSING_COPY_SOURCE_SNAP_NAME
Language     = English
The snapshot name of the copy source is missing.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8011
;//
MessageId    = 0x8011
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_MISSING_MEMBER_LUNS
Language     = English
Missing consistency group member LUNs.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8012
;//
MessageId    = 0x8012
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_OP_DENIED
Language     = English
The specified operation is not allowed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8013
;//
MessageId    = 0x8013
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_NOT_SNAPLUN
Language     = English 
The target LUN is not a snapshot mount point.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create or restore using a snapshot that is involved in a restore
;//                      
;// Generated value should be: 0xE16D8014
;//
MessageId    = 0x8014
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_IN_PROGRESS
Language     = English
Cannot create or restore a snapshot that is currently involved in a restore operation. 
Wait for the current restore to finish then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8015
;//
MessageId    = 0x8015
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_SNAP_VU_NOT_ATTACHED
Language     = English
The specified snapshot is not currently attached to the specified snapshot mount point.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8016
;//
MessageId    = 0x8016
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_DOES_NOT_EXIST
Language     = English 
The specified snapshot source does not exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do an attach/detach/rollback/delete/copy snapshot when
;//                     a rollback object operation(attach/detach/rollback) is in progress for the same snapshot
;//
;// Generated value should be: 0xE16D8017
;//
MessageId    = 0x8017
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_OBJECT_OPERATION_IN_PROGRESS
Language     = English
An attach, detach or restore operation is already in progress for this snapshot. 
Wait for the current operation to finish and retry the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do an operation which is not allowed while snapshot destroy is in progress 
;//
;// Generated value should be: 0xE16D8018
;//
MessageId    = 0x8018
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_IN_PROGRESS
Language     = English 
The operation failed because the specified snapshot is currently being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8019
;//
MessageId    = 0x8019
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_FILE_TYPE_FOR_ROLLBACK
Language     = English
The restore operation failed because the source is a primary LUN. The source must be a snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to unbind LUN with Snap LUs.
;//
;// Generated value should be: 0xE16D801A
;//
MessageId    = 0x801A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSHOT_LU_EXISTS
Language     = English
The LUN cannot be destroyed because it still has snapshot mount points. 
Destroy the snapshot mount points then retry the LUN destroy command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to unbind LUN with Snaps.
;//
;// Generated value should be: 0xE16D801B
;//
MessageId    = 0x801B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSHOTS_EXISTS
Language     = English
The LUN cannot be destroyed because it still has snapshots. 
Destroy the snapshots then retry the LUN destroy command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to unbind attached Snap LUs.
;//
;// Generated value should be: 0xE16D801C
;//
MessageId    = 0x801C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPLU_ATTACHED_ON_DESTROY
Language     = English
The snapshot mount point cannot be destroyed because it is attached. 
Detach the snapshot mount point and retry the destroy operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to detach a VU that was not attached.
;//
;// Generated value should be: 0xE16D801D
;//
MessageId    = 0x801D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_VU_ALREADY_DETACHED
Language     = English
The snapshot mount point cannot be detached because it is already detached.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to attach a VU that was already attached.
;//
;// Generated value should be: 0xE16D801E
;//
MessageId    = 0x801E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_VU_ALREADY_ATTACHED
Language     = English
The snapshot mount point cannot be attached because it is already attached.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D801F
;//
MessageId    = 0x801F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_CG_MEMBERS_EXCEEDED
Language     = English
The maximum number of members already exists in the consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8020
;//
MessageId    = 0x8020
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_CONSISTENCY_GROUPS_EXCEEDED
Language     = English
The maximum number of consistency groups already exists in the system.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8021
;//
MessageId    = 0x8021
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_NAME_EXISTS
Language     = English
The specified consistency group name is already in use.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8022
;//
MessageId    = 0x8022
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_OBJECT_ID_INVALID
Language     = English
The specified consistency group identifier is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8023
;//
MessageId    = 0x8023
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MEMBER_OBJECT_ID_INVALID
Language     = English
The specified consistency group member identifier is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to unbind consistency group with Snaps.
;//
;// Generated value should be: 0xE16D8024
;//
MessageId    = 0x8024
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPS_EXISTS
Language     = English
The consistency group cannot be destroyed because snapshots of it exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to unbind LUN that is a member of a CG.
;//
;// Generated value should be: 0xE16D8025
;//
MessageId    = 0x8025
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_VU_IS_A_CONSISTENCY_GROUP_MEMBER
Language     = English
The LUN cannot be destroyed because it is a member of a consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do an attach or detach on primary LUN 
;//
;// Generated value should be: 0xE16D8026
;//
MessageId    = 0x8026
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_BASELUN_OPERATION_RESTRICTED
Language     = English
An attempt to attach or detach a primary LUN is not allowed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to create a Snapshot of a CG which has no members  
;//
;// Generated value should be: 0xE16D8027
;//
MessageId    = 0x8027
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_CONTAINS_NO_MEMBERS
Language     = English 
Creation of a snapshot of a consistency group which has no members is not allowed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A member is being added to a group which contains 
;//                     other members which are incompatible
;//
;// Generated value should be: 0xE16D8028
;//
MessageId    = 0x8028
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_MEMBER_INCOMPATIBILITIES
Language     = English
The LUN cannot be added to the consistency group because it is incompatible with other LUNs already in group.
.

;// -----------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Snapshot LUN can't be added to a group if it is attached to an individual snapshot 
;//
;// Generated value should be: 0xE16D8029
;//
MessageId    = 0x8029
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SLU_ATTACHED_TO_INDIVIDUAL_SNAPSHOT
Language     = English
The snapshot mount point cannot be added to the consistency group because it is attached to a to a different snapshot than the other members of that consistency group.
Detach the snapshot then add the snapshot mount point to the consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Snapshot LUN can't be added to a group if it is attached to a snapshot which is 
;//                     part of a snapshot which is different from a snapshot already linked to current members
;//                     of the group
;//
;// Generated value should be: 0xE16D802A
;//
MessageId    = 0x802A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SLU_ATTACHED_TO_INVALID_SNAPSHOT
Language     = English
The snapshot mount point cannot be added to the consistency group because it is attached to a to a different snapshot than the other members of that consistency group. 
Detach the snapshot mount point then add it to the consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do a create operation which is not allowed on a snapshot member 
;//
;// Generated value should be: 0xE16D802B
;//
MessageId    = 0x802B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_SNAPSET_MEMBER_RESTRICTED
Language     = English 
The snapshot could not be created because the snapshot source is a snapshot mount point to which a snapshot of a consistency group is attached. Add the snapshot mount point to a consistency group and then create a snapshot of that group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to attach a snapshot to two Snapshot LUs of different CGs  
;//
;// Generated value should be: 0xE16D802C
;//
MessageId    = 0x802C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LU_CG_INVALID
Language     = English
Cannot attach a snapshot to snapshot mount points of different consistency groups.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to attach a snapshot to a Snapshot LUN which is a member of a CG which 
;//                     has other members attached to snapshots which are members of a different snapshot  
;//
;// Generated value should be: 0xE16D802D
;//
MessageId    = 0x802D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSET_INVALID
Language     = English
Cannot attach a snapshot to a snapshot mount point whose consistency group contains members attached to other snapshots.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Return if the File (snapshot) is unattachable.
;//
;// Generated value should be: 0xE16D802E
;//
MessageId    = 0x802E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAP_UNATTACHABLE
Language     = English
The snapshot cannot be attached while its Allow Read/Write property is disabled. Enable Allow Read/Write for the snapshot and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D802F
;//
MessageId    = 0x802F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_COULD_NOT_PROMOTE_REPLICA
Language     = English
An internal error occurred. Gather SP collects and contact your service provider. 
Internal Information Only. Error %2 while promoting %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8030
;//
MessageId    = 0x8030
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_COULD_NOT_DEMOTE_REPLICA
Language     = English
An internal error occurred. Gather SP collects and contact your service provider.
Internal Information Only. Error %2 while demoting %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to detach a snapshot that was not attached.
;//
;// Generated value should be: 0xE16D8031
;//
MessageId    = 0x8031
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FILE_NOT_ATTACHED
Language     = English
The snapshot is not attached.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: snapshot LUN cannot be created. 
;//
;// Generated value should be: 0xE16D8032
;//
MessageId    = 0x8032
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_BASELU_FOR_SNAPLU_CREATION
Language     = English
A snapshot mount point cannot be created using a snapshot mount point as the primary LUN. 
Retry the operation with the correct LUN type.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to UNISPHERE
;// 
;// Severity: Error
;//
;// Symptom of Problem: VNX Snaps enabler not installed.
;//
;// Generated value should be: 0xE16D8033
;//
MessageId    = 0x8033
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_NOT_ENABLED
Language     = English
VNX Snapshots feature software is not enabled. Please install the VNX Snapshots feature enabler.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The source of a create or copy snapshot command is not ready.
;//
;// Generated value should be: 0xE16D8034
;//
MessageId    = 0x8034
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_NOT_READY
Language     = English
The snapshot cannot be created because the snapshot source is not ready. 
Wait for the source to be ready then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The source of the snapshot creation is in the midst of a create.
;//
;// Generated value should be: 0xE16D8035
;//
MessageId    = 0x8035
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATE_IN_PROGRESS
Language     = English
The source of the snapshot create or copy operation already has a create or copy in progress.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: One of the snapshot Members is not in ready state for snapshot restore operation.  
;//
;// Generated value should be: 0xE16D8036
;//
MessageId    = 0x8036
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSET_MEMBER_NOT_READY_FOR_ROLLBACK
Language     = English
The snapshot is not ready for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: snapshot is not in ready state for snapshot restore operation.  
;//
;// Generated value should be: 0xE16D8037
;//
MessageId    = 0x8037
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSET_NOT_READY_FOR_ROLLBACK
Language     = English
The snapshot is not ready for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid destination specified for restore operation.  
;//
;// Generated value should be: 0xE16D8038
;//
MessageId    = 0x8038
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_DESTINATION_FOR_ROLLBACK
Language     = English
Invalid destination specified for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: One of the CG Members is not in ready state for restore operation.  
;//
;// Generated value should be: 0xE16D8039
;//
MessageId    = 0x8039
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_MEMBER_NOT_READY_FOR_ROLLBACK
Language     = English
One of the consistency group members is not ready for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: CG is not in ready state for restore operation.  
;//
;// Generated value should be: 0xE16D803A
;//
MessageId    = 0x803A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_NOT_READY_FOR_ROLLBACK
Language     = English
The consistency group is not ready for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt is made to set the expiration date and harvest priority of a Snapshot together. 
;//                     At any given time, either the Expiration Date or the Harvest Priority can be set for a Snapshot, 
;//                     but not both.  
;//
;// Generated value should be: 0xE16D803B
;//
MessageId    = 0x803B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_HARVESTING_AND_EXPIRATION_ARE_MUTUALLY_EXCLUSIVE
Language     = English
Snapshot cannot have both expiration date and auto delete enabled at the same time.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt is made to set the Snapshot expiration time to a time in the past 
;//                     or more than 100 years in the future.    
;//
;// Generated value should be: 0xE16D803C
;//
MessageId    = 0x803C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_EXPIRATION_DATE
Language     = English
Snapshot expiration cannot be set to a time in the past or more than 100 years into the future.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt is made to either create a Snapshot or do a restore 
;//                     where the source Snapshot/snapshot is obsolete.
;//
;// Generated value should be: 0xE16D803D
;//
MessageId    = 0x803D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_OBSOLETE
Language     = English
Snapshot could not be created or restored because the source snapshot has expired.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Force Flag is required for restore operation.  
;//
;// Generated value should be: 0xE16D803E
;//
MessageId    = 0x803E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FORCE_FLAG_REQUIRED_FOR_SNAPSET_ROLLBACK
Language     = English
Cannot restore the snapshot to its consistency group because the consistency group no longer has the same member LUNs that it had when the snapshot was created. Modify the consistency group's membership by adding or removing LUNs until it matches the snapshot's list of 'Source LUNs' and then retry the restore operation. It is advisable to take another snapshot of the consistency group before modifying its membership. Similarly, note that any LUNs that need to be added back to the consistency group will have their contents overwritten by the restore operation, so take snapshots of them beforehand if they contain any useful data.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid source specified for restore operation.  
;//
;// Generated value should be: 0xE16D803F
;//
MessageId    = 0x803F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_SOURCE_FOR_ROLLBACK
Language     = English
Invalid source specified for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Validation for CG membership failed for snapshot restore operation.  
;//
;// Generated value should be: 0xE16D8040
;//
MessageId    = 0x8040
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FAILED_TO_VALIDATE_CG_MEMBERSHIP_FOR_ROLLBACK
Language     = English
Validation for consistency group membership failed for snapshot restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For User Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snaps are being invalidated due to space issues
;//
;// Generated value should be: 0xE16D8041
;//
MessageId    = 0x8041
;//SSPG C4type=User
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INVALIDATED_SNAP
Language     = English
Snapshot %2 has been automatically destroyed due to insufficient pool space.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt was made to restore a snapshot that is attached.
;//
;// Generated value should be: 0xE16D8042
;//
MessageId    = 0x8042
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACHED_CANNOT_RESTORE
Language     = English
Specified snapshot is attached and cannot be used in a restore operation. 
Detach the snapshot and retry the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt was made to set the properties of a snapshot member.
;//
;// Generated value should be: 0xE16D8043
;//
MessageId    = 0x8043
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CANNOT_SET_PROPERTIES_FOR_SNAPSET_MEMBER
Language     = English
The operation attempted to set properties on an invalid snapshot. Retry the operation with a valid snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to attach, detach or restore a snapshot failed because the system is already 
;//                     processing the maximum allowed number of such operations.
;//
;// Generated value should be: 0xE16D8044
;//
MessageId    = 0x8044
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAXIMUM_OPERATION_COUNT_EXCEEDED
Language     = English
An attempt to attach, detach or restore a snapshot failed because the system is already processing the maximum allowed number of such operations. 
Wait for some to finish and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to add a LUN to a CG that is already a member of that CG
;//                         
;// Generated value should be: 0xE16D8045
;//
MessageId    = 0x8045
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MEMBER_ALREADY_EXISTS_IN_CG
Language     = English
The specified LUN is already a member of another consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An remove a LUN from a CG that it is not a member of.
;//                         
;// Generated value should be: 0xE16D8046
;//
MessageId    = 0x8046
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LUN_NOT_A_MEMBER_OF_THIS_CG
Language     = English
The specified LUN is not a member of the specified consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to restore a snapshot failed as the snapshot is not valid.
;//                         
;// Generated value should be: 0xE16D8047
;//
MessageId    = 0x8047
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_SNAPSHOT_FOR_ROLLBACK
Language     = English
A snapshot specified in the restore operation has is being destroyed due to insufficient pool space and cannot be used for the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log only
;//
;// Severity: Error
;//
;// Symptom of Problem: Rollback Object used to perform restore operation cannot proceed 
;//                     as the source snapshot has been invalidated.
;//
;// Generated value should be: 0xE16D8048
;//
MessageId    = 0x8048
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_FAILED_DUE_TO_INVALIDATED_SNAPSHOT
Language     = English
The restore failed because the source snapshot is being destroyed due to insufficient pool space.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to create Snapshot LUN cannot proceed because Compression is enabled on 
;//                     the primary LUN from which Snapshot LUN creation is attempted.
;//
;// Generated value should be: 0xE16D8049
;//
MessageId    = 0x8049
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LUN_CREATION_COMPRESSION_ENABLED
Language     = English
Snapshot mount point creation cannot proceed because compression is enabled on the primary LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt was made to attach a SLU that is a CG member to an individual snap 
;//                     (snap that is not a snapshot member)
;//
;// Generated value should be: 0xE16D804A
;//
MessageId    = 0x804A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_COULD_NOT_ATTACH_SNAP_TO_CG_SLU
Language     = English
The specified snapshot cannot be attached to a snapshot mount point in a consistency group. 
Retry the operation with a snapshot that is valid for the consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The Snapshot restore operation cannot be performed because RecoverPoint Splitter 
;//                     notification failed.
;//
;// Generated value should be: 0xE16D804B
;//
MessageId    = 0x804B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SPLITTER_START_NOTIFICATION_FAILED 
Language     = English
An internal error occurred. Gather SP collects and contact your service provider. 
The Snapshot Restore operation cannot be performed because RecoverPoint Splitter notification failed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The Snapshot operation failed trying to notify the RecoverPoint Splitter that 
;//                     the operation completed.
;//
;// Generated value should be: 0xE16D804C
;//
MessageId    = 0x804C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SPLITTER_COMPLETE_NOTIFICATION_FAILED 
Language     = English
An internal error occurred. Gather SP collects and contact your service provider. 
The Snapshot Restore operation failed trying to notify the RecoverPoint Splitter that the operation completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D804D
;//
MessageId    = 0x804D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_UNKNOWN_VOLUME
Language     = English
The specified volume WWID was not recognized by the SnapBack driver.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D804E
;//
MessageId    = 0x804E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_OPERATION_ALREADY_ACTIVE_ON_VOLUME
Language     = English
A snapback operation was already active for the specified volume.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D804F
;//
MessageId    = 0x804F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_OPERATION_NOT_ACTIVE_ON_VOLUME
Language     = English
No snapback operation was active for the specified volume.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D8050
;//
MessageId    = 0x8050
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_INVALID_INFO_FROM_DESCRIBE
Language     = English
The size of the information returned by IOCTL_FLARE_DESCRIBE_EXTENTS was invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D8051
;//
MessageId    = 0x8051
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_INVALID_INFO_FROM_NOTIFY
Language     = English
The information size returned by FLARE_NOTIFY_EXTENT_CHANGE was invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because source of restore is not in ready state.
;//
;// Generated value should be: 0xE16D8052
;//
MessageId    = 0x8052
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_NOT_READY_FOR_ROLLBACK
Language     = English
The restore operation failed because it timed out. Retry the operation. If the problem persists, gather SPcollects and contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because destination of restore is not in ready state.
;//
;// Generated value should be: 0xE16D8053
;//
MessageId    = 0x8053
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTINATION_NOT_READY_FOR_ROLLBACK
Language     = English
The restore operation cannot be performed because the destination of the restore is not in the ready state. 
Wait for the destination to become ready and retry the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8054
;//
MessageId    = 0x8054
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_FILE_NOT_READY
Language     = English
The snapshot mount point cannot be attached because the snapshot is not in the ready state. 
Wait for the snapshot to become ready and retry the attach operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//                                                                     
;// Severity: Error
;//
;// Symptom of Problem: A request to do an attach which is not allowed when the snapshot is 
;//                     attached to a snapshot LUN
;//
;// Generated value should be: 0xE16D8055
;//
MessageId    = 0x8055
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_OPERATION_RESTRICTED_SNAP_IS_ATTACHED
Language     = English
The attach operation cannot proceed because the snapshot is attached to a snapshot mount point. 
Attach to another snapshot mount point or detach what is currently attached and then attach.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request a Snapshot creation while expand/shrink operation is in progress for the Source LUN
;//
;// Generated value should be: 0xE16D8056
;//
MessageId    = 0x8056
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_LUN_EXPANDING_OR_SHRINKING
Language     = English
Snapshot creation is not allowed while the source LUN is involved in Expand or Shrink Operation. 
Wait for expand or shrink to finish then create the snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request a Snapshot creation that is not allowed on a LUN which is member of a CG
;//
;// Generated value should be: 0xE16D8057
;//
MessageId    = 0x8057
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_LUN_IS_MEMBER_OF_CG
Language     = English
Snapshot creation is not allowed because the source LUN belongs to a consistency group. 
You must create snapshot on the entire consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//                                                                     
;// Severity: Error
;//
;// Symptom of Problem: A request to find a non existent snapshot
;//
;// Generated value should be: 0xE16D8058
;//
MessageId    = 0x8058
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPMGMT_SNAPSHOT_DOES_NOT_EXIST
Language     = English
The specified snapshot does not exist. Operation must be for a snapshot that exists.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D8059
;//
MessageId    = 0x8059
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_INVALID_VERSION
Language     = English
The specified version of the SnapBack driver is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D805A
;//
MessageId    = 0x805A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_HANDSHAKE_ALREADY_PERFORMED
Language     = English
The handshake with the SnapBack driver has already been performed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned from SnapBack to MLU
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Generated value should be: 0xE16D805B
;//
MessageId    = 0x805B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = SNAPBACK_ERROR_VOLUME_NOT_OPEN
Language     = English
The specified volume is not open.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//                                                                     
;// Severity: Error
;//
;// Symptom of Problem: Request a snapshot creation when compression is enabled on the source LUN
;//
;// Generated value should be: 0xE16D805C
;//
MessageId    = 0x805C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_COMPRESSION_ENABLED
Language     = English
Snapshot creation cannot proceed since compression is enabled on the primary LUN. 
Disable compression and wait for decompression to be completed before initiating the operation again.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//                                                                     
;// Severity: Error
;//
;// Symptom of Problem: Request to add a LUN as a consistency group member when compression is enabled on the LUN
;//
;// Generated value should be: 0xE16D805D
;//
MessageId    = 0x805D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_TO_CG_RESTRICTED_COMPRESSION_ENABLED
Language     = English
The LUN cannot be added to a consistency group because it has compression enabled.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request a Snapshot restore while expand/shrink operation is in progress for the destination LUN
;//
;// Generated value should be: 0xE16D805E
;//
MessageId    = 0x805E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_RESTRICTED_DESTINATION_LUN_EXPANDING_OR_SHRINKING
Language     = English
Snapshot restore is not allowed while the destination LUN is involved in an expand or shrink operation. 
Wait for the expand or shrink to finish then perform the restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to add a LUN to a consistency group when the LUN is involved in a restore operation
;//                         
;// Generated value should be: 0xE16D805F
;//
MessageId    = 0x805F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_TO_CG_RESTRICTED_LUN_RESTORE_IN_PROGRESS
Language     = English
The LUN cannot be added to a consistency group because a restore operation on the LUN is still in progress. 
Wait for the restore to finish then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to remove a LUN from consistency group when the LUN is involved in a restore operation
;//                         
;// Generated value should be: 0xE16D8060
;//
MessageId    = 0x8060
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REMOVE_FROM_CG_RESTRICTED_LUN_RESTORE_IN_PROGRESS
Language     = English
The LUN cannot be removed from a consistency group because a restore operation on the LUN is still in progress. 
Wait for the restore to finish then remove the lun from the consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: expand / shrink operation is not allowed on a thick LUN with Snapshots
;//
;// Generated value should be: 0xE16D8061
;//
MessageId    = 0x8061
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_EXPAND_SHRINK_OF_BASE_DLU_WITH_SNAPSHOTS_RESTRICTED
Language     = English
The LUN cannot be expanded or shrunk because it is a thick LUN with snapshots.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to create a snapshot LUN when the primary LUN is not in ready state.
;//                         
;// Generated value should be: 0xE16D8062
;//
MessageId    = 0x8062
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_NOT_READY
Language     = English
The snapshot mount point cannot be created because the primary LUN is not ready. 
Wait for the primary LUN to be ready and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to create a Snapshot of a CG which has unattached members.
;//
;// Generated value should be: 0xE16D8063
;//
MessageId    = 0x8063
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_MEMBER_IS_UNATTACHED
Language     = English 
The snapshot of the consistency group cannot be created because some members of the group are unattached snapshot mount points. 
Attach all members of the consistency group and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A snapshot LUN cannot be expanded since it is associated with primary LUN of type 
;//                     thick LUN which currently does not support expand operation. 
;//
;// Generated value should be: 0xE16D8064
;//
MessageId    = 0x8064
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_BASELU_FOR_SNAPLU_EXPAND
Language     = English
The LUN cannot be expanded because it is a snapshot mount point associated with a thick LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: A snapshot LUN cannot cannot be shrinked since it is associated with primary LUN of type 
;//                     thick LUN which currently does not support shrink operation. 
;//
;// Generated value should be: 0xE16D8065
;//
MessageId    = 0x8065
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INVALID_BASELU_FOR_SNAPLU_SHRINK
Language     = English
The LUN cannot be shrunk because it is a snapshot mount point associated with a thick LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request a snapshot detach while expand/shrink operation is in progress for the snapshot LUN
;//
;// Generated value should be: 0xE16D8066
;//
MessageId    = 0x8066
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DETACH_RESTRICTED_LUN_EXPANDING_OR_SHRINKING
Language     = English
The snapshot mount point cannot be detached until its expand or shrink operation has completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to add LUN to a consistency group since it already has snapshot of another 
;//                     consistency group associated with it.
;//
;// Generated value should be: 0xE16D8067
;//
MessageId    = 0x8067
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_MEMBER_ADD_ALREADY_ASSOCIATED_WITH_SNAPSHOT_OF_CG
Language     = English
The LUN cannot be added to the consistency group because snapshots created when the LUN was a member of a different consistency group still exist. 
Destroy the snapshots that were created while the LUN was a member of a different consistency group and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The Snapshot restore operation is not supported on this RecoverPoint LUN.
;//                     The LUN may be either a secondary or a primary whose replication mode does not support restore operation.
;//
;// Generated value should be: 0xE16D8068
;//
MessageId    = 0x8068
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_RESTRICTED_ON_SPLITTER_LU 
Language     = English
The snapshot cannot be restored because the specified LUN is either a RecoverPoint secondary image or a RecoverPoint primary image with synchronous replication enabled.
Retry the operation using a RecoverPoint primary image with synchronous replication disabled.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to add or replace a consistency group member while the member or the group is in destroying state.
;//                         
;// Generated value should be: 0xE16D8069
;//
MessageId    = 0x8069
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_ADD_REPLACE_MEMBER_RESTRICTED_DESTROY_IN_PROGRESS
Language     = English
Adding or replacing LUNs in a consistency group is not allowed if the group or any of the LUNs are being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to attach a Snapshot to a Snapshot LUN while the copy operation is in 
;//                     progress for the Snapshot. The attach operation cannot proceed since the  
;//                     snapshot is currently being copied.
;//                         
;// Generated value should be: 0xE16D806A
;//
MessageId    = 0x806A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_RESTRICTED_COPY_IN_PROGRESS
Language     = English
The attach operation failed because the snapshot is being copied. Retry the operation after the copy has completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to destroy a Snapshot while the copy operation is in progress for the Snapshot. 
;//                     The destroy operation cannot proceed since the snapshot is currently being copied.
;//                         
;// Generated value should be: 0xE16D806B
;//
MessageId    = 0x806B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_RESTRICTED_COPY_IN_PROGRESS
Language     = English
The destroy operation failed because the snapshot is being copied. Retry the operation after the copy has completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to restore a Snapshot to a LUN while the copy operation is in 
;//                     progress for the Snapshot. The restore operation cannot proceed since the  
;//                     snapshot is currently being copied.
;//                         
;// Generated value should be: 0xE16D806C
;//
MessageId    = 0x806C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_RESTRICTED_COPY_IN_PROGRESS
Language     = English
The restore operation failed because the snapshot is being copied. Retry the operation after the copy has completed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: User sent a copy snapshot command via snapcli to a LUN that is not the source of the snapshot.
;//                     To ensure snapshots are not copied via other LUNs this error and check are added. 
;//
;// Generated value should be: 0xE16D806D
;//
MessageId    = 0x806D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_COPY_NOT_TO_SOURCE
Language     = English
The copy command must be sent to the LUN that is the source of the snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem: User sent a destroy snapshot command via snapcli to a LUN that is not the source of the snapshot.
;//                     To ensure snapshots are not destroyed via other LUNs this error and check are added. 
;//
;// Generated value should be: 0xE16D806E
;//
MessageId    = 0x806E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_DESTROY_NOT_TO_SOURCE
Language     = English
The destroy command must be sent to the LUN that is the source of the snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to modify AllowReadWrite property of an attached snapshot. The AllowReadWrite property 
;//                     can not be modified since the snap is already attached to a snap mount point.                              
;//                         
;// Generated value should be: 0xE16D806F
;//
MessageId    = 0x806F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MODIFY_ALLOWREADWRITE_PROPERTY_RESTRICTED_SNAP_IS_ATTACHED
Language     = English
The modify allowreadwrite property operation failed because the snapshot is attached to a snapshot mount point. Retry the operation after detaching the snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS File_not_found error while creating a snapshot/snapset. Destroy the snapshot/snapset and try again.                            
;//                         
;// Generated value should be: 0xE16D8070
;//
MessageId    = 0x8070
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CBFS_FILE_NOT_FOUND
Language     = English
An internal error occurred while creating %2 %3. Retry the operation after destroying the %4.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to attach a Snapshot to a LUN when the LUN is either in the process of being deleted or offline.
;//                         
;// Generated value should be: 0xE16D8071
;//
MessageId    = 0x8071
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_NOT_READY
Language     = English
The attach operation could not complete because the LUN is either in the process of being deleted or offline. Retry the operation once the LUN is ready.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A member is being added to a group which contains 
;//                     other members associated with the same Base LUN.
;//
;// Generated value should be: 0xE16D8072
;//
MessageId    = 0x8072
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_SNAPLU_TO_CG_WITH_SNAPLU_WITH_SAME_BASELU
Language     = English
The snapshot mount point cannot be added to the consistency group because the consistency group contains (or would contain) another snapshot mount point associated with the same primary LUN. A consistency group may not contain two (or more) different snapshot mount points associated with the same primary LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Snap LU is being added to a group which contains Base LUs
;//
;// Generated value should be: 0xE16D8073
;//
MessageId    = 0x8073
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_DOES_NOT_ALLOW_SNAPLU_MEMBERS
Language     = English
The snapshot mount point cannot be added to the consistency group because the consistency group contains (or would contain) a primary LUN. Consistency groups may contain only primary LUNs or only snapshot mount points, but not both.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Snap LU is being added to a group which contains Base LUs
;//
;// Generated value should be: 0xE16D8074
;//
MessageId    = 0x8074
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_DOES_NOT_ALLOW_BASELU_MEMBERS
Language     = English
The primary LUN cannot be added to the consistency group because the consistency group contains (or would contain) a snapshot mount point. Consistency groups may contain only primary LUNs or only snapshot mount points, but not both.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do a destroy operation which is not allowed on a snapshot member 
;//
;// Generated value should be: 0xE16D8075
;//
MessageId    = 0x8075
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DESTROY_SNAPSET_MEMBER_RESTRICTED
Language     = English 
The snapshot specified to be destroyed does not exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do a restore operation which is not allowed on a snapshot member 
;//
;// Generated value should be: 0xE16D8076
;//
MessageId    = 0x8076
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_SNAPSET_MEMBER_RESTRICTED
Language     = English
The snapshot specified to be restored does not exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do an attach operation which is not allowed on a snapshot member 
;//
;// Generated value should be: 0xE16D8077
;//
MessageId    = 0x8077
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_SNAPSET_MEMBER_RESTRICTED
Language     = English 
The snapshot specified for the attach operation does not exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to do a detach operation which is not allowed on a snapshot member 
;//
;// Generated value should be: 0xE16D8078
;//
MessageId    = 0x8078
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DETACH_SNAPSET_MEMBER_RESTRICTED
Language     = English 
The snapshot specified for the detach operation does not exist.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to create more than the maximum number of snapshots. 
;//
;// Generated value should be: 0xE16D8079
;//
MessageId    = 0x8079
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_SNAPS_PER_FAMILY_EXCEEDED
Language     = English
The maximum number of snapshots associated with the primary LUN have already been created on this array.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to restore snapshot to unattached snapshot mount point. 
;//
;// Generated value should be: 0xE16D807A
;//
MessageId    = 0x807A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_DESTINATION_IS_UNATTACHED
Language     = English
An unattached snapshot mount point cannot be used as destination LUN for restore operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request received to restore snapset to CG which has unattached snapshot mount point. 
;//
;// Generated value should be: 0xE16D807B
;//
MessageId    = 0x807B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_CG_MEMBER_IS_UNATTACHED 
Language     = English
The restore of the snapshot to its consistency group cannot proceed because some members of the group are unattached snapshot mount points.Attach all members of the consistency group and retry the operation.
.

;// -------------------------------------------------- 
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a snapshot when the source file is being initialized.
;//
;// Generated value should be: 0xE16D807C
;//
;// Replaces MLU_SNAPSHOT_FILE_NOT_READY
;//
MessageId    = 0x807C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FILE_INITIALIZING
Language     = English
The snapshot cannot be created because the snapshot source is being initialized. Wait until the snapshot source is ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a snapshot when the source file is offline.
;//
;// Generated value should be: 0xE16D807D
;//
;// Replaces MLU_SNAPSHOT_FILE_NOT_READY
;//
MessageId    = 0x807D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FILE_OFFLINE
Language     = English
The snapshot cannot be created because the source is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a snapshot when the source file is being destroyed.
;//
;// Generated value should be: 0xE16D807E
;//
;// Replaces MLU_SNAPSHOT_FILE_NOT_READY
;//
MessageId    = 0x807E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_FILE_DESTROYING
Language     = English
The snapshot cannot be created because the source is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The source of a create or copy snapshot command is being initialized.
;//
;// Generated value should be: 0xE16D807F
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY 
;//
MessageId    = 0x807F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_INITIALIZING
Language     = English
The snapshot cannot be created because the snapshot source is being initialized. Wait until the snapshot source is ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The source of a create or copy snapshot command is offline.
;//
;// Generated value should be: 0xE16D8080
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY 
;//
MessageId    = 0x8080
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_OFFLINE
Language     = English
The snapshot cannot be created because the snapshot source is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The source of a create or copy snapshot command is being destroyed.
;//
;// Generated value should be: 0xE16D8081
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY 
;//
MessageId    = 0x8081
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SOURCE_DESTROYING
Language     = English
The snapshot cannot be created because the snapshot source is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because one of the snapset members is being initialized.
;//
;// Generated value should be: 0xE16D8082
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_MEMBER_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8082
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_MEMBER_INITIALIZING
Language     = English
The snapshot cannot be restored because it is being initialized. Wait until the snapshot is ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because one of the snapset members is offline.
;//
;//
;// Generated value should be: 0xE16D8083
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_MEMBER_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8083
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_MEMBER_OFFLINE
Language     = English
The snapshot cannot be restored because it is offline. Resolve any hardware problems and retry the operation.
.

;// -------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because one of the snapset members is being destroyed.
;//
;// Generated value should be: 0xE16D8084
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_MEMBER_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8084
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_MEMBER_DESTROYING
Language     = English
The snapshot cannot be restored because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because the snapset is being initialized. 
;//
;// Generated value should be: 0xE16D8085
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8085
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_INITIALIZING
Language     = English
The snapshot cannot be restored because it is being initialized. Wait until the snapshot is ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because the snapset is offline. 
;//
;// Generated value should be: 0xE16D8086
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8086
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_OFFLINE
Language     = English
The snapshot cannot be restored because it is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The snapshot cannot be restored because the snapset is being destroyed. 
;//
;// Generated value should be: 0xE16D8087
;//
;// Replaces MLU_SNAPSHOT_SNAPSET_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8087
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAPSET_DESTROYING
Language     = English
The snapshot cannot be restored because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because one of its members is being initialized.
;//
;// Generated value should be: 0xE16D8088
;//
;// Replaces MLU_SNAPSHOT_CG_MEMBER_NOT_READY_FOR_ROLLBACK
MessageId    = 0x8088
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_MEMBER_INITIALIZING
Language     = English
The consistency group cannot be restored because one of its members is being initialized. Wait until all the members are ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because one of its members is offline.
;//
;// Generated value should be: 0xE16D8089
;//
MessageId    = 0x8089
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_MEMBER_OFFLINE
Language     = English
The consistency group cannot be restored because one of its members is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because one of its members is being destroyed.
;//
;// Generated value should be: 0xE16D808A
;//
MessageId    = 0x808A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_MEMBER_DESTROYING
Language     = English
The consistency group cannot be restored because one of its members is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because the consistency group is being initialized.
;//
;// Generated value should be: 0xE16D808B
;//
;// Replaces MLU_SNAPSHOT_CG_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x808B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_INITIALIZING
Language     = English
The consistency group cannot be restored because it being initialized. Wait until the consistency group is ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because the consistency group is offline.
;//
;// Generated value should be: 0xE16D808C
;//
;// Replaces MLU_SNAPSHOT_CG_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x808C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_OFFLINE
Language     = English
The consistency group cannot be restored because it is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The consistency group cannot be restored because the consistency group is being destroyed.
;//
;// Generated value should be: 0xE16D808D
;//
;// Replaces MLU_SNAPSHOT_CG_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x808D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_CG_DESTROYING
Language     = English
The consistency group cannot be restored because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because source of the data to be restored is initializing.
;//
;// Generated value should be: 0xE16D808E
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x808E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SOURCE_INITIALIZING
Language     = English
The restore operation cannot be performed because the source of the data to be restored is initalizing. Wait for the source to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because source of the data to be restored is offline.
;//
;// Generated value should be: 0xE16D808F
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x808F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SOURCE_OFFLINE
Language     = English
The restore operation cannot be performed because the source of the data to be restored is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because source of the data to be restored being destroyed.
;//
;// Generated value should be: 0xE16D8090
;//
;// Replaces MLU_SNAPSHOT_SOURCE_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8090
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SOURCE_DESTROYING
Language     = English
The restore operation cannot be performed because the source of the data to be restored is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination of the restore operation is being initialized.
;//
;// Generated value should be: 0xE16D8091
;//
;// Repalces MLU_SNAPSHOT_DESTINATION_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8091
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_DESTINATION_INITIALIZING
Language     = English
The restore operation cannot be performed because because at least one of the LUN(s) and/or snapshot mount point(s) to which data is to be restored is being initialized. Wait for the destination to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination of the restore operation is offline.
;//
;// Generated value should be: 0xE16D8092
;//
;// Repalces MLU_SNAPSHOT_DESTINATION_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8092
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_DESTINATION_OFFLINE
Language     = English
The restore operation cannot be performed because because at least one of the LUN(s) and/or snapshot mount point(s) to which data is to be restored is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination of the restore operation is being destroyed.
;//
;// Generated value should be: 0xE16D8093
;//
;// Repalces MLU_SNAPSHOT_DESTINATION_NOT_READY_FOR_ROLLBACK
;//
MessageId    = 0x8093
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_DESTINATION_DESTROYING
Language     = English
The restore operation cannot be performed because because at least one of the LUN(s) and/or snapshot mount point(s) to which data is to be restored is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8094
;//
;// Replaces MLU_SNAPSHOT_ATTACH_FILE_NOT_READY
;//
MessageId    = 0x8094
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_FILE_INITIALIZING
Language     = English
The snapshot cannot be attached because it is being initialized. Wait for the snapshot to become ready and retry the attach operation.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8095
;//
;// Replaces MLU_SNAPSHOT_ATTACH_FILE_NOT_READY
;//
MessageId    = 0x8095
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_FILE_OFFLINE
Language     = English
The snapshot cannot be attached because it is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Generated value should be: 0xE16D8096
;//
;// Replaces MLU_SNAPSHOT_ATTACH_FILE_NOT_READY
;//
MessageId    = 0x8096
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_FILE_DESTROYING
Language     = English
The snapshot cannot be attached because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to create a snapshot LUN when the primary LUN is being initialized.
;//                         
;// Generated value should be: 0xE16D8097
;//
;// Replaces MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_NOT_READY
;//
MessageId    = 0x8097
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_INITIALIZING
Language     = English
The snapshot mount point cannot be created because the primary LUN is being initialized. Wait for the primary LUN to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to create a snapshot LUN when the primary LUN is offline.
;//                         
;// Generated value should be: 0xE16D8098
;//
;// Replaces MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_NOT_READY
;//
MessageId    = 0x8098
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_OFFLINE
Language     = English
The snapshot mount point cannot be created because the primary LUN is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to create a snapshot LUN when the primary LUN is being destroyed.
;//                         
;// Generated value should be: 0xE16D8099
;//
;// Replaces MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_NOT_READY
;//
MessageId    = 0x8099
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LU_CREATION_RESTRICTED_BASELU_DESTROYING
Language     = English
The snapshot mount point cannot be created because the primary LUN is being destroyed. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to attach a Snapshot to a LUN when the LUN is being initialized.
;//
;// Generated value should be: 0xE16D809A
;//
;// Replaces MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_NOT_READY
;//
MessageId    = 0x809A
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_INITIALIZING
Language     = English
The attach operation could not complete because the snapshot mount point is still being initialized. Wait for the snapshot mount point to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to attach a Snapshot to a LUN when the LUN is offine.
;//
;// Generated value should be: 0xE16D809B
;//
;// Replaces MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_NOT_READY
;//
MessageId    = 0x809B
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_OFFLINE
Language     = English
The attach operation could not complete because the snapshot mount point is offline. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to attach a Snapshot to a LUN when the LUN is being deleted.
;//
;// Generated value should be: 0xE16D809C
;//
;// Replaces MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_NOT_READY
;//
MessageId    = 0x809C
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_RESTRICTED_LUN_DESTROYING
Language     = English
The attach operation could not complete because the snapshot mount point is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot could not be created because the source FileObj is not in public Ready state.
;//
;// Generated value should be: 0xE16D809D
;//
MessageId    = 0x809D
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_SOURCE_FILE_NOT_PUBLIC_READY
Language     = English
The snapshot creation failed because it timed out. Retry the operation. If the problem persists, gather SPcollects and contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot could not be created because the source FSObj is not in public Ready state.
;//
;// Generated value should be: 0xE16D809E
;//
MessageId    = 0x809E
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_SOURCE_FS_NOT_PUBLIC_READY
Language     = English
The snapshot creation failed because it timed out. Retry the operation. If the problem persists, gather SPcollects and contact your service provider.
.

;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: SnapSet copy operation rejected because user specified NoGroup option.
;//
;// Generated value should be: 0xE16D809F
;//
MessageId    = 0x809F
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_NOGROUP_OPTION_WITH_ILLEGAL_SOURCE
Language     = English
The snapshot creation failed. The 'no group' option is not compatible with the specified source.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot creation rejected because snap name would be too long.
;//
;// Generated value should be: 0xE16D80A0
;//
MessageId    = 0x80A0
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_BUILT_NAME_TOO_LONG
Language     = English
The snapshot creation failed because the resulting name would be too long. Retry with a shorter name.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot restore failed because snapback operation failed.
;//
;// Generated value should be: 0xE16D80A1
;//
MessageId    = 0x80A1
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_FAILED_BECAUSE_SNAPBACK_FAILED
Language     = English
An internal error occurred. Gather SP collects and contact your service provider.
Internal Information Only. Snapshot restore operation %2 for VU object %3 failed due to error %4 during the notification phase.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot set and snapshot mount points do not belong to the same family. 
;//
;// Generated value should be: 0xE16D80A2
;//
MessageId    = 0x80A2
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SNAPSET_MEMBER_NOT_IN_FAMILY
Language     = English
The operation failed because the snapshot and the snapshot mount points are associated with different primary LUNs. Retry the operation with a snapshot mount point that has the same primary LUN as the snapshot.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while a snapshot
;//                     attach is in progress. 
;//
;// Generated value should be: 0xE16D80A3
;//
MessageId    = 0x80A3
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ATTACH_IN_PROGRESS
Language     = English
The operation failed because the snapshot is still in the process of being attached.
Wait for the current operation to finish and retry the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while a snapshot
;//                     detach is in progress.
;//
;// Generated value should be: 0xE16D80A4
;//
MessageId    = 0x80A4
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DETACH_IN_PROGRESS
Language     = English
The operation failed because the snapshot is still in the process of being detached. 
Wait for the current operation to finish and retry the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while a snapshot
;//                     restore is in progress for the Restore Point Snap, 
;//                     Backup Snap or the Attached Snap. 
;//
;// Generated value should be: 0xE16D80A5
;//
MessageId    = 0x80A5
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_IN_PROGRESS
Language     = English
The operation failed because a snapshot restore operation is in progress.
Wait for the current operation to finish and retry the command.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while a snapshot mount point
;//                     attach is in progress. 
;//
;// Generated value should be: 0xE16D80A6
;//
MessageId    = 0x80A6
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MOUNT_POINT_ATTACH_IN_PROGRESS
Language     = English
The operation failed because the snapshot mount point is in the process of attaching a snapshot. 
Wait for the current operation to finish and retry the command.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while a snapshot mount point
;//                     detach is in progress. 
;//
;// Generated value should be: 0xE16D80A7
;//
MessageId    = 0x80A7
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MOUNT_POINT_DETACH_IN_PROGRESS
Language     = English
The operation failed because the snapshot mount point is in the process of detaching a snapshot. 
Wait for the current operation to finish and retry the command.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Requesting an operation which is not allowed while
;//                     restore is in progress for the VU 
;//
;// Generated value should be: 0xE16D80A8
;//
MessageId    = 0x80A8
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_VU_RESTORE_IN_PROGRESS
Language     = English
The operation failed because the contents of a snapshot are being restored to the LUN. 
Wait for the current operation to finish and retry the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem:  Attempt to remove LUNs from a consistency group which are not members of that consistency group.
;//
;// Generated value should be: 0xE16D80A9
;//
MessageId    = 0x80A9
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CG_ERROR_MEMBER_NOT_IN_CG
Language     = English
The specified LUNs and/or snapshot mount points cannot be removed from the specified consistency group because not all of them are members of that consistency group.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to snapcli
;//
;// Severity: Error
;//
;// Symptom of Problem:  Attempt to perform an InBand attach of a snapshot to an SMP whose "allowInBandSnapAttach" property is not enabled.
;//
;// Generated value should be: 0xE16D80AA
;//
MessageId    = 0x80AA
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_HOST_INITIATED_ATTACH_DISALLOWED
Language     = English
The snapshot cannot be attached to the snapshot mount point because the snapshot mount point does not permit host-initiated attaches. Enable the snapshot mount point�s �allowInBandSnapAttach� property and retry the attach operation.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot cannot be created since the pool's high watermark is breached and there are no auto-deletable snsaps in the pool.
;//
;// Generated value should be: 0xE16D80AB
;//
MessageId    = 0x80AB
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_AUTODELETABLE_SNAPS_DO_NOT_EXIST
Language     = English
The snapshot cannot be created because it will be immediately auto-deleted.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot could not be created because peer SP is not yet ready.
;//
;// Generated value should be: 0xE16D80AC
;//
MessageId    = 0x80AC
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CREATION_RESTRICTED_PEER_NOT_READY
Language     = English
The snapshot creation failed because it timed out. Retry the operation. If the problem persists, gather SPcollects and contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The Snapshot attach or restore operation cannot be performed because Data Change Notify 
;//                     start notification failed.
;//
;// Generated value should be: 0xE16D80AD
;//
MessageId    = 0x80AD
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DATA_CHANGE_NOTIFY_START_NOTIFICATION_FAILED 
Language     = English
An internal error occurred. The Snapshot Attach or Restore operation cannot be performed because Data Change Notify start notification failed.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Snapshot operations rejected because the target is a system snapshot.
;//
;// Generated value should be: 0xE16D80B0
;//
MessageId    = 0x80B0
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SYSTEM_SNAP_RESTRICTED
Language     = English
The specified operation cannot be performed on a system snapshot.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Internal Error
;//
;// Severity: Error
;// 
;// Symptom of Problem: The snapshot and the specified resource do not belong to the same family. 
;//
;// Generated value should be: 0xE16D80B1
;//
MessageId    = 0x80B1
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_AND_RESOURCE_IN_DIFFERENT_FAMILIES 
Language     = English
The specified snapshot and the resource are in different families. 
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Internal Error
;//
;// Severity: Error
;// 
;// Symptom of Problem: The snapshot is in the process of a replication update.
;//
;// Generated value should be: 0xE16D80B2
;//
MessageId    = 0x80B2
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REPLICATION_UPDATE_IN_PROGRESS 
Language     = English
The specified snapshot is in the process of a replication update.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to snapcli 
;//
;// Severity: Error
;//
;// Symptom of Problem: The inband operation is not allowed because it changes members of the immutable consistency group.
;//
;// Generated value should be: 0xE16D80B3
;//
MessageId    = 0x80B3
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_INBAND_CG_MEMBERSHIP_CHANGE_NOT_ALLOWED
Language     = English
The inband operation is not allowed because it changes members of the immutable consistency group.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limits of snaps per SF
;//
;// Generated value should be: 0xE16D80B4
;//
MessageId    = 0x80B4
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_UDI_ERROR_CREATE_MAX_SNAPS_PER_SF_LIMIT_EXCEEDED
Language     = English
Unable to create the snapshot because the maximum number of snapshots already exists for the file system.
Internal Information Only.Could not create snapshot because maximum number of snapshots %2 already exists for the filesystem %3.
.

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed limit of filesystem snaps.
;//
;// Generated value should be: 0xE16D80B5
;//
MessageId    = 0x80B5
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_UDI_ERROR_CREATE_MAX_FS_SNAPS_LIMIT_EXCEEDED
Language     = English
Unable to create the snapshot because the maximum number of filesystem snapshots already exist in the array.
Internal Information Only.Could not create snapshot for filesystem %3 because maximum number of filesystem snapshots %2 already exist.
.


;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the maximum allowed snap levels.
;//
;// Generated value should be: 0xE16D80B6
;//
MessageId    = 0x80B6
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_SNAP_LEVEL_REACHED
Language     = English
Unable to create the snapshot because the creation would exceed the maximum number of snapshot levels. 
Internal Information Only. Could not create snapshot for file system %3 because it would exceed the maximum number of snapshot levels of %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log and Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to create more than the maximum number of snapshots. 
;//
;// Generated value should be: 0xE16D80B7
;//
MessageId    = 0x80B7
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_MAX_LUN_SNAPS_LIMIT_EXCEEDED
Language     = English
Unable to create the snapshot because the creation will exceed the maximun number of LUN snapshots.
Internal Information Only.Could not create snapshot for LUN %3 because it will exceed the maximum numBer of LUN snapshots %2.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to copy a Snapshot while the copy operation is in 
;//                     progress for the Snapshot. The copy operation cannot proceed since the  
;//                     snapshot is currently being copied.
;//                         
;// Generated value should be: 0xE16D80B8
;//
MessageId    = 0x80B8
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_COPY_RESTRICTED_COPY_IN_PROGRESS
Language     = English
The copy operation failed because the snapshot is already being copied. Retry the operation after the copy has completed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to copy a Snapshot while the source is not a snapshot. 
;//                     The copy operation cannot proceed because the snapshot hasn't been specified 
;//                     as the source.
;//                         
;// Generated value should be: 0xE16D80B9
;//
MessageId    = 0x80B9
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_COPY_RESTRICTED_SOURCE_NOT_SNAPSHOT
Language     = English
The copy operation failed because the source is not a snapshot. Please specify snapshot as the source.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot create share on a checkpoint snap. 
;//
;// Generated value should be: 0xE16D80BA
;//
MessageId    = 0x80BA
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_CREATE_SHARE_ON_CHECKPOINT_SNAP
Language     = English
The share could not be created because the snapshot is a checkpoint snapshot.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot create share on a system snapshot.
;//
;// Generated value should be: 0xE16D80BB
;//
MessageId    = 0x80BB
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_CREATE_SHARE_ON_SYSTEM_SNAP
Language     = English
The share could not be created because the snapshot is a system snapshot.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot restore/delete a snapshot with shares.
;//
;// Generated value should be: 0xE16D80BC
;//
MessageId    = 0x80BC
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_OPERATION_FAILED_BECAUSE_SHARE_EXISTS
Language     = English
The operation cannot be performed because the snapshot has shares.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation cannot be executed because the snapshot is being destroyed.
;//
;// Generated value should be: 0xE16D80BD
;//
MessageId    = 0x80BD
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_DESTROYING
Language     = English
The operation cannot be performed because the snapshot is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation cannot be executed because the snapshot is not ready.
;//
;// Generated value should be: 0xE16D80BE
;//
MessageId    = 0x80BE
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_NOT_READY
Language     = English
The operation cannot be performed because the snapshot is not ready. Resolve any hardware problems and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation cannot be executed because the snapshot is being resized.
;//
;// Generated value should be: 0xE16D80BF
;//
MessageId    = 0x80BF
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_IS_BEING_RESIZED
Language     = English
The operation cannot be performed because the snap is being resized or converted to a thick LUN. Please wait for the resize or conversion to complete and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation cannot be executed because there is a rollback operation in progress.
;//
;// Generated value should be: 0xE16D80C0
;//
MessageId    = 0x80C0
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_RESTORE_IN_PROGRESS
Language     = English
The operation cannot be performed because the snap is being restored. Please wait for the restoration to complete and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot copy snap on a system snapshot.
;//
;// Generated value should be: 0xE16D80C1
;//
MessageId    = 0x80C1
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_COPY_UFS_SNAP_ON_SYSTEM_SNAP
Language     = English
Could not copy the snap because the snapshot is a system snapshot.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot copy snap on a system snapshot.
;//
;// Generated value should be: 0xE16D80C2
;//
MessageId    = 0x80C2
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_RESTORE_UFS_SNAP_ON_SYSTEM_SNAP
Language     = English
Could not restore the snap because the snapshot is a system snapshot.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create or copy a snap which is being
;//                     copied or created.
;//
;// Generated value should be: 0xE16D80C3
;//
MessageId    = 0x80C3
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTRICTED_COPY_OR_CREATE_IN_PROGRESS
Language     = English
The operation failed because the snapshot is being created or copied. Retry the operation after the copy or create operation has completed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Snaps are being invalidated due to space issues
;//
;// Generated value should be: 0xE16D80C4
;//
MessageId    = 0x80C4
;//SSPG C4type=User
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INVALIDATED_SNAP_IN_RESOURCE
Language     = English
All Snapshots in resource %2 have been automatically marked for destruction due to insufficient pool space.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to create a Snap failed
;//
;// Generated value should be: 0xE16D80C5
;//
MessageId    = 0x80C5
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_CREATE_SNAP_FAILED
Language     = English
An in-band request to create snapshot %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to destroy a Snap failed
;//
;// Generated value should be: 0xE16D80C6
;//
MessageId    = 0x80C6
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_DESTROY_SNAP_FAILED
Language     = English
An in-band request to destroy snapshot %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to attach a Snap failed
;//
;// Generated value should be: 0xE16D80C7
;//
MessageId    = 0x80C7
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_ATTACH_SNAP_FAILED
Language     = English
An in-band request to attach snapshot %2 to Mount Point %3 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to detach a Snap failed
;//
;// Generated value should be: 0xE16D80C8
;//
MessageId    = 0x80C8
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_DETACH_SNAP_FAILED
Language     = English
An in-band request to detach snapshot %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to copy a Snap failed
;//
;// Generated value should be: 0xE16D80C9
;//
MessageId    = 0x80C9
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_COPY_SNAP_FAILED
Language     = English
An in-band request to copy snapshot %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to create a CG failed
;//
;// Generated value should be: 0xE16D80CA
;//
MessageId    = 0x80CA
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_CREATE_CG_FAILED
Language     = English
An in-band request to create consistency group %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Inband request to replace members of a CG failed
;//
;// Generated value should be: 0xE16D80CB
;//
MessageId    = 0x80CB
;//SSPG C4type=Audit
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_INBAND_REPLACE_CG_FAILED
Language     = English
An in-band request to replace members of consistency group %2 failed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot delete share on a system snapshot.
;//
;// Generated value should be: 0xE16D80CC
;//
MessageId    = 0x80CC
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_DELETE_SHARE_ON_SYSTEM_SNAP
Language     = English
The share could not be deleted because the snapshot is a system snapshot.
.
;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The set_file_system_properties2 API returned an error while updating
;//                     UFS properties during snapshot restore operation.  
;//
;// Generated value should be: 0xE16D80CD
;//
MessageId    = 0x80CD
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_SET_UFS_FILE_SYSTEM_PROPERTIES_FAILED
Language     = English
Internal Information Only. FS restore object %2 failed while setting FS properties.
Attempt to set properties of FS %3 failed with status %4
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere.
;//
;// Severity: Error
;//
;// Symptom of Problem: File system snap creation failed because the lower deck is not active
;//                     on this SP.
;//
;// Generated value should be: 0xE16D80CE
;//
MessageId    = 0x80CE
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_LOWER_DECK_NOT_ACTIVE_ON_THIS_SP
Language     = English
The File System snapshot cannot be created because the lower deck is not active on this SP. Please wait for the failover to complete and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Unisphere.
;//
;// Severity: Error
;//
;// Symptom of Problem: File System snap creation failed because the VDM obj is not ready.
;//
;// Generated value should be: 0xE16D80CF
;//
MessageId    = 0x80CF
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_VDM_NOT_READY
Language     = English
The File System snapshot cannot be created because the NAS Server is not ready.
Retry the operation.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination of the restore operation is replication destination candidate.
;//
;// Generated value should be: 0xE16D80D0
;//
MessageId    = 0x80D0
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_DESTINATION_IS_REPLICATION_DESTINATION_CANDIDATE
Language     = English
The restore operation cannot be performed because destination of the restore operation is replication destination candidate.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination is in a replication session and shrinking is not allowed.
;//
;// Generated value should be: 0xE16D80D1
;//
MessageId    = 0x80D1
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_SNAP_DESTINATION_SHRINK
Language     = English
The restore operation cannot be performed because the destination is in a replication session and shrinking is not allowed.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: an initializing LUN that is a replication destination candidate can not be added to a CG.
;//
;// Generated value should be: 0xE16D80D2
;//
MessageId    = 0x80D2
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_CG_MEMBER_RESTRICTED_LUN_REP_DEST_INITIALIZING
Language     = English
The LUN could not be added to the consistency group because the LUN is a replication destination candidate and is also initializing.
.


;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the LUN is used for replication it cannot be added to a consistency group.
;//
;// Generated value should be: 0xE16D80D3
;//
MessageId    = 0x80D3
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_CG_MEMBER_RESTRICTED_LUN_IN_USE_BY_REPLICATION
Language     = English
The LUN could not be added to the consistency group because the LUN is being used for replication
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication members cannot be added to it.
;//
;// Generated value should be: 0xE16D80D4
;//
MessageId    = 0x80D4
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ADD_CG_MEMBER_RESTRICTED_CG_IN_USE_BY_REPLICATION
Language     = English
The LUN could not be added to the consistency group because the consistency group is being used for replication
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication members cannot be removed from it.
;//
;// Generated value should be: 0xE16D80D5
;//
MessageId    = 0x80D5
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REMOVE_CG_MEMBER_RESTRICTED_CG_IN_USE_BY_REPLICATION
Language     = English
The LUN could not be removed from the consistency group because the consistency group is being used for replication
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication it can not be deleted. 
;//
;// Generated value should be: 0xE16D80D6
;//
MessageId    = 0x80D6
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_DELETE_CG_RESTRICTED_CG_IN_USE_BY_REPLICATION
Language     = English
The Consistency Group could not be deleted because the Consistency Group is being used for replication
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication as a secondary (destination) it can not be restored from a snap set. 
;//
;// Generated value should be: 0xE16D80D7
;//
MessageId    = 0x80D7
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_RESTORE_CG_RESTRICTED_CG_IN_USE_BY_REPLICATION
Language     = English
The consistency group could not be restored because the consistency group is being used for replication as a destination
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication as an active secondary we can not clear the replication destination property
;//
;// Generated value should be: 0xE16D80D8
;//
MessageId    = 0x80D8
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_CG_IN_USE_AS_ACTIVE_REPLICATION_DESTINATION
Language     = English
The replication destination property can not be cleared while the consistency group is an active replication destination.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform an attach operation on a non-system File System snapshot.
;//
;// Generated value should be: 0xE16D80D9
;//
MessageId    = 0x80D9
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_ATTACH_NON_SYSTEM_UFS_SNAP
Language     = English
Cannot perform an attach operation on a non-system File System snapshot.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform an attach operation on a mounted File System snapshot.
;//
;// Generated value should be: 0xE16D80DA
;//
MessageId    = 0x80DA
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_ATTACH_MOUNTED_UFS_SNAP
Language     = English
Cannot perform an attach operation on a mounted File System snapshot.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System snapshot is already attached.
;//
;// Generated value should be: 0xE16D80DB
;//
MessageId    = 0x80DB
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_IS_ALREADY_ATTACHED
Language     = English
Cannot perform an attach operation because the File System snapshot is already attached.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System snapshot is already detached.
;//
;// Generated value should be: 0xE16D80DC
;//
MessageId    = 0x80DC
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_UFS_SNAP_IS_ALREADY_DETACHED
Language     = English
Cannot perform a detach operation because the File System snapshot is already detached.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform a detach operation on a non-system File System snapshot.
;//
;// Generated value should be: 0xE16D80DD
;//
MessageId    = 0x80DD
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_DETACH_NON_SYSTEM_UFS_SNAP
Language     = English
Cannot perform a detach operation on a non-system File System snapshot.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is in the process of being restored we can not modify the replication destination property
;//
;// Generated value should be: 0xE16D80DE
;//
MessageId    = 0x80DE
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_CG_ROLLBACK_REPLICATION_DESTINATION
Language     = English
Cannot modify the replication destination property of the consistency group while a rollback is in progress.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: The CG replication destination property can not be modified while and NDU is in progress.
;//
;// Generated value should be: 0xE16D80DF
;//
MessageId    = 0x80DF
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_CG_NDU_REPLICATION_DESTINATION
Language     = English
Cannot modify the replication destination property of the consistency group while an NDU is in progress.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is used for replication members cannot be replaced from it.
;//
;// Generated value should be: 0xE16D80E0
;//
MessageId    = 0x80E0
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REPLACE_CG_MEMBER_RESTRICTED_CG_IN_USE_BY_REPLICATION
Language     = English
The LUN could not be replaced from the consistency group because the consistency group is being used for replication
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: As the CG is initializing its properties can not be updated.
;//
;// Generated value should be: 0xE16D80E1
;//
MessageId    = 0x80E1
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_SET_CG_PROPERTIES_RESTRICTION_CG_INITIALIZING
Language     = English
Cannot modify the consistency group properties while it is initializing.
.
;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform a refresh snap operation due to another refresh snap operation in progress.
;//
;// Generated value should be: 0xE16D80E2
;//
MessageId    = 0x80E2
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REFRESH_SNAP_IN_PROGRESS
Language     = English
Cannot perform a refresh snap operation due to an existing refresh snap process object.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform a refresh snap operation due to a snap create, destroy or restore operation in progress.
;//
;// Generated value should be: 0xE16D80E3
;//
MessageId    = 0x80E3
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REFRESH_SNAP_PROHIBITED_DURING_SNAP_CREATION_OR_RESTORE_OR_DESTRUCTION
Language     = English
Cannot perform a refresh snap operation due to a snap creation, restore or destruction in progress.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Internal Error.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform a refresh snap operation due to a mounted snap.
;//
;// Generated value should be: 0xE16D80E4
;//
MessageId    = 0x80E4
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_REFRESH_SNAP_PROHIBITED_FOR_MOUNTED_SNAP
Language     = English
Cannot perform a refresh snap operation on a mounted snap.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Refresh Snap Process failed as the source file system has trespassed.
;//
;// Generated value should be: 0xE16D80E5
;//
MessageId    = 0x80E5
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_SNAPSHOT_SOURCE_HAS_TRESPASSED
Language     = English
The operation failed because the snapshot source has trespassed.
.

;// --------------------------------------------------
;// Introduced In: KH+ (PSI4)
;//
;// Usage: Returned to Unisphere Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot perform a refresh snap operation as the file system snap is not ready.
;//
;// Generated value should be: 0xE16D80E6
;//
MessageId    = 0x80E6
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ERROR_FILE_SYSTEM_SNAP_NOT_READY
Language     = English
The operation failed because the file system snapshot is not in the ready state.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: The restore operation cannot be performed because the destination of the restore operation is being used by an active 
;// sync replication session
;//
;// Generated value should be: 0xE16D80E7
;//
MessageId    = 0x80E7
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_ROLLBACK_DESTINATION_USED_BY_ACTIVE_SYNC_REPLICATION_SESSION
Language     = English
The restore operation cannot be performed because destination of the restore operation is being used by an active sync replication session.
.

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
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Critical
;//
;// Symptom of Problem: Template
;//
MessageId    = 0xC000
Severity     = Error
Facility     = MLUSnapshot
SymbolicName = MLU_SNAPSHOT_CRITICAL_TEMPLATE
Language     = English
Only for template
.


;#endif //__K10_MLU_SNAPSHOT_MESSAGES__
