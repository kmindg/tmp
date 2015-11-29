;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10MluCompressMessages.mc
;//
;// Description:     This file contains the message catalogue for
;//                  the MLU Compression feature.
;//
;// History:
;//
;//--
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;// The MLU Compression feature further classifies its range by breaking up each CLARiiON
;// category into a sub-category for each MLU Compression component.  The
;// second nibble represents the MLU component.  We are not using
;// the entire range of the second nibble so we can expand if the future if
;// necessary.
;//
;// +--------------------------+---------+-----+
;// | Component                |  Range  | Cnt |
;// +--------------------------+---------+-----+
;// | Compression Coordinator  |  0xk0mn | 256 |
;// | Compression Engine       |  0xk1mn | 256 |
;// | Compression Manager      |  0xk2mn | 256 |
;// | Admin DLL                |  0xk3mn | 256 |
;// +--------------------------+---------+-----+
;// | Unused                   |  0xk4mn |     |
;// | Unused                   |  0xk5mn |     |
;// | Unused                   |  0xk6mn |     |
;// | Unused                   |  0xk7mn |     |
;// | Unused                   |  0xk8mn |     |
;// | Unused                   |  0xk9mn |     |
;// | Unused                   |  0xkamn |     |
;// | Unused                   |  0xkbmn |     |
;// | Unused                   |  0xkcmn |     |
;// | Unused                   |  0xkdmn |     |
;// | Unused                   |  0xkemn |     |
;// | Unused                   |  0xkfmn |     |
;// +--------------------------+---------+-----+
;//
;// Where:
;// 'k' is the CLARiiON error code classification.  Note that we only use
;// the top two bits.  So there are two bits left for future expansion.
;//
;//  'mn' is the Manager specific error code.
;//
;//  *** NOTE : tabs in this file are set to every 4 spaces. ***
;//
;
;#ifndef __K10_MLU_COMPRESS_MESSAGES__
;#define __K10_MLU_COMPRESS_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_ERROR_CODE
                  MluCompress   = 0x165 : FACILITY_COMPRESS
                )

SeverityNames = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2 : STATUS_SEVERITY_WARNING
                  Error         = 0x3 : STATUS_SEVERITY_ERROR
                )

;//==============================================================
;//======= START COMPRESSION COORDINATOR INFORMATION MSGS =======
;//==============================================================
;//
;// Compression Coordinator "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_INFO_INFLATION_DETECTED
Language     = English
Internal Information Only.
.

;//==============================================================
;//======= END COMPRESSION COORDINATOR INFORMATION MSGS =========
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION ENGINE INFORMATION MSGS ============
;//==============================================================
;//
;// Compression Engine "Information" messages.
;//


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = 0x0100
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_COMPRESSION_STARTING_SUCCESS  
Language     = English
Internal information only. Compression pass successfully started on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//    %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_DECOMPRESSION_STARTING_SUCCESS
Language     = English
Internal information only. Decompression pass started successfully on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_RECOMPRESSION_STARTING_SUCCESS
Language     = English
Internal information only. Recompression pass started successfully on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_COMPRESSION_COMPLETE_SUCCESS
Language     = English
Internal information only. Compression pass on LUN %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_DECOMPRESSION_COMPLETE_SUCCESS
Language     = English
Internal information only. Decompression pass on LUN %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_INFO_RECOMPRESSION_COMPLETE_SUCCESS
Language     = English
Internal information only. Recompression pass on LUN %2 completed successfully.
.


;//==============================================================
;//======= END COMPRESSION ENGINE INFORMATION MSGS ==============
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION MANAGER INFORMATION MSGS ===========
;//==============================================================
;//
;// Compression Manager "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = 0x0200
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_ENABLING_INITIATED_SUCCESSFULLY
Language     = English
Turning on Compression was successfully initiated for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_ENABLING_SUCCESS
Language     = English
Compression was turned on successfully for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_STARTING_SUCCESS  
Language     = English
Internal information only. Compression pass successfully started on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_DISABLING_INITIATED_SUCCESSFULLY
Language     = English
Turning off Compression was successfully initiated for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_DISABLING_SUCCESS
Language     = English
Compression was turned off successfully for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//    %2 is the file OID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_DECOMPRESSION_STARTING_SUCCESS
Language     = English
Internal information only. Decompression started successfully on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_RECOMPRESSION_STARTING_SUCCESS
Language     = English
Recompression started successfully on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_MIG_COMPLETE_SUCCESS
Language     = English
Initial compression of LUN %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_COMPRESSION_COMPLETE_SUCCESS
Language     = English
Compression on LUN %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the LUN WWID.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_INFO_DECOMPRESSION_COMPLETE_SUCCESS
Language     = English
Decompression on LUN %2 completed successfully.
.

;//==============================================================
;//======= END COMPRESSION MANAGER INFORMATION MSGS =============
;//==============================================================
;//



;//==============================================================
;//======= START ADMIN DLL INFORMATION MSGS =====================
;//==============================================================
;//
;// Admin DLL "Information" messages.
;//


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %1 is the operation.
;//     %2 is the Source and Destination LUN WWID of migration.
MessageId    = 0x0300
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_INFO_INTERNAL_MIGRATION_STARTED_SUCCESSFULLY
Language     = English
Internal Information Only. Internal migration for %1 was successfully started on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %1 is the operation.
;//     %2 is the Source and Destination LUN WWID of migration.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_INFO_INTERNAL_MIGRATION_COMPLETED_SUCCESSFULLY
Language     = English
Internal Information Only. Internal migration for %1 was successfully completed on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %1 is the operation.
;//     %2 is the Source and Destination LUN WWID of migration.
MessageId    = +1
Severity     = Informational
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_INFO_INTERNAL_MIGRATION_CANCELED_SUCCESSFULLY
Language     = English
Internal Information Only. Internal migration for %1 was successfully canceled on LUN %2.
.


;//==============================================================
;//======= END ADMIN DLL INFORMATION MSGS =======================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION COORDINATOR WARNING MSGS ===========
;//==============================================================

;//
;// Compression Coordinator "Warning" messages.
;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log.
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = 0x4000
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_WARNING_MARK_BLOCK_BAD
Language     = English
Internal Information Only. Compression requested %2 blocks to be marked as bad blocks on LUN %3 at file offset %4.
.



;//==============================================================
;//======= END COMPRESSION COORDINATOR WARNING MSGS =============
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION ENGINE WARNING MSGS ================
;//==============================================================
;//
;// Compression Engine "Warning" messages.
;//





;//==============================================================
;//======= END COMPRESSION ENGINE WARNING MSGS ==================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION MANAGER WARNING MSGS ===============
;//==============================================================
;//
;// Compression Manager "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code).
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = 0x4200
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_COMPRESS_SYSTEM_PAUSED
Language     = English
The system has paused compression on this LUN because it could not obtain needed temporary space in the Storage Pool.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code).
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_DECOMPRESS_PROHIBITED_SYSTEM_PAUSED
Language     = English
The system has paused decompression on this LUN to prevent exhaustion of free space in the Storage Pool. To continue, either add storage or free existing storage in the Storage Pool.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code).
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_DECOMPRESS_RESTRICTED_SYSTEM_PAUSED
Language     = English
The system has paused decompression on this LUN to prevent exhaustion of free space in the Storage Pool. To continue, either add storage or free existing storage in the Storage Pool. To override this warning, resume the decompression.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code).
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_SET_OVERRIDE_IN_PROHIBITED_MODE
Language     = English
Override for System Pause was successfully applied to this LUN. Since the amount of free space in the Storage Pool is very low, decompression will remain System Paused. To continue, either add storage or free existing storage in the Storage Pool.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log.
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//     Compression is system paused.
;// Description:
;//     %2 is LUN WWID, %3 is pool name.
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_COMPRESS_SYSTEM_PAUSED_LOG
Language     = English
The system has paused compression on LUN %2 because it could not obtain needed temporary space in the Storage Pool %3.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log.
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_DECOMPRESS_PROHIBITED_SYSTEM_PAUSED_LOG
Language     = English
The system has paused decompression on LUN %2 to prevent exhaustion of free space in the Storage Pool %3. To continue, either add storage or free existing storage in the Storage Pool.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log.
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = +1
Severity     = Warning
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_WARNING_DECOMPRESS_RESTRICTED_SYSTEM_PAUSED_LOG
Language     = English
The system has paused decompression on LUN %2 to prevent exhaustion of free space in the Storage Pool %3. To continue, either add storage or free existing storage in the Storage Pool. To override this warning, resume the decompression.
.
;//==============================================================
;//======= END COMPRESSION MANAGER WARNING MSGS =================
;//==============================================================
;//


;//==============================================================
;//======= START ADMIN DLL WARNING MSGS =========================
;//==============================================================
;//
;// Admin DLL "Warning" messages.
;//





;//==============================================================
;//======= END ADMIN DLL WARNING MSGS ===========================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION COORDINATOR ERROR MSGS =============
;//==============================================================
;//
;// Compression Coordinator "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//
MessageId    = 0x8000
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_NO_EXTENT_INFO
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_INVALID_EXTENT_INFO
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_FORMAT_VIOLATION
Language     = English
Compression found invalid or inconsistent data on LUN %2 at LBA %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_COMPRESSED_DATA_CORRUPT
Language     = English
Compression detected corrupt compressed data on LUN %2 at LBA %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;// 
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_INVALID_ARGUMENT
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;// 
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_INVALID_SUBIO_STATUS
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Corrupt data is found on the LUN during the Compression operation.
;//
;// Description:
;//     %2 is the data length in blocks.
;//     %3 is the LUN id.
;//     %4 is the user visible LBA.
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_UNCOMPRESSED_DATA_CORRUPT
Language     = English
Compression detected %2 blocks of corrupt uncompressed data on LUN %3 at LBA %4. Please gather SPcollects and contact your Service Provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Returned to the engine when fixture is quiesced. 
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_COORD_ERROR_FIXTURE_IS_QUIESCED
Language     = English
Internal Information Only.
.

;//==============================================================
;//======= END COMPRESSION COORDINATOR ERROR MSGS ===============
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION ENGINE ERROR MSGS ==================
;//==============================================================
;//
;// Compression Engine "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the FileOID.
MessageId    = 0x8100
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_ALREADY_IN_LIST
Language     = English
An internal error occurred. Please contact your service provider. Compression object %2 is already in list.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_MAX_OBJS_EXCEEDED
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the FileOID.
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_NOT_IN_LIST
Language     = English
An internal error occurred. Please contact your service provider. Compression object %2 is not in list.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_FEATURE_ALREADY_SUSPENDED
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_FEATURE_NOT_SUSPENDED
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_BUFFER_TOO_SMALL
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log.
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the operation.
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_INITIALIZATION_FAILED
Language     = English
Internal Information Only. Compression Engine failed to initialize when %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_NO_PROCESS_STREAM_AVAILABLE
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the FileOID.
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_RUNNING
Language     = English
Internal Information Only. Compression object %2 is running.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the FileOID.
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_ALREADY_STARTED
Language     = English
Internal Information Only. Compression object %2 was already started.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//     %2 is the FileOID.
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_ALREADY_STOPPED
Language     = English
Internal Information Only. Compression object %2 was already stopped.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the FileOID, %3 is the operation.
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ENG_ERROR_OBJ_NOT_IN_LIST_INTERNAL
Language     = English
Internal Information Only. Compression object %2 was not found in list when %3.
.

;//==============================================================
;//======= END COMPRESSION ENGINE ERROR MSGS ====================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION MANAGER ERROR MSGS =================
;//==============================================================
;//
;// Compression Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;// 
MessageId    = 0x8200
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_COMMON_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_ENABLING_FAILED
Language     = English
Compression could not be turned on for LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_STARTING_FAILED
Language     = English
Compression failed to start on LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_DISABLING_FAILED
Language     = English
Compression could not be turned off for LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_DECOMPRESSION_STARTING_FAILED
Language     = English
Decompression failed to start on LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_RECOMPRESSION_STARTING_FAILED
Language     = English
Recompression failed to start on LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is the pool name, %4 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_MIG_COMPLETE_FAILED
Language     = English
Migrating/Compressing LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_COMPLETE_FAILED
Language     = English
Compression failed on LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID, %3 is failed status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_DECOMPRESSION_COMPLETE_FAILED
Language     = English
Decompression failed on LUN %2 (error %3). Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User attempted the following sequence of actions in a short time decompress - compress - decompress. The last decompression attempt will fail with this error.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_RCPO_STILL_NOT_DESTROYED
Language     = English
Compression could not be turned on for this LUN because a previously aborted attempt to turn on Compression was still pending on it. Please retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     User attempted to perform a Compression operation on a LUN that wasn't compressed.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_NOT_ENABLED_ON_FILE
Language     = English
The Compression operation could not be performed on this LUN because Compression is not turned on for it.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User attempted to a decompress a LU that is already decompressing.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_ALREADY_DECOMPRESSING
Language     = English
Compression could not be turned off for this LUN because it was already in the process of being turned off.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User attempted the following sequence of actions in a short time compress - decompress - compress. The last Compression attempt will fail with this error.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_ACPO_STILL_NOT_DESTROYED
Language     = English
Compression could not be turned off for this LUN because a previously aborted attempt to turn off Compression was still pending on it. Please retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User attempted to turn on Compression on a LU that is already Compressed.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_ALREADY_ENABLED_ON_FILE
Language     = English
Compression could not be turned on for this LUN because it is already turned on.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The user attempted to enable Compression on a LUN after the system had already reached the limit.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_MAX_COMPRESSED_LUS_LIMIT_REACHED
Language     = English
The system has already reached the limit of the maximum number of Compressed LUNs.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is the FileOID
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_OBJ_NOT_REGISTERED_WITH_ENG
Language     = English
Internal Information Only. The Object ID (%2) is not registered with the Compression Engine.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Invalid Compression Rate passed in for Change Rate command.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_INVALID_COMPRESSION_RATE
Language     = English
An internal error occurred while trying to change the Compression rate. Please contact your service provider. Invalid Compression rate provided.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Force recompression command received when the LU cannot be recompressed.
;// Description:
;//     %2 is LUN WWID
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_INVALID_STAGE_FOR_RECOMPRESSION
Language     = English
Internal Information Only. The system cannot force a recompression on LUN %2 because it is already compressing.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//     %2 is LUN WWID
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_UNEXPECTED_MIG_COMPLETE_NOTIFICATION
Language     = English
Internal Information Only. Unexpected Migration completion on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attempted to pause a Compression that was already paused.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_ALREADY_PAUSED
Language     = English
Pausing Compression failed because Compression is already paused on this LUN.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attempted to resume a Compression that wasn't paused.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_NOT_PAUSED
Language     = English
Resuming Compression failed because Compression is not paused on this LUN.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to pause a faulted Compression.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_CANNOT_PAUSE_WHILE_FAULTED
Language     = English
Pausing Compression failed because Compression is faulted on this LUN.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to set override when not decompressing.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_CANNOT_SET_OVERRIDE_WHEN_NOT_DECOMPRESSING
Language     = English
An internal error occurred. Please contact your service provider. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to pause a faulted Compression.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_CANNOT_SET_OVERRIDE_WHEN_NOT_SYSTEM_PAUSED
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to set the override when it was already set.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_OVERRIDE_ALREADY_SET
Language     = English
An internal error occurred. Please contact your service provider..
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to reset the override when it wasn't set.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_OVERRIDE_NOT_SET
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Attemped to suspend Compression when it was already suspended
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_FEATURE_ALREADY_SUSPENDED
Language     = English
Pausing Compression feature failed because Compression is already paused.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//      Attemped to reset the override when it wasn't set.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_FEATURE_NOT_SUSPENDED;
Language     = English
Resuming Compression feature failed because Compression is not paused.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (As Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The LU on which Compression is turned on is in some kind error state.    
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_VU_IN_ERROR;
Language     = English
The LUN is in an error state. Please resolve any issues with the LUN for Compression to continue.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error code only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The LU on which Compression is turned on is in some kind error state.    
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_UNEXPECTED_ALLOCATION_MODE;
Language     = English
Internal Information Only. Unknown allocation mode.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  
;//
;// Description:
;//    %2 is the operation %3 is the failure status
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_OPERATION_FAILED;
Language     = English
Internal Information Only. Could not complete operation %2 because %3. 
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  ACPO failed
;//
;// Description:
;//  Returned as ReasonCode if Compression was faulted by the ACPO
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_ACPO_FAILED
Language     = English
An error occurred while initializing Compression on the LUN. Please turn off Compression for the LUN to recover from the error and retry the operation.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  RCPO failed.
;//
;// Description:
;//  Returned as ReasonCode if Compression was faulted by RCPO
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_RCPO_FAILED
Language     = English
An error occurred while cleaning up after decompressing the LUN. Please turn on Compression for the LUN to recover from the error and retry the operation.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  Compression\Recompression pass failed
;//
;// Description:
;//  Returned as ReasonCode if a compression pass failed.
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_PASS_FAILED
Language     = English
Compression encountered an I/O failure. Please resolve any issues with the LUN for compression to continue.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi (Reason Code)
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  Decompression pass failed
;//
;// Description:
;//  Returned as ReasonCode if a decompression pass failed.
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_DECOMPRESSION_PASS_FAILED
Language     = English
Decompression encountered an I/O failure. Please resolve any issues with the LUN for decompression to continue.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//  Compression was faulted.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_MARKED_FAULTED
Language     = English
Compression on LUN %2 was marked faulted with status %3. Please contact your service provider.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:
;//  Used to log additional information when Compression goes faulted.
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_COMPRESSION_MARKED_FAULTED_INTERNAL
Language     = English
Internal Information Only. Compression on file %2 was marked faulted with status %3. %4 failed with status of %5.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User tried to pause Compression on the LU after decompression is complete but while the file 
;//     is being truncated. Pausing during this stage is not allowed.
;//
;// Description:
;//     
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_CANNOT_PAUSE_AFTER_DECOMPRESSION_COMPLETE
Language     = English
Decompression is in process of being finalized on this LUN. Cannot pause while this is in progress.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     User tried to start Compression on a Feature Storage LUN
;//
;// Description:
;//     
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_CANNOT_ENABLE_COMPRESSION_ON_FSL
Language     = English
Compression cannot be enabled on a Feature Storage LUN.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//        Turn compression on a LUN failed because it has Deduplication enabled on it.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_DEDUPLICATION_CURRENT_DEDUP_ENABLED
Language     = English
The Compression operation could not be performed on this LUN because the LUN already has deduplication enabled.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//        Turn compression on a LUN failed because the LUN is under dedup progress.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_DEDUPLICATION_TARGET_DEDUP_ENABLED
Language     = English
The Compression operation could not be performed on this LUN because deduplication is in progress for this LUN.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//        Turn compression on a LUN failed because it's a snapshot mount point of the VNX Snapshots feature.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_VNXSNAPS_SNAPSHOT_LU
Language     = English
The Compression operation could not be performed on this LUN because the LUN is a snapshot mount point of the VNX Snapshots feature.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//        Turn compression on a LUN failed because it's in a Consistency Group of the VNX Snapshots feature.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_VNXSNAPS_IN_CONSISTENCY_GROUP
Language     = English
The Compression operation could not be performed on this LUN because the LUN is in a Consistency Group of the VNX Snapshots feature.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//        Turn compression on a LUN failed because it has snapshots of the VNX Snapshots feature taken of it.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_MGR_ERROR_VNXSNAPS_BASE_LU_HAS_SNAPSHOTS
Language     = English
The Compression operation could not be performed on this LUN because this LUN has snapshots of the VNX Snapshots feature taken of it.
.
;//==============================================================
;//======= END COMPRESSION MANAGER ERROR MSGS ===================
;//==============================================================
;//


;//==============================================================
;//======= START ADMIN DLL ERROR MSGS ===========================
;//==============================================================
;//
;// Admin DLL "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = 0x8300
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_REQUIRED_PARAMETER_MISSING
Language     = English
An internal error has occurred. Please contact your service provider. Internal Information Only. Expected parameter not specified for the command.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_DATA_ERROR
Language     = English
An internal error has occurred. Please contact your service provider. Internal Information Only. Invalid value specified during operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_INVALID_OBJ_OP_TYPE
Language     = English
An internal error has occurred. Please contact your service provider. Invalid object operation type specified during operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_PERSIST_ERROR
Language     = English
An internal error has occurred. Please contact your service provider. Request failed due to persistent storage problem. Check CIMOM log for details.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_OUT_OF_RESOURCE_ERROR
Language     = English
Operation was not able to proceed due to insufficient resources. Please retry the operation later.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_OUT_OF_IDM_ID_ERROR
Language     = English
Operation was not able to proceed due to insufficient internal resources. Please retry the operation later.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_GET_VOLUME_STATE
Language     = English
Failed to get volume state. Please retry the operation later.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_CAN_NOT_COMPRESS_FRESHLY_BOUND_DLU
Language     = English
You can only turn on Compression after the thick LUN has been written to. Please create a thin LUN and turn on Compression on that LUN instead.
.
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_NDU_IN_PROGRESS
Language     = English
Operation was not able to proceed because NDU is in progress. Please retry the operation after the NDU is complete.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_OFFLINE
Language     = English
Operation was not able to proceed because the LUN is offline.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_BEING_DESTROYED
Language     = English
Operation was not able to proceed because the LUN is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_BEING_SHRINKING
Language     = English
Operation was not able to proceed because the LUN is shrinking. Please retry the operation after the LUN shrink is complete.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_BEING_EXPANDING
Language     = English
Operation was not able to proceed because the LUN is expanding. Please retry the operation after the LUN expansion is complete.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     Uni\DAQ attempted to turn Compression on for a TLU\DLU that already has Compression turned on 
;//     and is not decompressing.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LU_ALREADY_COMPRESSED
Language     = English
Compression could not be turned on for this LUN because it is already turned on.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     Uni\DAQ attempted to turn Compression off for a LUN that does not have 
;//     Compression enabled on it.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LU_NOT_COMPRESSED
Language     = English
The Compression operation could not be performed on this LUN because Compression is not turned on for it.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     Uni\DAQ attempted to turn Compression off for a LUN that does not have 
;//     Compression enabled on it.
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LU_ALREADY_DECOMPRESSING
Language     = English
Compression could not be turned off for this LUN because it was already in the process of being turned off.
.
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     Internal migration session faulted.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LU_MIGRATION_FAULTED
Language     = English
Migration/Compression is faulted for this LUN. Please resolve any issues with the LUN for Migration/Compression to continue.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     A Create LU operation from Compress or Decompress command was attempted that would have caused
;//     the Pool's Percent Full Threshold to be exceeded.
;//
;// Solution:
;//     Please either add more space to the Pool and rerun the command, or force the Compress or Decompress command.
;//
;// Description:
;//     %1 is the operation
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_EXCEEDS_LOW_SPACE
Language     = English
Compression could not be %1 for this LUN because the attempt to create a LUN would have caused the pool to exceed its percent full threshold. Please either add more space to the pool and rerun the command, or force the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     A Create LU operation from Compress or Decompress command was attempted that would have caused
;//     the Harvesting High Water Mark to be exceeded.
;//
;// Solution: 
;//     Please either add more space to the Pool and rerun the command, or force the Compress or Decompress command (not recommended).
;//
;// Description:
;//     %1 is the operation
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_EXCEEDS_HARVESTING_HWM
Language     = English
Compression could not be %1 for this LUN because the attempt to create a LUN would have caused the pool to exceed its auto-delete pool full high watermark. Please either add more space to the pool and rerun the command, or force the command (not recommended).
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     A Create LU operation from Compress or Decompress command was attempted that would have caused
;//     the Pool's Percent Full Threshold and Harvesting High Water Mark to be exceeded.
;//
;// Solution: 
;//     Please either add more space to the Pool and rerun the command, or force the Compress or Decompress command.
;//
;// Description:
;//     %1 is the operation
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_EXCEEDS_BOTH
Language     = English
Compression could not be %1 for this LUN because the attempt to create a LUN would have caused the pool to exceed its percent full threshold and its auto-delete pool full high watermark. Please either add more space to the pool and rerun the command, or force the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     A Create LU operation from Compress or Decompress command was attempted that would have caused
;//     the Pool's Free Space to be exceeded.
;//
;// Solution: 
;//     Please add more space to the Pool and rerun the command.
;//
;// Description:
;//     %1 is the Pool ID
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_INSUFFICIENT_SPACE
Language     = English
There is not enough free space in the specified Storage Pool (ID: %1) to complete the operation. Please add available storage to the Pool and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Unable to complete the Create LU operation from Compress or Decompress command because the 
;//     maximum number of Pool LUNs already exists.
;//
;// Solution: 
;//
;// Description:
;//     %1 is the maximum number of Pool LUNs
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_EXCEEDS_MAX_COUNT
Language     = English
Unable to complete the operation; the system has already reached the limit of the maximum number (%1) of LUNs.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     Migration/Compression can not complete the operation because data synchronization is completed.
;//
;// Solution: 
;//     Please retry the operation.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_MIGRATION_SYNCHRONIZED_CANCEL
Language     = English
Compression can not complete the operation at this time. Please retry the operation later.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     An internal error has occurred.
;//
;// Solution: 
;//     Please gather SPcollects and contact your Service Provider.
;//
;// Description:
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_COMMON_ERROR
Language     = English
An internal error has occurred. Please gather SPcollects and contact your Service Provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     A Create LU operation from Compress or Decompress command was failed because the Storage Pool 
;//     is not ready.
;//
;// Solution: 
;//     Please wait for the Storage Pool to become ready and retry the operation.
;//
;// Description:
;//     %1 is the Pool ID
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_POOL_NOT_READY
Language     = English
Unable to complete the operation because the Storage Pool (ID: %1) is not ready. Please wait for the Storage Pool to become ready and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The Migration operation from Compress or Decompress command was failed because the LUN was in error condition like not Ready, Offline, Disk Full or etc.
;//
;// Solution: 
;//     Please re-enable or disable compression, verify the Storage Pool is online and has available 
;//     storage, and retry the operation. If problem persists, please gather SPCollects and contact 
;//     the Service Provider.
;//
;// Description:
;//     %1 is the Operation
;//     %2 is the Operation
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_MIGRATION_LUN_SEVERE_ERROR
Language     = English
%1 operation failed due to internal error. Please %2 compression, verify the Storage Pool is online and has available storage, and retry the operation. If problem persists, please gather SPCollects and contact your Service Provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The LUN Create operation from Compress or Decompress command was failed because the LUN was in error condition like not Ready, Offline, Disk Full or etc.
;//
;// Solution: 
;//     Please verify the Storage Pool is online and has available storage, and retry the operation. 
;//     If problem persists, please gather SPCollects and contact the Service Provider.
;//
;// Description:
;//     %1 is the Operation
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_LUN_CREATION_SEVERE_ERROR
Language     = English
%1 operation failed due to internal error. Please verify the Storage Pool is online and has available storage, and retry the operation. If problem persists, please gather SPCollects and contact your Service Provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     An attempt was made to turn compression on a LUN in a RAID Group with Power Saving enabled.
;//
;// Solution: 
;//     Please disable the Power Saving feature on the RAID Group first and then retry the operation.
;//
;// Description:
;//    
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_RG_POWERSAVINGS_ON
Language     = English
You attempted to enable compression on a LUN in a RAID Group that has the Power Saving feature enabled. Disable the Power Saving feature on the RAID Group first, and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//     The LUN Create operation from Compress command was failed because a pool LUN with the same nice name already exists.
;//
;// Solution: 
;//     Pool LUN nice names must be unique.  Please resolve the conflict by changing the nice name of one of the LUNs and retry the operation.
;//
;// Description:
;//    
;//
MessageId    = +1
Severity     = Error
Facility     = MluCompress
SymbolicName = COMPRESS_ADMIN_ERROR_NICE_NAME_EXISTS
Language     = English
You cannot enable compression on the LUN because a pool LUN with the same name already exists. Pool LUN names must be unique. Change the name of one of the LUNs, and then retry the operation.
.
 
;//==============================================================
;//======= END ADMIN DLL ERROR MSGS =============================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION COORDINATOR CRITICAL MSGS ==========
;//==============================================================
;//
;// Compression Coordinator "Critical" messages.
;//





;//==============================================================
;//======= END COMPRESSION COORDINATOR CRITICAL MSGS ============
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION ENGINE CRITICAL MSGS ===============
;//==============================================================
;//
;// Compression Engine "Critical" messages.
;//





;//==============================================================
;//======= END COMPRESSION ENGINE CRITICAL MSGS =================
;//==============================================================
;//


;//==============================================================
;//======= START COMPRESSION MANAGER CRITICAL MSGS ==============
;//==============================================================
;//
;// Compression Manager "Critical" messages.
;//





;//==============================================================
;//======= END COMPRESSION MANAGER CRITICAL MSGS ================
;//==============================================================
;//


;//==============================================================
;//======= START ADMIN DLL CRITICAL MSGS ========================
;//==============================================================
;//
;// Admin DLL "Critical" messages.
;//





;//==============================================================
;//======= END ADMIN DLL CRITICAL MSGS ==========================
;//==============================================================
;//

;#endif // __K10_MLU_COMPRESS_MESSAGES__
