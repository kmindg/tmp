;//++
;// Copyright (C) EMC Corporation, 2014
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            LXFMessages.mc
;//
;// Description:     This file contains the message catalog for LXF messages.
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
;#ifndef __LXF_MESSAGES__
;#define __LXF_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( LXFusion      = 0x174 : FACILITY_LXFUSION )

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
;// Introduced In: Liberty-Northbridge
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LXF Volume was created.
;//
;// Generated value should be: 0x41740000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = LXFusion
SymbolicName = LXF_INFO_CREATED_LXF_VOLUME
Language     = English
Created LXF Volume "%2".
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LXF Volume was destroyed.
;//
;// Generated value should be: 0x41740001
;//
MessageId    = +1
Severity     = Informational
Facility     = LXFusion
SymbolicName = LXF_INFO_DESTROYED_LXF_VOLUME
Language     = English
Destroyed LXF Volume "%2".
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LXF Volume state has changed.
;//
;// Generated value should be: 0x41740002
;//
MessageId    = +1
Severity     = Informational
Facility     = LXFusion
SymbolicName = LXF_INFO_VOLUME_STATE_CHANGED
Language     = English
Internal Information Only. The state of LXF Volume "%2" has changed from %3 to %4.
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
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x81744000
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_TEMPLATE
Language     = English
Only for template
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: LXF Volume is in a warning state because its
;//                     underlying storage device has increased its
;//                     capacity, but LXF cannot utilize the added storage.
;//
;// Generated value should be: 0x91744001
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_VOLUME_CAPACITY_INCREASED
Language     = English
The system found %2 attached vdisks.  This exceeds the system maximum of %3 vdisks.  The remaining disks cannot be utilized and will not be displayed by the system.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: The number of virtual disks has exceeded its
;//                     limit.
;//
;// Generated value should be: 0x91744002
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_VOLUME_EXCEEDED_LIMIT
Language     = English
Found number of virtual disks (%2) exceeds the maximum allowed by vVNX (%3). The system cannot use the extra disks.
.

;// --------------------------------------------------
;// Introduced In: Thunderbird
;//
;// Usage: Return to Unisphere and logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: The virtual disk is degraded; the peer SP cannot reach the storage.
;//
;// Generated value should be: 0x91744003
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_VOLUME_DEGRADED_PEER_OFFLINE
Language     = English
The peer SP reports that it cannot reach the virtual disk "%2".  The virtual disk is degraded.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Thunderbird
;//
;// Usage: Return to Unisphere and logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: The virtual disk is degraded; the peer SP took an error.
;//
;// Generated value should be: 0x91744004
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_VOLUME_DEGRADED_PEER_ERROR
Language     = English
The peer SP reports a failure on virtual disk "%2".  The virtual disk is degraded.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Thunderbird
;//
;// Usage: Return to Unisphere and logged in Event log
;//
;// Severity: Warning
;//
;// Symptom of Problem: The virtual disk is degraded; the peer SP took an error.
;//
;// Generated value should be: 0x91744005
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Warning
Facility     = LXFusion
SymbolicName = LXF_WARNING_VOLUME_DEGRADED_PEER_ONLINE
Language     = English
The local SP reports a failure to discover or perform I/O to virtual disk "%2".  The virtual disk is degraded.  Please check or update the storage configuration, and contact your service provider if the problem persists.
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
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to failure
;//                     to open its underlying storage device.
;//
;// Generated value should be: 0xd1748000
;//
MessageId    = 0x8000
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_COULD_NOT_OPEN_STORAGE
Language     = English
%2 has failed because the storage system could not connect to the volume's underlying storage.  Please check or update the storage configuration, and contact your service provider if the problem persists.
Internal Information Only. Error %3 attempting to open internal storage device %4.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state because its
;//                     underlying storage device has shrunk.
;//
;// Generated value should be: 0xd1748001
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_VOLUME_CAPACITY_DECREASED
Language     = English
%2 has failed because the volume's underlying storage device's capacity has decreased.  Please check or update the storage configuration, and contact your service provider if the problem persists.
Internal Information Only. Device %3 capacity is now %4 sectors. Expected %5 sectors.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to failure
;//                     to read its metadata from disk.
;//
;// Generated value should be: 0xd1748002
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_COULD_NOT_READ_LABEL
Language     = English
%2 has failed because the storage system was unable to read its internal metadata from disk.  Please check or update the storage configuration, and contact your service provider if the problem persists.
Internal Information Only. Error %3 reading LX-Fusion metadata from device %4.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to reading
;//                     bad metadata from disk.
;//
;// Generated value should be: 0xd1748003
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_LABEL_INVALID_OR_CORRUPT
Language     = English
%2 has failed because its internal metadata was invalid or corrupt on disk. Please check or update the storage configuration, and contact your service provider if the problem persists.
Internal Information Only. Error %3 reading LX-Fusion metadata from device %4.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to failure
;//                     to write its metadata to disk.
;//
;// Generated value should be: 0xd1748004
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_COULD_NOT_WRITE_LABEL
Language     = English
%2 has failed because the storage system was unable to write its internal metadata to disk.  Please check or update the storage configuration, and contact your service provider if the problem persists.
Internal Information Only. Error %3 writing LX-Fusion metadata to device %4.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume has failed and could not be added 
;//                     to a storage pool because it is in use by another
;//                     storage system.
;//
;// Generated value should be: 0xd1748005
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_VOLUME_ALREADY_IN_USE
Language     = English
%2 has failed and could not be added to a storage pool because it is already in use by another storage pool.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation is failing because the LXF Volume has
;//                     failed.
;//
;// Generated value should be: 0xc1748006
;//
MessageId    = +1
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_VOLUME_FAILED
Language     = English
The operation could not be completed because %2 has failed.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to failure
;//                     to write its metadata to disk.
;//
;// Generated value should be: 0xd1748007
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_DEVICE_SEEK_FAILED
Language     = English
%2 has failed because the storage system was unable to determine the device size.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: LXF Volume is in an error state due to volume
;//                     size detected during init not matching persisted
;//                     volume size.
;//
;// Generated value should be: 0xd1748008
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_VOLUME_SIZE_MISMATCH
Language     = English
%2 has failed because the underlying device size has changed.  Please check or update the storage configuration, and contact your service provider if the problem persists.
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Underlying virtual disk size is below min limit
;//
;// Generated value should be: 0xd1748009
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_DEVICE_SIZE_TOO_SMALL
Language     = English
Unable to consume virtual disk %2. Size of the virtual disk (%3 GB) is below minimum supported limit (%4 GB).
.

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Return to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Underlying virtual disk size is above max limit
;//
;// Generated value should be: 0xd174800a
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_ERROR_DEVICE_SIZE_TOO_LARGE
Language     = English
Unable to consume virtual disk %2. Size of the virtual disk (%3 GB) is above maximum supported limit (%4 GB).
.

;
;
;
;//============================
;//======= CRITICAL MSGS ======
;//============================
;// NOTE: Although these messages are critical, the Severity must be Error
;// as Critical is not a supported severity
;

;// --------------------------------------------------
;// Introduced In: Liberty-Northbridge
;//
;// Usage: Template
;//
;// Severity: Critical
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0xc174c000
;//
MessageId    = 0xC000
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_CRITICAL_TEMPLATE
Language     = English
This error code is left as a template for future error codes.
.

;// --------------------------------------------------
;// Introduced In: Liberty-BrooksHill
;//
;// Usage: Logged in Event log
;//
;// Severity: Critical
;//
;// Symptom of Problem: An unsupported SCSI controller has been added to the system
;//
;// Generated value should be: 0xd174c001
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Error
Facility     = LXFusion
SymbolicName = LXF_CRITICAL_UNSUPPORTED_SCSI_CONTROLLER
Language     = English
An unsupported virtual SCSI controller (%2) was added to the system.
.

;#endif //__LXF_MESSAGES__
