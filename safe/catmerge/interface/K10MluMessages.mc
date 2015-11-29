;//++
;// Copyright (C) EMC Corporation, 2007-2014
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10MluMessages.mc
;//
;// Description:     This file contains the message catalogue for
;//                  the MLU driver.
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
;// The MLU driver further classifies its range by breaking up each CLARiiON
;// category into a sub-category for each MLU component.  The
;// second nibble represents the MLU component.  We are not using
;// the entire range of the second nibble so we can expand if the future if
;// necessary.
;//
;// +--------------------------+---------+-----+
;// | Component                |  Range  | Cnt |
;// +--------------------------+---------+-----+
;// | Common                   |  0xk0mn | 256 |
;// | Executive                |  0xk1mn | 256 |
;// | MLU Manager              |  0xk2mn | 512 |
;// | FS Manager               |  0xk4mn | 256 |
;// | Pool Manager             |  0xk5mn | 512 |
;// | Slice Manager            |  0xk7mn | 256 |
;// | Object Manager           |  0xk8mn | 256 |
;// | I/O Coordinator/Mapper   |  0xk90n | 128 |
;// | Vault Manager            |  0xk98n | 128 |
;// | Buffer Manager           |  0xka0n | 128 |
;// | C-Clamp                  |  0xka8n | 128 |
;// | MLU Admin DLL            |  0xkbmn | 256 |
;// | Recovery Manager         |  0xkcmn | 256 |
;// | VU Manager               |  0xkdmn | 256 |
;// | File Manager             |  0xkemn | 256 |
;// | Family Manager           |  0xkf0n | 128 |
;// | LAG Manager              |  0xkf8n | 128 |
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
;#ifndef __K10_MLU_MESSAGES__
;#define __K10_MLU_MESSAGES__
;
;#if defined(CSX_BV_LOCATION_KERNEL_SPACE) && defined(CSX_BV_ENVIRONMENT_WINDOWS)
;#define MLU_MSG_ID_ASSERT(_expr_) C_ASSERT((_expr_));  
;#else
;#define MLU_MSG_ID_ASSERT(_expr_)
;#endif 



MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_ERROR_CODE
                  Mlu           = 0x12d : FACILITY_MLU
                )

SeverityNames = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2 : STATUS_SEVERITY_WARNING
                  Error         = 0x3 : STATUS_SEVERITY_ERROR
                )

;//========================
;//======= INFO MSGS ======
;//========================
;//
;//

;//=====================================
;//======= START COMMON INFO MSGS ======
;//=====================================
;//
;// Common "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: Driver init started
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_DRIVER_INIT_STARTED
Language     = English
Virtual provisioning driver initialization started.
Compiled: %2
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0000L == MLU_COMMON_INFO_DRIVER_INIT_STARTED);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initialization of the driver is successful.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_DRIVER_INIT_SUCCESSFUL
Language     = English
The Virtual Provisioning driver was initialized successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Common object operation completed.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_SERVICE_OPERATION_COMPLETED
Language     = English
Internal information only. Service request completed type - %2(%3) successfully.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: MLU requested a cbfs operation.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_START_OPERATION
Language     = English
Internal information only. Operation %2 started by %3 on %4.
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0003L == MLU_COMMON_INFO_START_OPERATION);
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: MLU received callback from CBFS for the CBFS operation.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_COMPLETED_OPERATION
Language     = English
Internal information only. Operation %2 completed on %3. 
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS) 0x612D0004L == MLU_COMMON_INFO_COMPLETED_OPERATION);
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: MLU requested CBFS a cancel operation.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_CANCEL_OPERATION
Language     = English
Internal information only. Operation %2 cancelled by %3. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Event for any diagnostics messages.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_DIAGNOSTIC_LOG
Language     = English
Internal information only. %2. 
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0006L == MLU_COMMON_INFO_DIAGNOSTIC_LOG);
;

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Async operation is ready to start.
;//
MessageId    = 0x0007
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_ASYNC_OP_PREPARED
Language     = English
Internal information only. Operation %2 is ready to start.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Async operation started.
;//
MessageId    = 0x0008
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_ASYNC_OP_STARTED
Language     = English
Internal information only. Operation %2 started.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Async operation was canceled.
;//
MessageId    = 0x0009
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_ASYNC_OP_CANCELED
Language     = English
Internal information only. Operation %2 was canceled after %3 ms.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Async operation completed.
;//
MessageId    = 0x000A
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_COMMON_INFO_ASYNC_OP_COMPLETED
Language     = English
Internal information only. Operation %2 completed in %3 ms.
.

;//=====================================
;//======= END COMMON INFO MSGS ========
;//======================================
;//

;//========================================
;//======= START EXECUTIVE INFO MSGS ======
;//========================================
;//
;// Executive "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Create Pool operation initiated
;//
MessageId    = 0x0100
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_POOL_RECEIVED
Language     = English
The user has initiated the creation of Storage Pool %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0100L == MLU_EXECUTIVE_INFO_CREATE_POOL_RECEIVED);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Init Pool completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_STORAGE_POOL_INITIALIZATION_COMPLETED
Language     = English
The initialization of Storage Pool ID %2 completed successfully.
Internal Information Only. Internal Storage Pool ID %3
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Add Flu initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ADD_FLU_TO_POOL_RECEIVED
Language     = English
Internal information only. Initiated the addition of LUN with WWN %2 to Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Add Flu completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ADD_FLU_TO_POOL_COMPLETED
Language     = English
Internal information only. Addition of LUN with WWN %2 to Pool %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destroy Pool operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_POOL_RECEIVED
Language     = English
The user has initiated the destruction of Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool marked as destroying successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_POOL_INITIALIZATION_COMPLETED
Language     = English
The destruction of Storage Pool %2 initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Remove flu received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_REMOVE_FLU_FROM_POOL_RECEIVED
Language     = English
Internal information only. Initiated the removal of LUN with WWN %2 from Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Remove flu completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_REMOVE_FLU_FROM_POOL_COMPLETED
Language     = English
Internal information only. Removal of LUN with WWN %2 to Pool %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Activate flus received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ACTIVATE_FLUS_IN_POOL_RECEIVED
Language     = English
Internal information only.Initiated the activation of privates FLUs in Pool %2
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Activate flus received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ACTIVATE_FLUS_IN_POOL_COMPLETED
Language     = English
Internal information only. Activation of privates FLUs in Pool %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Shrink Pool operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SHRINK_POOL_RECEIVED
Language     = English
The user has initiated the shrinking of Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Shrink Pool operation initiated successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SHRINK_POOL_COMPLETED
Language     = English
The shrinking of Storage Pool %2 initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set Pool properties initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SET_POOL_PROPERTIES_RECEIVED
Language     = English
The user has initiated the change of one or more properties for Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set Pool properties completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SET_POOL_PROPERTIES_COMPLETED
Language     = English
The user has successfully changed one or more properties for Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Delete Pool operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_POOL_RECEIVED
Language     = English
Internal Information Only. Successfully initiated the deletion of Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Delete pool completed successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_POOL_COMPLETED
Language     = English
The deletion of Storage Pool %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Create LUN operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_LU_RECEIVED
Language     = English
The user has initiated the creation of LUN %2, WWN %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Init LUN completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_LU_INITIALIZATION_COMPLETED
Language     = English
The initialization of LUN %2, WWN %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Destroy LUN operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DESTROY_LU_RECEIVED
Language     = English
The user has initiated the destruction of LUN %2.
Internal Information Only. VU object %3, Pool object %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN destroyed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_LU_DESTRUCTION_COMPLETED
Language     = English
The destruction of LUN %2 completed successfully.
Internal Information Only. VU object %3, Pool object %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set LUN properties initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SET_LU_PROPERTIES_RECEIVED
Language     = English
The user has initiated the change of one or more properties for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set LUN properties completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SET_LU_PROPERTIES_COMPLETED
Language     = English
The user has successfully changed one or more properties for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass ownership loss received
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_OWNERSHIP_LOSS_RECEIVED
Language     = English
Internal Information Only. Trespass ownership loss received on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass ownership loss completed
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_OWNERSHIP_LOSS_COMPLETED
Language     = English
Internal Information Only. Trespass ownership loss completed on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass Execute received
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_EXECUTE_RECEIVED
Language     = English
Internal Information Only. Trespass Execute received on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass Execute completed
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_EXECUTE_COMPLETED
Language     = English
Internal Information Only. Trespass Execute completed on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass query received
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_QUERY_RECEIVED
Language     = English
Internal Information Only. Trespass Query received on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass query completed
;//                     THIS CODE IS DEPRECATED.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_TRESPASS_QUERY_COMPLETED
Language     = English
Internal Information Only. Trespass Query completed on device, Mlu Device ID %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Get Compat Mode Received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_GET_COMPAT_MODE_RECEIVED
Language     = English
Internal Information Only. Received Get Compat Mode IOCTL.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Get Compat Mode completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_GET_COMPAT_MODE_COMPLETED
Language     = English
Internal Information Only. Get Compat Mode IOCTL completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Put Compat Mode Received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_PUT_COMPAT_MODE_RECEIVED
Language     = English
Internal Information Only. Received Put Compat Mode IOCTL.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Put Compat Mode completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_PUT_COMPAT_MODE_COMPLETED
Language     = English
Internal Information Only. Put Compat Mode IOCTL completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Shutdown Received
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SHUTDOWN_RECEIVED
Language     = English
Internal Information Only. Received Shutdown IOCTL.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pending shutdown
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_PENDING_SHUTDOWN
Language     = English
Internal Information Only. Pending Shutdown.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Shutdown completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_SHUTDOWN_COMPLETED
Language     = English
Internal Information Only. Shutdown IOCTL completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Unload driver
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_UNLOAD_DRIVER
Language     = English
Internal Information Only. Unloading Driver.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Driver Unloaded
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_UNLOAD_DRIVER_COMPLETED
Language     = English
Internal Information Only. All the components unloaded successfully. Driver unloaded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Driver Unload failed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_UNLOAD_DRIVER_FAILED
Language     = English
Internal Information Only. Failed to unload driver.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initiate Pool Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_RECOVER_POOL_RECEIVED
Language     = English
Request received to recover Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initiated Pool Recovery successfully
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_RECOVER_POOL_INITIATED
Language     = English
A manual recovery of Storage Pool %2 has been initiated and is currently running in the background.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort Pool Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_POOL_RECEIVED
Language     = English
Request received to cancel recovery for Storage Pool %2.
Internal information only. Storage Pool recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Aborted Pool Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_POOL_COMPLETED
Language     = English
Cancel recovery for Storage Pool %2 initiated successfully.
Internal information only. Storage Pool recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Throttle Pool Recovery Process
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_THROTTLE_RECOVER_POOL_RECEIVED
Language     = English
Request received to throttle recovery for Storage Pool %2.
Internal information only. Storage Pool recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Throttled Pool Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_THROTTLE_RECOVER_POOL_COMPLETED
Language     = English
Throttle recovery of Storage Pool %2 initiated successfully.
Internal information only. Storage Pool recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Delete Pool Recovery Process
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_POOL_RECEIVED
Language     = English
Internal information only. Request received to delete recovery process %3 for Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Deleted Pool Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_POOL_COMPLETED
Language     = English
Internal information only. Delete recovery process %3 for Storage Pool %2 initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Clear Pool Recovery Flag
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CLEAR_POOL_RECOVERFLAG_RECEIVED
Language     = English
Internal information only. Request received to clear recovery required flag on Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Cleared Pool Recovery Flag
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CLEAR_POOL_RECOVERFLAG_COMPLETED
Language     = English
Internal information only. Cleared recovery flag on Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initiate lu Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_RECOVER_LU_RECEIVED
Language     = English
Request received to recover LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initiated lu Recovery successfully
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_RECOVER_LU_INITIATED
Language     = English
A manual recovery of LUN %2 has been initiated and is currently running in the background.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Abort lu Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_LU_RECEIVED
Language     = English
Request received to cancel recovery for LUN %2.
Internal information only. LUN recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Aborted LUN Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ABORT_RECOVER_LU_COMPLETED
Language     = English
Cancel recovery of LUN %2 initiated successfully.
Internal information only. LUN recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Throttle LUN Recovery Process
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_THROTTLE_RECOVER_LU_RECEIVED
Language     = English
Request received to throttle recovery of LUN %2.
Internal information only. LUN recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Throttled lu Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_THROTTLE_RECOVER_LU_COMPLETED
Language     = English
Throttled recovery of LUN %2 successfully.
Internal information only. LUN recovery process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Delete lu Recovery Process
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_LU_RECEIVED
Language     = English
Internal information only. Request received to delete recovery process %3 for LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Deleted lu Recovery successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_DELETE_RECOVER_LU_COMPLETED
Language     = English
Internal information only. Delete recovery process %3 for LUN %2 initiated successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Clear lu Recovery Flag
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CLEAR_LU_RECOVERFLAG_RECEIVED
Language     = English
Internal information only. Request received to clear recovery flag on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Cleared lu Recovery Flag
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CLEAR_LU_RECOVERFLAG_COMPLETED
Language     = English
Internal information only. Cleared recovery flag on LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Ack lu Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ACK_RECOVER_LU_RECEIVED
Language     = English
Request received to acknowledge recovery of LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Acked lu Recovery
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_ACK_RECOVER_LU_COMPLETED
Language     = English
Acknowledged recovery of LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Clear "Cache Dirty" initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CLEAR_CACHE_DIRTY_RECEIVED
Language     = English
A request has been received to clear the Cache Dirty status of the LUN with ID %2, which belongs to a Pool.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: A FLU cleared its "Cache Dirty" status
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_FLU_CLEARED_CACHE_DIRTY_STATUS
Language     = English
The LUN with ID %2 has cleared its previous Cache Dirty status. The Pool to which
that LUN belongs, as well as all LUNs in that pool, have been marked as needing recovery.
To use those LUNs again, run recovery on those items.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set LUN Generic Property initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_PUT_GENERIC_PROPERTIES_RECEIVED
Language     = English
Request received to change the name of LUN %2 to %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Set LUN Generic Property completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_PUT_GENERIC_PROPERTIES_COMPLETED
Language     = English
The user has successfully changed the name of LUN %2 from %3 to %4.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request received to initiate Slice Relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_START_REQUEST_RECEIVED_OBSOLETE
Language     = English
Internal Information: Relocation request received for %2. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Slice Relocation initiated successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_START_REQUEST_COMPLETED_OBSOLETE
Language     = English
Internal Information: Relocation %2 request initiated successfully for %3 (Slice Offset %4 FLU %5)
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request received to initiate Slice Relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_SETSLICE_REQUEST_RECEIVED
Language     = English
Internal Information: Set slice information request received for %2. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Slice Relocation initiated successfully
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_SETSLICE_REQUEST_COMPLETED
Language     = English
Internal Information: Slice information set for Relocation %2
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request received to throttel slice relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_THROTTLE_REQUEST_RECEIVED
Language     = English
Internal Information: Request received to throttle %2 relocation %3. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Successfully throttled slice relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_THROTTLE_REQUEST_COMPLETED
Language     = English
Internal Information: Successfully throttle slice relocation %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Request received to abort slice relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_ABORT_REQUEST_RECEIVED_OBSOLETE
Language     = English
Internal Information: Request received to abort relocation %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Successfully aborted slice relocation
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_ABORT_REQUEST_COMPLETED
Language     = English
Internal Information: Request initiated to abort relocation %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_DESTROY_REQUEST_RECEIVED_OBSOLETE
Language     = English
Internal Information: Request received to destroy relocation %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_RELOCATE_INFO_DESTROY_REQUEST_COMPLETED_OBSOLETE
Language     = English
Internal Information: Request initiated to destroy relocation %2.
.

;// --------------------------------------------------
;// Introduced In: R31
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Create LUN operation failed due to insufficient storage pool space.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_CREATE_LU_RESERVE_SLICES_FAILED
Language     = English
LUN creation failed due to insufficient space in the storage pool. Request LUN size is %2 MB which is %3 MB more than is available.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_WITHIN_SAFE_OPERATING_CAPACITY_OBSOLETE
Language     = English
The array is now operating within its Safe Operating Capacity.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Used for CBFS informational message
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_FROM_CBFS
Language     = English
Internal information only. CBFS Event: %2, Msg: %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D014CL == MLU_EXECUTIVE_INFO_FROM_CBFS);


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass Ownership Loss ioctl received
;//
MessageId    = 0x014D
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_LUN_RECEIVED_TRESPASS_OWNERSHIP_LOSS
Language     = English
Internal Information Only. Trespass Ownership Loss received on LUN %2 (object ID %3, WWN %4) in pool %5 (object ID %6).
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass Execute ioctl received
;//
MessageId    = 0x014E
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_LUN_RECEIVED_TRESPASS_EXECUTE
Language     = English
Internal Information Only. Trespass Execute received on LUN %2 (object ID %3, WWN %4) in pool %5 (object ID %6).
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Trespass Query ioctl received
;//
MessageId    = 0x014F
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_LUN_RECEIVED_TRESPASS_QUERY
Language     = English
Internal Information Only. Trespass Query received on LUN %2 (object ID %3, WWN %4) in pool %5 (object ID %6).
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: NDMP Snapshot created
;//
MessageId    = 0x0150
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_NDMP_SNAPSHOT_CREATED
Language     = English
An NDMP snapshot %2 was internally created for file system %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: NDMP Snapshot destroyed
;//
MessageId    = 0x0151
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_INFO_NDMP_SNAPSHOT_DESTROYED
Language     = English
NDMP snapshot %2 was internally destroyed.
.

;
;//========================================
;//======= END EXECUTIVE INFO MSGS ========
;//========================================
;//


;//==========================================
;//======= START MLU MANAGER INFO MSGS ======
;//==========================================
;//
;// MLU Manager "Information" messages.
;//


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: TLU name changed
;//
MessageId    = 0x0200
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_MLUMGR_INFO_CHANGED_MLU_NAME_OBSOLETE
Language     = English
The name of the Thin LUN %2 has been changed to %3 from %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0200L == MLU_MLUMGR_INFO_CHANGED_MLU_NAME_OBSOLETE);
;
;//==========================================
;//======= END MLU MANAGER INFO MSGS ========
;//==========================================

;//=========================================
;//======= START FS MANAGER INFO MSGS ======
;//=========================================
;//
;// File System Manager "Information" messages.
;//

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: A slice add operation is ongoing for the given FS
;//
MessageId    = 0x0400
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_SLICE_ADD_ALREADY_IN_PROGRESS
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0400L == MLU_FSMGR_INFO_SLICE_ADD_ALREADY_IN_PROGRESS);
;
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: Slice add operation was successfully started.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_SLICE_ADD_INITIATED
Language     = English
.

;//
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: A process object that is already in queue waiting for
;//                     permission to run an operation from an Exclusive CBFS
;//                     family has requested for token again.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_FAMILY_ALREADY_QUEUED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0402L == MLU_FSMGR_INFO_FAMILY_ALREADY_QUEUED);
;
;//=========================================
;//======= END FS MANAGER INFO MSGS ========
;//=========================================
;//

;//===========================================
;//== START OBSOLETE POOL MANAGER INFO MSGS ==
;//===========================================
;//
;// Pool Manager "Information" messages.
;//
;// These messages should being at 0x500, but they didn't in R28, so
;// they cannot be moved without changing documentation for existing releases.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool description changed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_CHANGED_POOL_DESCRIPTION_OBSOLETE
Language     = English
The description of Storage Pool %2 has been changed to %3 from %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0403L == MLU_POOLMGR_INFO_CHANGED_POOL_DESCRIPTION_OBSOLETE);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Percent Full Threshold changed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_CHANGED_POOL_PERCENT_FREE_THRESHOLD_OBSOLETE
Language     = English
The percent full threshold of Storage Pool %2 has been changed to %3 from %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Expand Pool operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_EXPAND_POOL_RECEIVED_OBSOLETE
Language     = English
The user has initiated the expansion of Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Expansion of Storage Pool started
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_EXPANSION_STARTED_OBSOLETE
Language     = English
The expansion of Storage Pool %2 has started.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Storage Pool expanded
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_EXPANSION_COMPLETED_OBSOLETE
Language     = English
The expansion of Storage Pool %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: FLU online event from Flare
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_FLU_ONLINE_EVENT_FROM_FLARE_OBSOLETE
Language     = English
Internal information only. LUN %2 in Pool %3 is now online.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: FLU RAID protection upgraded event from Flare
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_FLU_RAID_PROTECTION_UPGRADED_OBSOLETE
Language     = English
Internal information only. RAID protection upgraded for FLU. FLU WWN %2 FLU OID %3 Pool OID %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool RAID protection upgraded.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_RAID_PROTECTION_UPGRADED_OBSOLETE
Language     = English
RAID protection has been upgraded for Storage Pool %2. Internal information only. Pool OID %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool is now online as all FLUs in the pool are reported to be online.
;//
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_ONLINE_OBSOLETE
Language     = English
Storage Pool %2 is now functioning correctly. Internal information only. Pool OID %3 was reported online.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Remove flu completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_REMOVE_FLU_FROM_POOL_COMPLETED_OBSOLETE
Language     = English
Internal information only. Removal of LUN with WWN %2 to Pool %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool free space has been increased
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_FREE_SPACE_INCREASED_OBSOLETE
Language     = English
Storage pool %2 free space has been increased to %3 percent.
.

;

;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: A synchronous unmount failure occurred due to the FS object
;//			being reset on the way to disabled such that the plumber
;//			stack quiesce ran after the unmount and allowed a mapping
;//			callback to issued to the FS right before the unmount.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_UNMOUNT_FAILED_DUE_TO_RESET_OF_FSOBJ
Language     = English
Internal Information Only.  FS %2 failed to unmount because it was reset on the way to disabled.
.

;
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D040EL == MLU_FSMGR_INFO_UNMOUNT_FAILED_DUE_TO_RESET_OF_FSOBJ);

;//===========================================
;//=== END OBSOLETE POOL MANAGER INFO MSGS ===
;//===========================================
;//


;//===========================================
;//======= START POOL MANAGER INFO MSGS ======
;//===========================================
;//
;// Pool Manager "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool name changed
;//
MessageId    = 0x0500
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_CHANGED_POOL_NAME
Language     = English
The name of Storage Pool %2 has been changed to %3 from %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0500L == MLU_POOLMGR_INFO_CHANGED_POOL_NAME);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool description changed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_CHANGED_POOL_DESCRIPTION
Language     = English
The description of Storage Pool %2 has been changed to %3 from %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Percent Full Threshold changed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_CHANGED_POOL_PERCENT_FREE_THRESHOLD
Language     = English
The percent full threshold of Storage Pool %2 has been changed to %3 from %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Expand Pool operation initiated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_EXPAND_POOL_RECEIVED
Language     = English
The user has initiated the expansion of Storage Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Expansion of Storage Pool started
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_EXPANSION_STARTED
Language     = English
The expansion of Storage Pool %2 has started.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Storage Pool expanded
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_EXPANSION_COMPLETED
Language     = English
The expansion of Storage Pool %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: FLU online event from Flare
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_FLU_ONLINE_EVENT_FROM_FLARE
Language     = English
Internal information only. LUN %2 in Pool %3 is now online.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: FLU RAID protection upgraded event from Flare
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_FLU_RAID_PROTECTION_UPGRADED
Language     = English
Internal information only. RAID protection upgraded for FLU. FLU WWN %2 FLU OID %3 Pool OID %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool RAID protection upgraded.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_RAID_PROTECTION_UPGRADED
Language     = English
RAID protection has been upgraded for Storage Pool %2. Internal information only. Pool OID %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool is now online as all FLUs in the pool are reported to be online.
;//
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_ONLINE
Language     = English
Storage Pool %2 is now functioning correctly. Internal information only. Pool OID %3 was reported online.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Remove flu completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_REMOVE_FLU_FROM_POOL_COMPLETED
Language     = English
Internal information only. Removal of LUN with WWN %2 to Pool %3 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool free space has been increased
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_STORAGE_POOL_FREE_SPACE_INCREASED
Language     = English
Storage pool %2 free space has been increased to %3 percent.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Calculated usable pool space is less than the persistent value.
;//                     due to offline FLUs.  The value will be corrected when the FLUs
;//			come back online.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_USABLE_SPACE_WOULD_HAVE_DECREASED
Language     = English
Internal information only. Storage pool %2 usable space in blocks. Old = %3 Calculated = %4
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool usable space has been increased
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_USABLE_SPACE_INCREASE
Language     = English
Internal information only. Storage pool %2 usable space has been increased from %3 to %4 blocks.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D050DL == MLU_POOLMGR_INFO_POOL_USABLE_SPACE_INCREASE);
 
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Pre-Alloc Info Update
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_PREALLOC_UPDATE
Language     = English
Internal information only. Storage pool %2 pre-alloc info update: Request = %3, Recorded = %4, IsComplete = %5.
.

;
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D050EL == MLU_POOLMGR_INFO_POOL_PREALLOC_UPDATE);

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Pre-Alloc Info Update
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_POOL_PREALLOC_UPDATED_BY_SESSION
Language     = English
Internal information only. Storage pool %2 pre-alloc info update: Request = %3, Recorded = %4.
.

;
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D050FL == MLU_POOLMGR_INFO_POOL_PREALLOC_UPDATED_BY_SESSION);


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log only
;//
;// Severity: Informational
;//
;// Symptom of Problem: System Pool has gone offline
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_POOLMGR_INFO_NO_LONGER_USED
Language     = English
This event code is no longer used.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0510 == MLU_POOLMGR_INFO_NO_LONGER_USED);
;

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Space reclamation has been disabled on a file system.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_FS_SPACE_RECLAMATION_DISABLED
Language     = English
Internal information only. Space reclamation has been disabled for File System object %2.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Space reclamation has been enabled on a file system.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FSMGR_INFO_FS_SPACE_RECLAMATION_ENABLED
Language     = English
Internal information only. Space reclamation has been enabled for File System object %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0512L == MLU_FSMGR_INFO_FS_SPACE_RECLAMATION_ENABLED);

;//===========================================
;//======= END POOL MANAGER INFO MSGS ========
;//===========================================
;//

;//============================================
;//======= START SLICE MANAGER INFO MSGS ======
;//============================================
;//
;// Slice Manager "Information" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: List Slices has returned the last slice.
;//
MessageId    = 0x0700
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_INFO_NO_MORE_SLICES
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0700L == MLU_SLICE_MANAGER_INFO_NO_MORE_SLICES);
;
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: Last resource reached in a collection while enumerating.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_END_REACHED_IN_THE_COLLECTION
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: List Slices could not get slice information because it does not have the permit
;//                     
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_INFO_FULL_INFO_NOT_AVAILABLE
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: Owner is paused. Any operations must be paused
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_OPERATION_PAUSED
Language     = English
.


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: The FLU is not owned by any SPs and does not have any slices allocated
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_INFO_NO_ALLOCATED_SLICES
Language     = English
.


;// --------------------------------------------------
;// Introduced In: R31
;//
;// Usage: Event Log and Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: Slice zerofill not complete
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_INFO_SLICER_ZEROFILL_PENDING
Language     = English
Internal information only. Zero fill of FLU %2, Offset %3 is not yet complete after 5 minutes.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0705L == MLU_SLICE_MANAGER_INFO_SLICER_ZEROFILL_PENDING);


;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Internal MLU use only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Slice allocation group destruction can't start yet.
;//
MessageId    = 0x0706
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_INFO_SLICE_ALLOCATION_GROUP_DESTRUCTION_CANT_START
Language     = English
Internal Information Only. The destruction of the slice allocation group can't start yet.
.

;
;//============================================
;//======= END SLICE MANAGER INFO MSGS ========
;//============================================
;//

;//=============================================
;//======= START OBJECT MANAGER INFO MSGS ======
;//=============================================
;//
;// Object Manager "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Wait dependency exists while trying to create it.
;//
MessageId    = 0x0800
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_WAIT_DEPENDENCY_EXISTS
Language     = English
Internal Information Only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0800L == MLU_OBJMGR_INFO_WAIT_DEPENDENCY_EXISTS);
;
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Wait dependency not removed because it does not exist.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_WAIT_DEPENDENCY_DOES_NOT_EXIST
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Existence dependency not removed because it does not exist.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_EXISTENCE_DEPENDENCY_DOES_NOT_EXIST
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: There are no more objects left to be returned in the current iteration.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_NO_MORE_OBJECTS
Language     = English
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: There are no more objects left to be returned in the current iteration.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_NO_MORE_RELATIONSHIPS
Language     = English
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: The requested existence dependency already exists.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_EXISTENCE_DEPENDENCY_EXISTS
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: The object that was asked to be deactivated is already deactivated on this SP.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_OBJECT_ALREADY_DEACTIVATED
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: The requested operation caused the SP to degrade.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_SP_DEGRADED
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: The object that was requested to be created already exists.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_OBJECT_EXISTS
Language     = English
Internal Information Only.
.

;//
;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: The object activation request is pending.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_OBJECT_ACTIVATION_PENDING
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Usage
;//
;// Severity: Informational
;//
;// Symptom of Problem: Group member relationship exists while trying to create it.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_GROUPMGR_INFO_MEMBER_RELATIONSHIP_EXISTS
Language     = English
Internal Information Only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D080AL == MLU_GROUPMGR_INFO_MEMBER_RELATIONSHIP_EXISTS);


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage: Internal
;//
;// Severity: Informational
;//
;// Symptom of Problem: Success - nothing to do
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_OBJMGR_INFO_OBJECT_ALREADY_ACTIVATED
Language     = English
Internal only. 
.


;//=============================================
;//======= END OBJECT MANAGER INFO MSGS ========
;//=============================================
;//

;//=====================================================
;//======= START I/O COORDINATOR/MAPPER INFO MSGS ======
;//=====================================================
;//
;// I/O Coordinator/Mapper "Information" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Unused
;//
;// Severity: Informational
;//
;// Symptom of Problem:  STATUS_PARTIAL_READ_COMPLETE moved to flare_ioctls.h
;//
MessageId    = 0x0900
Severity     = Informational
Facility     = Mlu
SymbolicName = STATUS_PARTIAL_READ_COMPLETE_OBSOLETE
Language     = English
Internal Information Only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0900L == STATUS_PARTIAL_READ_COMPLETE_OBSOLETE);
;
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_INFO_RETRY_NOW
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_INFO_RETRY_LATER
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_INFO_ABORT_XCHANGE_AND_WAIT
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS returned an EndOfOp status
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_INFO_END_OF_OP
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_SEARCH_TERMINATED_EARLY
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_SEARCH_TERMINATED_EOF
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_SEARCH_EOF_WITH_DATA
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_EMPTY_MAPPING
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_GENERATION_MISMATCH
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_GENERATION_MISMATCH_KEY0
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_GENERATION_MISMATCH_KEY1
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_GENERATION_MISMATCH_BOTH
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_INVALID_BLOCK_KEY
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_OK_NO_OP
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS DeDup Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_DD_OK_BLOCK_FREED
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Info
;//
;// Symptom of Problem: CBFS Error/Info Messages
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_INFO_MAPPING_PAUSED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0910L == MLU_IO_MAPPER_INFO_MAPPING_PAUSED);
;
;//=====================================================
;//======= END I/O COORDINATOR/MAPPER INFO MSGS ========
;//=====================================================
;//

;//============================================
;//======= START VAULT MANAGER INFO MSGS ======
;//============================================
;//
;// Vault Manager "Information" messages.
;//

;//============================================
;//======= END VAULT MANAGER INFO MSGS ========
;//============================================
;//

;//=============================================
;//======= START BUFFER MANAGER INFO MSGS ======
;//=============================================
;//
;// Buffer Manager "Information" messages.
;//

;//=============================================
;//======= END BUFFER MANAGER INFO MSGS ========
;//=============================================
;//

;//======================================
;//======= START C-CLAMP INFO MSGS ======
;//======================================
;//
;// C-Clamp "Information" messages.
;//
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:  The Logical Volume does not have a quiesce started.
;//
MessageId    = 0x0A80
Severity     = Informational
Facility     = Mlu
SymbolicName = STATUS_SEND_QUIESCE_TO_LOGICAL_VOLUME
Language     = English
Internal Information Only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0A80L == STATUS_SEND_QUIESCE_TO_LOGICAL_VOLUME);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Only
;//
;// Severity: Informational
;//
;// Symptom of Problem:  Fixture acknowledged Cancel and will not reference the IOD data buffer.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_FIXTURE_CANCELLATION_ACKNOWLEDGED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Initialization of disk performance parameters is successful.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_DISK_PER_STRIP_USER_VALUE_INIT_SUCCESSFUL
Language     = English
The Virtual Provisioning user-defined disk-per-stripe values were entered successfully.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0A82L == MLU_DISK_PER_STRIP_USER_VALUE_INIT_SUCCESSFUL);
;
;//======================================
;//======= END C-CLAMP INFO MSGS ========
;//======================================
;//

;//======================================
;//======= START MLU ADMIN INFO MSGS ======
;//======================================
;//
;// MLU ADMIN "Information" messages.
;//

;//======================================
;//======= END MLU ADMIN INFO MSGS ========
;//======================================
;//

;//=======================================
;//== START RECOVERY MANAGER INFO MSGS  ==
;//=======================================
;//
;// RECOVERY MANAGER "Information" messages.
;//

;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Volume recovery complete.
;//
MessageId    = 0x0c00
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_VOLUME_RECOVERY_COMPLETE
Language     = English
Internal Information Only. Volume recovery completed for pool %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0C00L == MLU_RM_INFO_VOLUME_RECOVERY_COMPLETE);
;
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN recovery completed successfully.
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_LUN_RECOVERY_COMPLETED_SUCCESSFULLY
Language     = English
Recovery of LUN %2 has completed successfully.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Recovery Required flag was manually cleared.
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_POOL_RECOVERY_REQUIRED_FLAG_CLEARED
Language     = English
Storage pool %2 was manually modified to no longer require recovery. An attempt will be made to bring the pool back online.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Recovery Required flag was manually set.
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_POOL_RECOVERY_REQUIRED_FLAG_SET
Language     = English
Storage pool %2 was manually modified to require recovery and will be taken offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool Recovery Recommended flag was set.
;//
MessageId    = +1
;//SSPG C4type=System
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_POOL_RECOVERY_RECOMMENDED_FLAG_SET
Language     = English
Storage pool %2 was manually modified to require recovery later. The pool will remain online but will be reported in a degraded state until recovery is performed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LU Recovery Required flag was manually cleared.
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_LU_RECOVERY_REQUIRED_FLAG_CLEARED
Language     = English
LUN %2 was manually modified to no longer require recovery. An attempt will be made to bring the LUN back online.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LU Recovery Required flag was manually set.
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_LU_RECOVERY_REQUIRED_FLAG_SET
Language     = English
LUN %2 was manually modified to require recovery and will be taken offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LU Recovery Recommended flag was set.
;//
MessageId    = +1
;//SSPG C4type=System
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_LU_RECOVERY_RECOMMENDED_FLAG_SET
Language     = English
The Recovery Recommended flag for LUN %2 was set.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Starting Recovery of File System.
;//
MessageId    = +1
;//SSPG C4type=User
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_FS_RECOVERY_STARTED
Language     = English
A manual recovery of file system %2 has been initiated and is currently running in the background.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Recovered LU Object has been brought online.
;//
MessageId    = +1
;//SSPG C4type=System
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_RECOVERED_LU_OBJECT_BROUGHT_ONLINE
Language     = English
Recovered LUN object %2 has been brought online.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Recovered FS has been brought online.
;//
MessageId    = +1
;//SSPG C4type=System
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_RECOVERED_FS_BROUGHT_ONLINE
Language     = English
Recovered File System %2 has been brought online.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File System recovery required flag was cleared.
;//
MessageId    = +1
;//SSPG C4type=System
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_FS_RECOVERY_REQUIRED_FLAG_CLEARED
Language     = English
Recovery required flag for File System %2 has been cleared.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File System Recovery Required flag was set.
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_FS_RECOVERY_REQUIRED_FLAG_SET
Language     = English
File system %2 was manually modified to require recovery and will be taken offline.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Audit Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File System Recovery Recommended flag was set.
;//
;//
MessageId    = +1
;//SSPG C4type=Audit
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_FS_RECOVERY_RECOMMENDED_FLAG_SET
Language     = English
File system %2 was manually modified to require recovery later. The file system will remain online but will be reported in a degraded state until recovery is performed.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN recovery failed.
;//                     (LUN is the "user" term for what is internally called a File System)
MessageId    = +1
Severity     = Informational
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_RM_INFO_LUN_RECOVERY_FAILED_USER
Language     = English
Recovery of LUN %2 has failed. Please take a Data Collection and contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For System Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: File system recovery failed.
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_INFO_FS_RECOVERY_FAILED
Language     = English
Recovery of file system %2 has failed with status %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool recovery failed
;//
MessageId    = +1
Severity     = Informational
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_RM_INFO_POOL_RECOVERY_FAILED
Language     = English
Recovery of pool %2 has failed. Please take a Data Collection and contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Pool recovery completed successfully
;//
MessageId    = +1
Severity     = Informational
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_RM_INFO_POOL_RECOVERY_COMPLETED_SUCCESSFULLY
Language     = English
Recovery of pool %2 has completed successfully.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0C11L == MLU_RM_INFO_POOL_RECOVERY_COMPLETED_SUCCESSFULLY );
;
;//======================================
;//==  END RECOVERY MANAGER INFO MSGS  ==
;//======================================
;//

;//======================================
;//======= START VU MANAGER INFO MSGS ===
;//======================================
;//
;// VU Manager "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN name changed
;//
MessageId    = 0x0D00
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_VUMGMT_INFO_CHANGED_MLU_NAME
Language     = English
The name of the LUN %2 has been changed to %3 from %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0D00L == MLU_VUMGMT_INFO_CHANGED_MLU_NAME);
;
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN is online
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_VUMGMT_INFO_LU_ONLINE
Language     = English
LUN %2 is ready to service IO. Internal information only:LU OID %3 Pool OID %4.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: LUN has at least one poached slice
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_VUMGMT_INFO_LU_HAS_POACHED_SLICES
Language     = English
Internal information only. LUN %2 contains poached slices. LUN allocation SP is %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0D02L == MLU_VUMGMT_INFO_LU_HAS_POACHED_SLICES);
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: User Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: System LU has gone offline
;//
MessageId    = +1
Severity     = Informational
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_VUMGMT_INFO_SYSTEM_LU_OFFLINE
Language     = English
System LUN %2 is offline and all of its services are unavailable. Please take a Data Collection and contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D0D03L == MLU_VUMGMT_INFO_SYSTEM_LU_OFFLINE);
;


;//======================================
;//======= END VU MANAGER INFO MSGS =====
;//======================================
;//

;//======================================
;//======= START FILE MANAGEMENT INFO MSGS =
;//======================================

;//
;// FILE Management "Information" messages.
;//


;//======================================
;//======= END FILE MANAGEMENT INFO MSGS ===
;//======================================
;//


;//======================================
;//======= START FAMILY MANAGEMENT INFO MSGS =
;//======================================

;//
;// FAMILY Management "Information" messages.
;//


;//======================================
;//======= END FAMILY MANAGEMENT INFO MSGS ===
;//======================================
;//

;//=============================================
;//======= START COMPRESSION MANAGER INFO MSGS =
;//=============================================
;//
;// Compression Manager "Information" messages.
;//

;//===========================================
;//======= END COMPRESSION MANAGER INFO MSGS =
;//===========================================
;//


;//===========================
;//======= WARNING MSGS ======
;//===========================
;//
;//

;//========================================
;//======= START COMMON WARNING MSGS ======
;//========================================
;//
;// Common "Warning" messages.
;//

;//========================================
;//======= END COMMON WARNING MSGS ========
;//========================================
;//

;//===========================================
;//======= START EXECUTIVE WARNING MSGS ======
;//===========================================
;//
;// Executive "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
MessageId    = 0x4100
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_WARNING_UNSAFE_OPERATING_CAPACITY_OBSOLETE
Language     = English
The array is no longer operating within its Safe Operating Capacity. Shrink or delete storage pools and LUs in storage pools until the array reports that it is once again within its Safe Operating Capacity. 
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4100L == MLU_EXECUTIVE_WARNING_UNSAFE_OPERATING_CAPACITY_OBSOLETE);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Used for CBFS Warning messages
;//
MessageId    = +1
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_WARNING_FROM_CBFS
Language     = English
Internal information only. Event: %2, Msg: %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4101L == MLU_EXECUTIVE_WARNING_FROM_CBFS);
;
;//===========================================
;//======= END EXECUTIVE WARNING MSGS ========
;//===========================================
;//

;//=============================================
;//======= START MLU MANAGER WARNING MSGS ======
;//=============================================
;//
;// MLU Manager "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: MLU RAID protection is degraded
;//
;//
MessageId    = 0x4200
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_MLUMGR_WARNING_MLU_RAID_PROTECTION_DEGRADED_OBSOLETE
Language     = English
Thin LUN %2 has detected a fault but will continue to service IO. Please check for hardware
faults such as disk drive. Internal information only. TLU OID %3 Pool OID %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4200L == MLU_MLUMGR_WARNING_MLU_RAID_PROTECTION_DEGRADED_OBSOLETE);
;
;//=============================================
;//======= END MLU MANAGER WARNING MSGS ========
;//=============================================
;//

;//============================================
;//======= START FS MANAGER WARNING MSGS ======
;//============================================
;//
;// File System Manager "Warning" messages.
;//

;//============================================
;//======= END FS MANAGER WARNING MSGS ========
;//============================================
;//

;//==============================================
;//======= START POOL MANAGER WARNING MSGS ======
;//==============================================
;//
;// Pool Manager "Warning" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Storage pool space is at or above user specified percent full threshold
;//
;//
MessageId    = 0x4600
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_POOLMGR_WARNING_STORAGE_POOL_AT_BELOW_FREE_SPACE_THRESHOLD
Language     = English
The storage pool %2 has %3 percent free space left.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4600L == MLU_POOLMGR_WARNING_STORAGE_POOL_AT_BELOW_FREE_SPACE_THRESHOLD);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: FLU RAID protection degraded event from FLARE
;//
;//
MessageId    = +1
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_POOLMGR_WARNING_FLU_RAID_PROTECTION_DEGRADED
Language     = English
Internal information only. FLU RAID protection is degraded. FLU WWN %2 FLU OID %3 Pool OID %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Pool RAID protection is degraded.
;//
;//
MessageId    = +1
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_POOLMGR_WARNING_POOL_RAID_PROTECTION_DEGRADED
Language     = English
RAID protection for Storage Pool %2 is degraded. Please resolve any hardware problems.
Internal information only: Pool OID %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4602L == MLU_POOLMGR_WARNING_POOL_RAID_PROTECTION_DEGRADED);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Recovery scratch space information could not be built in a pool.
;//
;//
MessageId    = +1
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_POOLMGR_WARNING_REBUILD_SCRATCH_SPACE_FS_INFO_FAILURE
Language     = English
Recovery scratch space information could not be built in Storage Pool %2, status %3.
Internal information only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4603L == MLU_POOLMGR_WARNING_REBUILD_SCRATCH_SPACE_FS_INFO_FAILURE);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Recovery scratch slices could not be pre-reserved in a pool.
;//
;//
MessageId    = +1
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_POOLMGR_WARNING_PRE_RESERVE_SCRATCH_SLICE_FAILURE
Language     = English
Recovery scratch slices could not be pre-reserved in Storage Pool %2, status %3.
Internal information only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4604L == MLU_POOLMGR_WARNING_PRE_RESERVE_SCRATCH_SLICE_FAILURE);
;
;
;//==============================================
;//======= END POOL MANAGER WARNING MSGS ========
;//==============================================
;//

;//===============================================
;//======= START SLICE MANAGER WARNING MSGS ======
;//===============================================
;//
;// Slice Manager "Warning" messages.
;//

;//===============================================
;//======= END SLICE MANAGER WARNING MSGS ========
;//===============================================
;//

;//================================================
;//======= START OBJECT MANAGER WARNING MSGS ======
;//================================================
;//
;// Object Manager "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Some object identifiers passed by peer SP were invalid.
;//
MessageId    = 0x4800
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_INVALID_OBJECT_IDS_FOUND
Language     = English
Internal Information Only.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4800L == MLU_OBJMGR_WARNING_INVALID_OBJECT_IDS_FOUND);
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Local Instantiation in a Remit Table Objects Message didn't match 
;//                     the Global Instantiation in MluObjMgrODBReflectionProcessRemitTableObjectsRequest(). 
;//
MessageId    = 0x4801
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_INSTANTIATION_MISMATCH
Language     = English
Internal Information Only.
Remit Table Objects processing instantiation mismatch Table %2 Global %3 Message %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4801L == MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_INSTANTIATION_MISMATCH);
;
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Xaction Id in a Remit Table Objects Message didn't match 
;//                     the Global Xaction Id in MluObjMgrODBReflectionProcessRemitTableObjectsRequest(). 
;//
MessageId    = 0x4802
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_XACTION_ID_MISMATCH
Language     = English
Internal Information Only.
Remit Table Objects processing transaction mismatch Table %2 Global %3 Message %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4802L == MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_XACTION_ID_MISMATCH);
;
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Table Id in a Remit Table Objects Message didn't match 
;//                     the Global Table Id in MluObjMgrODBReflectionProcessRemitTableObjectsRequest(). 
;//
MessageId    = 0x4803
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_TABLE_MISMATCH
Language     = English
Internal Information Only.
Remit Table Objects processing table id mismatch Table %2 Global %3 Message %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4803L == MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_TABLE_MISMATCH);
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Remitted Object Count in a Remit Table Objects Message didn't match 
;//                     the Global Remitted Object Count in MluObjMgrODBReflectionProcessRemitTableObjectsRequest(). 
;//
MessageId    = 0x4804
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_REMITTED_COUNT_MISMATCH
Language     = English
Internal Information Only.
Remit Table Objects processing remitted object count mismatch Table %2 Global %3 Message %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4804L == MLU_OBJMGR_WARNING_PROC_REMIT_TABLE_OBJECTS_REMITTED_COUNT_MISMATCH);
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Local Instantiation in an Is Transacton Active Message didn't match 
;//                     the Global Instantiation in MluObjMgrODBReflectionProcessIsTransactionActiveOnPeerRequest(). 
;//
MessageId    = 0x4805
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_IS_XACTION_ACTIVE_INSTANTIATION_MISMATCH
Language     = English
Internal Information Only.
Is Transaction Active processing instantiation mismatch Global %2 Message %3.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4805L == MLU_OBJMGR_WARNING_IS_XACTION_ACTIVE_INSTANTIATION_MISMATCH);
;
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Xaction Id in an Is Transacton Active message didn't match 
;//                     the Global Xaction Id in MluObjMgrODBReflectionProcessIsTransactionActiveOnPeerRequest(). 
;//
MessageId    = 0x4806
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_IS_XACTION_ACTIVE_XACTION_ID_MISMATCH
Language     = English
Internal Information Only.
Is Transaction Active processing transaction mismatch Global %2 Message %3.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4806L == MLU_OBJMGR_WARNING_IS_XACTION_ACTIVE_XACTION_ID_MISMATCH);
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The Xaction Id in the call to MluObjMgrODBReflectTable() doesn't match the 
;//                     Global Xaction Id. 
;//
MessageId    = 0x4807
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_REFLECT_ODB_TABLE_XACTION_ID_MISMATCH
Language     = English
Internal Information Only.
Reflect ODB Table transaction mismatch Table %2 Global %3 Arg %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4807L == MLU_OBJMGR_WARNING_REFLECT_ODB_TABLE_XACTION_ID_MISMATCH);
;
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: The number of objects processed in a Remit Table Objects sequence doesn't 
;//                     the number of Objects sent in the MluObjMgrODBReflectionRemitTableObjects()
;//
MessageId    = 0x4808
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_OBJMGR_WARNING_REMIT_TABLE_OBJECTS_PROCESSED_COUNT_MISMATCH
Language     = English
Internal Information Only.
Remit Table Objects requested/processed counts mismatch Table %2 Requested %3 Processed %4.
.
;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4808L == MLU_OBJMGR_WARNING_REMIT_TABLE_OBJECTS_PROCESSED_COUNT_MISMATCH);
;
;
;//================================================
;//======= END OBJECT MANAGER WARNING MSGS ========
;//================================================
;//

;//========================================================
;//======= START I/O COORDINATOR/MAPPER WARNING MSGS ======
;//========================================================
;//
;// I/O Coordinator/Mapper "Warning" messages.
;//

;//======================================================
;//======= END I/O COORDINATOR/MAPPER WARNING MSGS ======
;//======================================================
;//

;//===============================================
;//======= START VAULT MANAGER WARNING MSGS ======
;//===============================================
;//
;// Vault Manager "Warning" messages.
;//

;//===============================================
;//======= END VAULT MANAGER WARNING MSGS ========
;//===============================================
;//

;//================================================
;//======= START BUFFER MANAGER WARNING MSGS ======
;//================================================
;//
;// Buffer Manager "Warning" messages.
;//

;//================================================
;//======= END BUFFER MANAGER WARNING MSGS ========
;//================================================
;//

;//=========================================
;//======= START C-CLAMP WARNING MSGS ======
;//=========================================
;//
;// C-Clamp "Warning" messages.
;//

;//=========================================
;//======= END C-CLAMP WARNING MSGS ========
;//=========================================
;//

;//=========================================
;//======= START MLU ADMIN WARNING MSGS ======
;//=========================================
;//
;// MLU ADMIN "Warning" messages.
;//

;//=========================================
;//======= END MLU ADMIN WARNING MSGS ========
;//=========================================
;//

;//=========================================
;//======= START VU MANAGER WARNING MSGS ===
;//=========================================
;//
;// VU Manager "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: LUN RAID protection is degraded
;//
MessageId    = 0x4D00
Severity     = Warning
Facility     = Mlu
SymbolicName = MLU_VUMGMT_WARNING_LU_RAID_PROTECTION_DEGRADED
Language     = English
LUN %2 has detected a fault but will continue to service IO. Please check for hardware
faults such as disk drive. Internal information only. LUN OID %3 Pool OID %4.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xA12D4D00L == MLU_VUMGMT_WARNING_LU_RAID_PROTECTION_DEGRADED);
;
;//=========================================
;//======= END VU MANAGER WARNING MSGS =====
;//=========================================
;//

;//=========================================
;//======= START FILE MANAGEMENT WARNING MSGS =
;//=========================================
;//
;// FILE Manager "Warning" messages.
;//

;//=========================================
;//======= END FILE MANAGEMENT WARNING MSGS ===
;//=========================================
;//

;//======================================
;//======= START FAMILY MANAGEMENT WARNING MSGS =
;//======================================

;//
;// FAMILY Management "Warning" messages.
;//


;//======================================
;//======= END FAMILY MANAGEMENT WARNING MSGS ===
;//======================================
;//


;//================================================
;//======= START COMPRESSION MANAGER WARNING MSGS =
;//================================================
;//
;// Compression Manager "Warning" messages.
;//

;//==============================================
;//======= END COMPRESSION MANAGER WARNING MSGS =
;//==============================================
;//

;//=========================
;//======= ERROR MSGS ======
;//=========================
;//
;//

;//======================================
;//======= START COMMON ERROR MSGS ======
;//======================================
;//
;// Common "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Could not allocate sufficient memory
;//
MessageId    = 0x8000
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OUT_OF_MEMORY
Language     = English
Insufficient system resources were available. Please contact your service provider.
Internal Information Only. Can't allocate buffer %2 of size %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8000L == MLU_COMMON_ERROR_OUT_OF_MEMORY);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested object cound not be found
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OBJECT_NOT_FOUND
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Unable to find Object %2 while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The revision id in the input buffer was invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_INVALID_INPUT_BUFFER_REVISION
Language     = English
A mismatch in revision numbers was detected while performing the operation. Please contact your service provider.
The System expected command revision %2 but received revision %3 while %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified LUN does not belong to the specified pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_LU_NOT_IN_POOL
Language     = English
The LUN does not reside in the specified Storage Pool. Retry the operation with the correct information.
Unable to locate LUN %2 in Storage Pool %3 while %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified process object id is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_INVALID_PROCESS_OBJECT_ID
Language     = English
The specified operation is not in progress. Verify that the operation has not already been completed.
Unable to %2 Process %3 because it could not be found
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was failed because the MLU driver was already shutdown.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_DRIVER_IS_SHUTDOWN
Language     = English
The array is in the process of being shut down. Retry the operation after the array has been rebooted.
Internal information only.  Unable to perform operation %2 because the array is in the process of shutting down.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was failed because the MLU driver was degraded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_DRIVER_IS_DEGRADED
Language     = English
The Virtual Provisioning feature is currently not able to process the command. Please contact your service provider
Internal Information only.  Unable to perform operation %2 because the feature is degraded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A invalid parameter was encountered
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_INVALID_PARAMETER
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid parameter %2 was passed to %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An error was returned by the DLS subsystem
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_DLS_OPERATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 was returned by DLS while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OPERATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Could not complete operation %2 because %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Object is impounded because of PSM failure
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OBJECT_IMPOUNDED_PSM_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Object is impounded because of CMI failure
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OBJECT_IMPOUNDED_CMI_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_NOT_A_MAPPED_LUN
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Driver init failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_DRIVER_INIT_FAILED
Language     = English
Virtual provisioning driver initialization failed.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An error was returned accessing system threads
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_SYSTHREAD_OPERATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 was returned while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An error was returned during State Change Notifications
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_SCN_OPERATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 was returned while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Could not access ObjMgr specific part of the object
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OBJECT_CORRUPTED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Unable to access common properties for object handle %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Error while initializing object.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_OBJECT_INIT_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while initializing object %3
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The process was cancelled.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_PROCESS_CANCELLED
Language     = English
The process was administratively cancelled.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Shutting down the driver took too long. 
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_DRIVER_SHUTDOWN_TIMED_OUT
Language     = English
Time out on driver shutdown.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Insufficient Pool space to Bind LU.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_INSUFFICIENT_SLICES
Language     = English
There was no available storage in the specified Storage Pool to complete the operation. Please add available storage to the Pool and retry the operation.
There was no available storage in Pool %2 which is required to %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8014L == MLU_COMMON_ERROR_INSUFFICIENT_SLICES);


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere (but only as a result of using internal tool `mlucli`)
;//
;// Severity: Error
;//
;// Symptom of Problem: An MLU object was marked as failed due to explicit request.
;//
MessageId    = 0x8015
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_SIMULATED_FAILURE
Language     = English
A virtual provisioning object was marked as failed due to explicit request.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Unknown.  (This error replaces an OS-specific error code that was
;//                     about to be returned, inappropriately, as an Admin error code.)
;//
MessageId    = 0x8016
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_GENERIC_OS_ERROR
Language     = English
An internal error occurred. Please gather SPcollects and contact your service provider.
Internal Information Only. An OS-specific error code was about to be returned as an Admin error code.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed due to a loss of contact with the other SP.
;//
MessageId    = 0x8017
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_LOST_CONTACT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Could not complete operation %2 because %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8017L == MLU_COMMON_ERROR_LOST_CONTACT);

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation context is already in use.
;//
MessageId    = 0x8018
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_CONTEXT_IN_USE
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation couldn't be started.
;//
MessageId    = 0x8019
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_FAILED_TO_PREPARE
Language     = English
Internal information only. Operation %2 couldn't be started due to timer failure 0x%3.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation failed to start.
;//
MessageId    = 0x801A
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_FAILED_TO_START
Language     = English
Internal information only. Operation %2 failed to start with status 0x%3.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation completed unsuccessfully.
;//
MessageId    = 0x801B
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_COMPLETED
Language     = English
Internal information only. Operation %2 completed with status 0x%3 in %4 ms.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation timed out (its start not yet having been reported).
;//
MessageId    = 0x801C
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_TIMED_OUT_START_NOT_YET_REPORTED
Language     = English
Internal information only. Operation %2 timed out after %3 ms (start not yet reported).
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation timed out (having not yet completed).
;//
MessageId    = 0x801D
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_TIMED_OUT_NOT_YET_COMPLETED
Language     = English
Internal information only. Operation %2 timed out after %3 ms.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Async operation timed out (having already completed).
;//
MessageId    = 0x801E
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_ERROR_ASYNC_OP_TIMED_OUT_ALREADY_COMPLETED
Language     = English
Internal information only. Operation %2 timed out after %3 ms (already completed in %4 ms).
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The function is not able to complete the operation at this time.
;//
MessageId    = 0x801F
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_DEVICE_ERROR
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified Oid is not supported.
;//
MessageId    = 0x8020
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_DEVICE_NOT_FOUND
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The specifiied Oids (Oid1 and Oid2) are not compatible and 
;//                     cannot be compared for shared/unshared chunks.
;//
MessageId    = 0x8021
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INCOMPATIBLE_COMPARISON_RESOURCES
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid timeout value was specified.
;//
MessageId    = 0x8022
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INVALID_TIMEOUT_VALUE
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid chunk size value was specified.
;//
MessageId    = 0x8023
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INVALID_CHUNK_SIZE
Language     = English
.

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid chunk option was Specified.
;//
MessageId    = 0x8024
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INVALID_GET_CHUNK_OPTION
Language     = English
.


;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid range of the resource's address space was specified.

;//
MessageId    = 0x8025
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INVALID_ADDRESS_RANGE
Language     = English
.


;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX copy session is in invalid state.

;//
MessageId    = 0x8026
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_INVALID_TDX_SESSION_STATE
Language     = English
.

;
;//======================================
;//======= END COMMON ERROR MSGS ========
;//======================================
;//

;//=========================================
;//======= START EXECUTIVE ERROR MSGS ======
;//=========================================
;//
;// Executive "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed because the driver has been degraded.
;//
MessageId    = 0x8100
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DRIVER_DEGRADED
Language     = English
The Virtual Provisioning feature is in a degraded state. Please contact your service provider.
Unable to process the %2 command because the feature is in a degraded state.
.


;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8100L == MLU_EXECUTIVE_ERROR_DRIVER_DEGRADED);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Executive init failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_INIT_FAILED
Language     = English
Internal Information Only. MLU Exec init failed with status %2 causing the feature to go into a degraded state.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Error
;//
;// Symptom of Problem: Executive detected incomplete stack operations.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_WL_OP_INCOMPLETE
Language     = English
Internal Information Only. Incomplete stack operations detected.Corrective action will be taken.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Usage
;//
;// Severity: Error
;//
;// Symptom of Problem: Executive detected incomplete stack operations.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_MISMATCH_OBJECT_COUNT
Language     = English
Internal Information Only. Incomplete stack operations detected.Corrective action will be taken.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: The driver is degraded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DRIVER_DEGRADED_LOG_CALL_HOME
Language     = English
The Virtual Provisioning feature is in a degraded state. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and internal information only
;//
;// Severity: Error
;//
;// Symptom of Problem: The driver is degraded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DRIVER_DEGRADED_LOG_INTERNAL_INFO
Language     = English
Internal information only. The driver is degraded. Reason: %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and internal information only
;//
;// Severity: Error
;//
;// Symptom of Problem: Admin operation failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_ADMIN_OPERATION_FAILED
Language     = English
Could not complete operation %2 due to %3. Internal information only - Operation Status: %4, Commit Status: %5.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and internal information only
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS returned an error.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_FROM_CBFS
Language     = English
Internal information only. CBFS Event: %2, Msg: %3.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU Engine not committed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_MLUENGINE_COMMIT_REQUIRED
Language     = English
The operation is not supported until the current software package is committed.  Commit the software and retry the operation.
Internal Information Only. An attempted operation failed because it was prohibited while running uncommitted.
.
;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Compression enabler not installed.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_COMPRESSION_NOT_ENABLED
Language     = English
Compression feature software is not enabled. Please install the Compression feature enabler.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation was blocked because it would have put the array
;// over its Safe Operating Capacity.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_OPERATION_WOULD_EXCEED_SAFE_OPERATING_CAPACITY_OBSOLETE
Language     = English
The attempted creation or expansion of a pool or pool LU was not allowed because it would have left the array operating over its Safe Operating Capacity.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Deduplication enabler not installed.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DEDUP_NOT_ENABLED
Language     = English
Deduplication feature software is not enabled. Please install the Deduplication feature enabler.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Deduplication cannot be enabled on a thick LUN.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DEDUP_ENABLED_INVALID_FOR_DLU
Language     = English
The Deduplication feature cannot be enabled on a thick LUN.
.


;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Deduplication cannot be enabled with compression also enabled.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_DEDUP_AND_COMPRESSION_NOT_ALLOWED
Language     = English
The Deduplication and Compression features cannot be enabled at the same time on a LUN.
.


;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Not compatible operation.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_RESTRICTED_OPERATION
Language     = English
Tiering policy cannot be set because deduplication is enabled for the lun. Set pool level deduplication tiering policy.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Deduplication cannot be enabled on a DLU whose slice allocation policy is on-demand.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DEDUP_ENABLED_INVALID_FOR_DLU_WITH_ON_DEMAND_SLICE_ALLOCATION
Language     = English
Deduplication could not be enabled on the LUN because it is using the on-demand allocation policy.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Cannot delete System LU.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_DESTROY_SYSTEM_VU_RESTRICTED
Language     = English
Cannot destroy LU because it is a system LU.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN creation failed because the preferred path's SP is still booting.
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_ERROR_CREATE_LUN_INVALID_PREFERRED_PATH
Language     = English
The operation could not proceed because one of the storage processors is still booting. Please wait until both storage processors are fully online and then retry the operation.
.


;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8111L == MLU_EXECUTIVE_ERROR_CREATE_LUN_INVALID_PREFERRED_PATH);

;//=========================================
;//======= END EXECUTIVE ERROR MSGS ========
;//=========================================
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request for the file ID for a TLU
;//                     was received, but the file had not
;//                     yet been created.
;//
MessageId    = 0x8200
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FILE_HAS_NOT_BEEN_CREATED_OBSOLETE
Language     = English
Internal Information Only.  The CBFS file has not yet been created for TLU %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8200L == MLU_MLUMGR_ERROR_FILE_HAS_NOT_BEEN_CREATED_OBSOLETE);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The MLU failed during creation because the
;//                     CBFS file could not be created.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_COULD_NOT_CREATE_FILE_OBSOLETE
Language     = English
An internal error occurred.  Please contact your service provider.
Internal Information Only.  Error %2 occurred while creating file %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The file allocation limit could not be
;//                     for this MLU.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_COULD_NOT_SET_ALLOCATION_LIMIT_OBSOLETE
Language     = English
An internal error occurred.  Please contact your service provider.
Internal Information Only.  Error %2 occurred while setting allocation limit %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind an MLU
;//                     with a WWN that already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_THIN_LU_WWN_EXISTS_OBSOLETE
Language     = English
Unable to create the specified Thin LUN because it already exists.  Please contact your service provider.
TLU %2 already exists with the WWN %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind an MLU
;//                     with a name that already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_NICE_NAME_EXISTS_OBSOLETE
Language     = English
Unable to create the Thin LUN because the specified name is already in use.  Please retry the operation using a unique name.
TLU %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of supported Thin
;//                     LUs has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_MAX_THIN_LU_COUNT_EXCEEDED_OBSOLETE
Language     = English
Unable to create the Thin LUN because the maximum number of TLUs already exists on the array.
Could not create TLU %2 because there are already %3 TLUs on the array.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The allocation SP parameter for a Thin Lu
;//                     is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_ALLOC_SP_OUT_OF_RANGE_OBSOLETE
Language     = English
Unable to create the Thin LUN because of an internal error.  Please contact your service provider.
Invalid Allocation SP value %s specified while creating TLU %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LU offset parameter for a Thin Lu
;//                     is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_LU_OFFSET_OUT_OF_RANGE_OBSOLETE
Language     = English
Unable to create the Thin LUN because the specified LU offset value is invalid.  Please retry the operation with a valid value.
Could not create TLU %2 because of an invalid allocation SP value of %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The size parameter for a Thin Lu
;//                     is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_THIN_LU_SIZE_OUT_OF_RANGE_OBSOLETE
Language     = English
Unable to create the Thin LUN because the specified TLU size is invalid.  Please retry the operation with a valid size.
Could not create TLU %2 because of an invalid size of %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received for an MLU
;//                     that does not exist.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_THIN_LU_DOES_NOT_EXIST_OBSOLETE
Language     = English
The specified Thin LUN does not exist.
Could not perform a %2 operation on TLU %3 because it does not exist.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: TLU is offline due to FS Failure
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_MLU_OFFLINE_DUE_TO_FS_FAILURE_OBSOLETE
Language     = English
Possible hardware problem or storage pool out of space. Resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal information only: TLU %2 is offline because FS %3 has failed.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: TLU is offline due to Pool being failed
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_MLU_OFFLINE_DUE_TO_POOL_FAILURE_OBSOLETE
Language     = English
A problem was detected while accessing the Storage Pool.  Please resolve any hardware problems. If the problem persists, please contact your service provider.
Internal information only.  TLU %2 is offline because Pool %3 is offline.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: TLU is faulted due to Pool fault
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_MLU_FAULTED_DUE_TO_POOL_FAULT_OBSOLETE
Language     = English
A problem was detected while accessing the Storage Pool.  Please resolve any hardware problems. If the problem persists, please contact your service provider.
Internal information only.  TLU %2 is faulted because Pool %3 is faulted.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Now
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FS_REQUIRES_RECOVERY_NOW_OBSOLETE
Language     = English
Internal information only.
Thin LUN %2 requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Asap
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FS_REQUIRES_RECOVERY_ASAP_OBSOLETE
Language     = English
Thin LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Later
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FS_REQUIRES_RECOVERY_LATER_OBSOLETE
Language     = English
Thin LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery has completed and must be
;//                     acknowledged by the user.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FS_REQUIRES_RECOVERY_ACK_OBSOLETE
Language     = English
Thin LUN recovery must be acknowledged before LUN can be brought
online. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU is offline
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_MLU_OFFLINE_OBSOLETE
Language     = English
Thin LUN %2 is unable to service IO due to a storage pool problem. Please resolve any hardware issues. If the problem persists, please contact your service provider.
Internal Information only: TLU OID %3 Pool OID %4.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU operation is progress
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_OPERATION_IN_PROGRESS_OBSOLETE
Language     = English
Unable to perform the specified operation because the Thin LUN %2 is being %3.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid FS Object ID
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_FS_ID_INVALID_OBSOLETE
Language     = English
Internal Information Only. The FS object ID %3 associated with the Thin LUN %2 is invalid
.



;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU destruction operation is progress
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_TLU_DESTRUCTION_IN_PROGRESS_OBSOLETE
Language     = English
Unable to perform the specified operation because the Thin LUN %2 is being destroyed.
.



;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU initialization operation is progress
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_TLU_CONSTRUCTION_IN_PROGRESS_OBSOLETE
Language     = English
Unable to perform the specified operation because the Thin LUN %2 is being initialized.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: TLU trespass is completed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_TRESPASS_TIMED_OUT_OBSOLETE
Language     = English
A trespass operation on TLU %2 has timed out. It will be completed shortly. Internal Information only: OID %3
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: TLU trespass loss request could not be processed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_TOL_MLU_ALREADY_DEACTIVATED_OBSOLETE
Language     = English
Internal Information Only: Deactivating TLU %2 during a trespass loss failed because the TLU was already deactivated. OID %3
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: IO requests on a TLU are timing out.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_MLU_NOT_DEACTIVATED_ON_TRESPASS_OBSOLETE
Language     = English
The Virtual Provisioning driver is unable to service IOs on Thin LUN %2 because an internal operation is currently in progress. Please wait and manually trespass the Thin LUN to the peer SP to resume IO service. Internal Information only: OID %3
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log and returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Unbind of a TLU failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_MLU_NOT_DEACTIVATED_ON_UNBIND_OBSOLETE
Language     = English
The Virtual Provisioning driver is unable to process an unbind operation on Thin LUN %2 because an internal operation is currently in progress. Please wait and retry the unbind. Internal Information only: OID %3
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Thin LUN consumed space persisted may be incorrect
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_MLUMGR_ERROR_TLU_SIZE_DISCREPANCY_FOUND_OBSOLETE
Language     = English
Internal Information Only: Thin LUN %2 consumed space; Internal = %3 Reported = %4
.


;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D821AL == MLU_MLUMGR_ERROR_TLU_SIZE_DISCREPANCY_FOUND_OBSOLETE);
;
;//===========================================
;//======= END MLU MANAGER ERROR MSGS ========
;//===========================================

;//==========================================
;//======= START FS MANAGER ERROR MSGS ======
;//==========================================
;//
;// File System Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred creating a filesystem
;//
MessageId    = 0x8400
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CREATE_FS_ERROR_FROM_CBFS
Language     = English
An error occurred while initializing a LUN. Please resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal Information only: CBFS error %2 while creating the FS for %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8400L == MLU_FSMGR_ERROR_CREATE_FS_ERROR_FROM_CBFS);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred setting the low space warning
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CONFIG_LOW_SPACE_WARNING_ERROR_FROM_CBFS
Language     = English
An error occurred while configuring a LUN. Please resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal Information only: CBFS error %2 while setting low space warning to %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred mounting a filesystem
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_MOUNT_FS_ERROR_FROM_CBFS
Language     = English
An error occurred while initializing a LUN. Please resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal Information only: CBFS error %2 occurred mounting FS %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred unmounting a filesystem
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_UNMOUNT_FS_ERROR_FROM_CBFS
Language     = English
An error occurred while trespassing or destroying a LUN. Please resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal Information only: CBFS error %2 occurred unmounting FS %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred destroying a filesystem
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_DELETE_FS_ERROR_FROM_CBFS
Language     = English
An error occurred while destroying a LUN. Please resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal Information only: CBFS error %2 occurred unbinding FS %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred cancelling process
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CANCEL_PROCESS_ERROR_FROM_CBFS
Language     = English
An internal error occurred. Please resolve any hardware problems and retry the operation. If the problem persists, please contact your service provider.
Internal Information Only. CBFS error %2 occurred cancelling process %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: FS meta-data is corrupt and requires recovery
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_RECOVERY_REQUIRED_ERROR_FROM_CBFS
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS reported FS as frozen so the LUN is offline
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_FS_FROZEN_BY_CBFS
Language     = English
An error occurred accessing the Storage Pool. Please resolve any hardware problems and retry the operation. If the problem persists, please contact your service provider.
Internal Information Only. CBFS froze FS %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Did not specify root/first slice.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_POS_INVALID
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The object is not disabled and thus the root/first slice cannot be updated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_OBJ_NOT_DISABLED
Language     = English
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation cannot be performed because the LU's file system has not been created yet.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_FS_NOT_CREATED
Language     = English
Cannot perform the specified operation until the LUN has been created.
Retry the operation when the creation has completed.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation failed because the file system requires recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_FS_REQUIRES_RECOVERY
Language     = English
The operation failed because the LUN requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal error
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error indicating Slice could not be added
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_SLICE_ADD_ERROR_FROM_CBFS
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal error
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error indicating Slice already in use
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_SLICE_IN_USE_ERROR_FROM_CBFS
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log only
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to remove Slice Adder from FS
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CANNOT_REMOVE_SA_FROM_FS
Language     = English
Internal Information Only. Unable to remove SA %2 from FS %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log only
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS returned an error determining a slice position
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_SELECT_SLICE_POSITION_ERROR_FROM_CBFS
Language     = English
Internal Information Only. Error %2 returned from CBFS while determining position for %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Slicer returned insufficient slices error selecting a slice
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_INSUFFICIENT_SLICES
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN is not waiting to be brought back online following a recovery operation.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_ACK_RECOVERY_UNEXPECTED
Language     = English
The LUN has already recovered.
.


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Start Relocation process Failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_START_RELOCATE_FAILED
Language     = English
.


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Relocation object is not ready.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_OBJECT_NOT_READY
Language     = English
.


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to throttle relocation failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_THROTTLE_FAILED
Language     = English
Internal Information Only. Attempt to throttle relocation failed.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if Slice relocation throttle rate requested exceeds 100%
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_THROTTLE_OUT_OF_RANGE
Language     = English
Internal Information Only. Specified throttle exceeds 100 percent. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if old slice FsId does not match with the FS Object FsId
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_SLICEINFO_MISMATCH
Language     = English
Internal Information Only. Relocate %2 - Old Slice information does not match with the FS object FsId.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if CBFS failed to relocate old slice
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_CBFS_RELOCATE_FAILED
Language     = English
Internal Information Only. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if CBFS cancel relocate request failed
;// 
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_CBFS_CANCEL_RELOCATE_FAILED
Language     = English
Internal Information Only. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal Information Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if relocation failed with unexpected state
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_UNEXPECTED_STATE
Language     = English
Internal Information Only. 
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_OPERATION_FAILED
Language     = English
Internal Information Only. Could not complete operation %2 because %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The free blocks cannot be computed file could not be deleted.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_COMPUTE_SLICES_FOR_EVAC
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while obtaining free blks; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to unmark slices failed in CBFS.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_UNMARK_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while unmarking slices; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to choose and mark slices failed in CBFS.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_CHOOSE_AND_MARK_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while marking slices for evacuation; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to release a slice from slicer failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_RELEASE_SLICE_FROM_SLICER
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while releasing slice from slicer; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to release a slice from slicer failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_REMOVE_SLICE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while removing slice from cbfs; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to evacuate slice failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_COULD_NOT_EVACUATE_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while evacuating slice; %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_FS_NOT_READY
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The relocation process is waiting for TE request
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_WAITING_FOR_TE
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed because the relocation 
;//                     process is completed either successfully or with failures
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_PROCESS_ALREADY_COMPLETED
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal 
;//
;// Severity: Error
;//
;// Symptom of Problem: The attempt to create an FS Object failed because
;//                     the previous Recovery operation requested more slices.
;// 
;//                     Another Recovery should succeed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CREATE_FS_FAILED_RECOVERY_SLICES_ALLOCATED
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed because the relocation process is still running
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_PROCESS_STILL_RUNNING
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The relocation process is aborted
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_PROCESS_ABORTED
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log 
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation failed because the relocation process is still destroying
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELOCATE_PROCESS_STILL_DESTROYING
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS reported FS as Recovery required.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_FS_ERROR_NOTIFICATION_BY_CBFS
Language     = English
An internal error occurred on a LUN. Please contact your service provider.
Internal Information Only. FS %2 encountered CBFS error %3.
.


;//
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem: Max concurrent Pre Provision Slice request reached.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_MAX_PRE_PROVISION_SLICES_INPROGRESS
Language     = English
.


;//
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encountered while replaying slices for File Systems
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_REPLAYING_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: File System Evacuation failure due to lack of free space in the file system.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_EVAC_FAILED_DUE_TO_NO_FREE_SPACE
Language     = English
Internal Information Only. Error %2 occurred while evacuating slice; %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Cancel evac failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGMT_ERROR_CANCEL_EVAC_FAILED
Language     = English
Internal information only. %2 Status %3
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D842CL == MLU_FSMGMT_ERROR_CANCEL_EVAC_FAILED);
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation was attempted that is prohibited by the 'on demand' allocation policy of the LU.
;//
MessageId    = 0x842D
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_ALLOCATION_POLICY_PROHIBITS_OPERATION
Language     = English
The operation failed because it is not allowed on LUNs which use the 'on demand' allocation policy. Change the LUN's allocation policy to 'automatic' and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified allocation policy is not allowed.
;//
MessageId    = 0x842E
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_INVALID_ALLOCATION_POLICY
Language     = English
The allocation policy is not allowed for the type of LUN specified.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempting to change the LU slice allocation policy from 'automatic' to 'on demand'.
;//
MessageId    = 0x842F
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_ALLOCATION_POLICY_ALREADY_AUTOMATIC
Language     = English
The allocation policy of the LUN is 'automatic'. It cannot be changed to 'on demand'.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred allowing slice request
;//
MessageId    = 0x8430
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_ALLOW_SLICE_REQUEST_ERROR_FROM_CBFS
Language     = English
An error occurred while allowing slice request.
Internal Information only: CBFS error %2 while allowing slice request for %3.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred setting FS props
;//
MessageId    = 0x8431
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_SET_FS_PROPS_ERROR_FROM_CBFS
Language     = English
An error occurred while setting FS props.
Internal Information only: CBFS error %2 while setting FS props for %3.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS error occurred getting FS props
;//
MessageId    = 0x8432
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_GET_FS_PROPS_ERROR_FROM_CBFS
Language     = English
An error occurred while getting FS props.
Internal Information only: CBFS error %2 while getting FS props %3.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to caller of IOCTL_MLU_START_SLICE_RELOCATION (mainly Policy Engine).
;//        Not emitted to event log.
;//        Believed not to be shown to customer.
;//
;// Severity: Error
;//
;// Symptom of Problem: The limit on number of slice relocation objects has been reached.
;//
MessageId    = 0x8433
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_SLICE_RELOCATION_OBJECT_LIMIT_REACHED
Language     = English
Unable to initiate a slice relocation because the maximum number of concurrent relocations has been reached.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: This error code is no longer in use
;//
MessageId    = 0x8434
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_UNUSED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8434L == MLU_FSMGR_UNUSED);
;

;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encountered while releasing FS token. A File Obj
;// which was waiting on token is now invalid. 
;// An error occurred while releasing the FS token. File Obj which was waiting on
;// this token is now invalid on this SP. As a result of this error, LUNs could go
;// offline and no admin operations could be performed on the LUN.
;//
MessageId    = 0x8435
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_RELEASING_FS_TOKEN
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8435L == MLU_FSMGR_ERROR_RELEASING_FS_TOKEN);
;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The FS token was acquired by another process object hence 
;// the current process object cannot acquire it immediately.
;//
MessageId    = 0x8436
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_CANNOT_ACQUIRE_FS_TOKEN
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8436L == MLU_FSMGR_ERROR_CANNOT_ACQUIRE_FS_TOKEN);
;

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to lookup the Family Object ID from the Family ID.
;//
MessageId    = 0x8437
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_FAILED_TO_LOOKUP_FAMILY_OID
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8437L == MLU_FSMGR_ERROR_FAILED_TO_LOOKUP_FAMILY_OID);
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The relocation object is unable to expire because 
;//                     complete-relocate-slice is required.
;//
MessageId    = 0x8438
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FSMGR_ERROR_COMPLETE_RELOCATE_SLICE_REQUIRED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8438L == MLU_FSMGR_ERROR_COMPLETE_RELOCATE_SLICE_REQUIRED);
;

;//==========================================
;//======= END FS MANAGER ERROR MSGS ========
;//==========================================
;//


;//============================================
;//======= START POOL MANAGER ERROR MSGS ======
;//============================================
;//
;// Pool Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of storage pools has been exceeded.
;//
MessageId    = 0x8500
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_MAX_POOL_COUNT_EXCEEDED
Language     = English
Unable to create the Storage Pool because the maximum number of Storage Pools already exists on the array.
Could not create Pool %2 because there are already %3 Pools on the array.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8500L == MLU_POOLMGR_ERROR_MAX_POOL_COUNT_EXCEEDED);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A storage pool with the given name already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_NAME_IN_USE
Language     = English
Unable to create the Storage Pool because the specified name is already in use. Please retry the operation using a unique Storage Pool name.
Pool %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The given percent full threshold is out of range.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FREESPACE_THRESHOLD_OUT_OF_RANGE
Language     = English
Unable to create the Storage Pool because the specified percent full threshold value is invalid. Please retry the operation with a valid value.
Could not create Pool %2 because of an invalid threshold value of %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The supplied object id is not a valid pool object id.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_INVALID_POOL_OBJECT_ID
Language     = English
The specified Storage Pool does not exist.
Could not perform a %2 operation on Pool OID %3 because it does not exist.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The given storage pool user id is already in use.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_USER_ID_IN_USE
Language     = English
Unable to create the Storage Pool because the specified Pool ID is already in use. Please retry the operation using a unique Storage Pool ID.
Pool %2 already exists with the ID %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The given storage pool user id is out of range.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_USER_ID_OUT_OF_RANGE
Language     = English
Unable to create the Storage Pool because the specified ID is invalid. Please retry the operation with a valid ID.
Could not create Pool %2 because of an invalid ID value of %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The desired storage pool is not found.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_NOT_FOUND
Language     = English
The specified Storage Pool does not exist.
Could not perform a %2 operation on Pool ID %3 because it does not exist.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An Flu with the supplied WWN already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FLU_EXISTS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Private FLU with WWN %2 is being added to Pool %3 but already exists in Pool %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An activate operation is attempted on a pool with no Flus to activate.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_NO_FLUS_TO_ACTIVATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. An error occurred during the activation phase while creating Pool %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An activate operation is attempted when the Flus are not ready.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FLU_NOT_READY
Language     = English
An error occurred while adding drives to the Storage Pool. Please resolve any hardware problems and retry the operation.
A hardware problem was detected with LUN %2 during the activation phase while creating Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A delete-flu-process operation is received for an Flu that is not a process.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FLU_IS_NOT_PROCESS_OBJECT
Language     = English
The specified operation is not in progress. Please contact your service provider.
Unable to %2 Process %3 because it could not be found.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An activate operation failed on one of the supplied Flus.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_ACTIVATION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. An error occurred accessing LUN %2 during the activation phase while creating Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An operation is attempted on a pool that is being destroyed (or shrunk).
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_DESTRUCTION_IN_PROGRESS
Language     = English
Unable to perform the specified operation because the Storage Pool is in the process of being destroyed.
Cannot perform operation %2 on Pool %3 because it is being destroyed.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A pool operation is attempted when the pool is being expanded or created.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_CONSTRUCTION_IN_PROGRESS
Language     = English
Unable to perform operations on Storage Pool that is in the process of being created. 
Operation %2 on Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A pool operation is attempted when another operation is in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_OPERATION_IN_PROGRESS
Language     = English
Unable to perform the specified operation because another operation is currently in progress on the Storage Pool. Please cancel the current operation or wait until it is done.
Cannot perform operation %2 on Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A pool destroy operation is being attempted while there are storage LUNs in the pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_HAS_LUS
Language     = English
Unable to destroy the Storage Pool. Remove any LUNs in the pool and verify that the pool is accessible, then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified Flu for the remove-Flu operation cannot be found.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FLU_NOT_FOUND
Language     = English
An internal error occurred while removing drives from the Storage Pool. Please contact your service provider.
Internal Information Only. Unable to remove LUN %2 from Pool %3 because it does not belong to the Pool.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The pool is neither destroying nor shrinking.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_ILLEGAL_REMOVE_OPERATION
Language     = English
An internal error occurred while removing drives from the Storage Pool. Please contact your service provider.
Internal Information Only. Unable to remove LUN %2 from Pool %3 because it is currently %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool meta-data is corrupt and requires recovery now
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_RECOVERY_REQUIRED_ERROR_FROM_SLICER
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool meta-data is corrupt and requires recovery
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_RECOVERY_RECOMMENDED_ERROR_FROM_SLICER
Language     = English
An internal error occurred. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: FLU offline event from Flare
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_FLU_OFFLINE_EVENT_FROM_FLARE
Language     = English
An internal error occurred resulting in a Pool lun going offline.
Internal information only. LUN %2 in Pool %3 was reported as offline.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: FLU faulted event from Flare due to single drive error
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_FLU_FAULTED_SINGLE_DRIVE_ERROR
Language     = English
A problem was detected while accessing the Storage Pool. Please resolve any hardware problems.
Internal information only. LUN %2 in Pool %3 was reported as faulted.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool is offline due to FLU Failure
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_POOL_OFFLINE_DUE_TO_FLU_FAILURE
Language     = English
A problem was detected while accessing the Storage Pool. Please resolve any hardware problems.
Internal information only.  FLU %2 went offline causing Pool %3 to go offline.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool is faulted due to FLU fault
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_POOL_FAULTED_DUE_TO_FLU_FAULT
Language     = English
A problem was detected while accessing the Storage Pool. Please resolve any hardware problems.
Internal information only.  An FLU went faulted causing Pool %2 to go faulted.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Storage Pool Requires Recovery Now
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_STORAGE_POOL_REQUIRES_RECOVERY_NOW
Language     = English
Storage Pool requires recovery. Please contact your service provider.
Internal information only.  Storage Pool %2 requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Storage Pool Requires Recovery Asap
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_STORAGE_POOL_REQUIRES_RECOVERY_ASAP
Language     = English
Storage Pool requires recovery. Please contact your service provider.
Internal information only.  Storage Pool %2 requires recovery.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Storage Pool Requires Recovery Later
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_STORAGE_POOL_REQUIRES_RECOVERY_LATER
Language     = English
Storage Pool requires recovery. Please contact your service provider.
Internal information only.    Storage Pool %2 requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A pool destroy operation is being attempted while there are FLUs in the pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_HAS_FLUS
Language     = English
Unable to destroy the Storage Pool because there are still FLUs in the Pool. Remove all FLUs from the Storage Pool and retry the operation.
Cannot destroy Pool %2 because there are still FLUs in it.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A LUN bind operation is being attempted while the pool object is not ready.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_NOT_READY_FOR_LU_BIND
Language     = English
Unable to create the LUN because the Storage Pool is not ready.  Wait for the Storage Pool to become ready and retry the operation.
Cannot create LUN because Storage Pool %2 is not ready. Internal information only. Pool OID %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A storage pool is being expanded while the pool object is not ready.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_NOT_READY_FOR_EXPANSION
Language     = English
Storage Pool is not ready for expansion.  Wait for the Storage Pool to go ready and retry the operation.
Storage Pool %1 not ready for expansion.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool is offline due to FLU(s) being not ready.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_OFFLINE
Language     = English
Storage Pool is unable to process any operations. Please resolve any hardware problems and retry the operation.
Internal information only. Pool %2 OID %3 was reported offline.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Error while unconsuming an FLU during Pool destry
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_UNABLE_TO_UNCONSUME_FLU
Language     = English
An internal error occurred while destroying the Storage Pool. Please contact your service provider.
Internal Information Only. Error %2 occurred while unconsuming LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A LUN unbind operation is being attempted while the pool object is offline.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_OFFLINE_FOR_LU_UNBIND
Language     = English
Unable to destroy the LUN because the Storage Pool is offline. Please resolve any hardware problems and retry the operation.
Cannot destroy LUN because Storage Pool %2 is offline. Internal Information Only. Pool OID %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Error while removing an FLU with allocated slices
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_FLU_HAS_ALLOCATED_SLICES
Language     = English
An internal error occurred while removing LUN. Please contact your service provider.
Internal Information Only. Error %2 occurred while removing LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Error while adding a FLARE LUN to a pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_FLU_NOT_QUALIFIED
Language     = English
An internal error occurred while adding a FLARE LUN to a pool. 
The FLARE LUN is too small or of different RAID type. Please contact your service provider.
Internal Information Only. Error: %2,  status:%3
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An operation is attempted on a pool that is expunging.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_EXPUNGE_IN_PROGRESS
Language     = English
Unable to perform the specified operation because the Storage Pool is in the expunge progress.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The storage pool is not Deduplication Enabled.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_NOT_DEDUP_ENABLED
Language     = English
Non-Deduplication enabled storage pools can't contain a Deduplication enabled lun.
Internal Information Only. Deduplication is not enabled on the Storage Pool %d.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: In a dedup enabled storage pool, an FSL is the only non-dedup enabled lun allowed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOL_INVALID_DEDUP_LUN
Language     = English
An FSL is the only non-deduplication enabled lun allowed in a deduplication enabled pool.
An FSL is the only non-deduplication enabled lun allowed in a deduplication enabled pool %2.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of FSLs per pool has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_MAX_FSL_COUNT_PER_POOL_EXCEEDED
Language     = English
Internal Information Only. Unable to create the FSL because the maximum number of Feature Storage LUNs already exists in the pool.
Could not create FSL in Pool %2 because there are already %3 Feature Storage LUNs in the pool.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Create LU operation was attempted that would have caused
;//                     the Pool's Percent Full Threshold to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Create command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_CREATION_EXCEEDS_LOW_SPACE
Language     = English
Creation attempt failed because it would have caused the pool to exceed its percent full threshold.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An Expand LU operation was attempted that would have caused
;//                     the Pool's Percent Full Threshold to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Expand command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_EXPANSION_EXCEEDS_LOW_SPACE
Language     = English
An attempt to expand a LUN failed because it would have caused the pool to exceed its percent full threshold.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Create LU operation was attempted that would have caused
;//                     the auto-delete pool full high watermark to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Create command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_CREATION_EXCEEDS_HARVESTING_HWM
Language     = English
Creation attempt failed because it would have caused the pool to exceed its auto-delete pool full high watermark.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An Expand LU operation was attempted that would have caused
;//                     the auto-delete pool full high watermark to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Expand command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_EXPANSION_EXCEEDS_HARVESTING_HWM
Language     = English
An attempt to expand a LUN failed because it would have caused the pool to exceed its auto-delete pool full high watermark.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: A Create LU operation was attempted that would have caused
;//                     the Pool's Percent Full Threshold and auto-delete pool full high watermark to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Create command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_CREATION_EXCEEDS_BOTH
Language     = English
Creation attempt failed because it would have caused the pool to exceed its percent full threshold and its auto-delete pool full high watermark.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: An Expand LU operation was attempted that would have caused
;//                     the Pool's Percent Full Threshold and auto-delete pool full high watermark to be exceeded.
;//
;// Solution: Either add more space to the Pool, or specify "-ignoreThresholds" on the Expand command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_LUN_EXPANSION_EXCEEDS_BOTH
Language     = English
An attempt to expand a LUN failed because it would have caused the pool to exceed its percent full threshold and its auto-delete pool full high watermark.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The given Auto-Delete Pool Full Watermark is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_POOLSPACE_HARVEST_WATERMARK_INVALID_RANGE
Language     = English
Unable to create the pool because the specified auto-delete pool full watermark values are invalid. Please retry the operation with valid values.
Could not create pool %2 because at least one auto-delete pool full watermark was invalid (LWM was %3 and HWM was %4).
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The given Auto-Delete Snapshot Space Watermark is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_SNAPSPACE_HARVEST_WATERMARK_INVALID_RANGE
Language     = English
Unable to create the pool because the specified auto-delete snapshot space watermark values are invalid. Please retry the operation with valid values.
Could not create pool %2 because at least one auto-delete snapshot space watermark was invalid (LWM was %3 and HWM was %4).
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to modify a modulus value that had been previously set.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_TIER_MODULUS_ALREADY_SET
Language     = English
The pool's RAID modulus for the specified tier cannot be changed because it has already been set.
Internal Information Only: Failed to set RAID modulus of tier %2 of pool %3 to %4.
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS) 0xE12D852FL == MLU_POOLMGR_ERROR_TIER_MODULUS_ALREADY_SET);
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Create/Modify pool failed because auto-delete pool full high watermark is not greater than auto-delete pool full low watermark.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_AUTO_DELETE_POOL_LWM_EXCEEDS_POOL_HWM
Language     = English
Create/Modify pool failed because auto-delete pool full high watermark is not greater than auto delete pool full low watermark. Please retry the operation with auto-delete pool full high watermark > auto-delete pool full low watermark. 
Unable to create/modify pool. Pool = %2. Auto-Delete pool full high watermark = %3 is not greater than auto-delete pool low high watermark = %4. 
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS) 0xE12D8530L == MLU_POOLMGR_ERROR_AUTO_DELETE_POOL_LWM_EXCEEDS_POOL_HWM);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Create/Modify pool failed because auto-delete snapshot space high watermark is not greater than auto delete snapshot space low watermark.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_AUTO_DELETE_SNAP_LWM_EXCEEDS_SNAP_HWM
Language     = English
Create/Modify pool failed because auto-delete snapshot space high watermark is not greater than auto delete snapshot space low watermark. Please retry the operation auto-delete snapshot space high watermark > auto-delete snapshot space low watermark .
Unable to create/modify pool. Pool = %2. Auto-Delete snapshot space high watermark = %3 is not greater than auto-delete snapshot space low watermark = %4. 
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS) 0xE12D8531L == MLU_POOLMGR_ERROR_AUTO_DELETE_SNAP_LWM_EXCEEDS_SNAP_HWM);
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Create/Modify pool failed because auto-delete auto-delete pool full high watermark is not greater than auto delete snapshot space high watermark.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_AUTO_DELETE_SNAP_HWM_EXCEEDS_POOL_HWM
Language     = English
Create/Modify pool failed because auto-delete pool full high watermark is not greater than auto delete snapshot space high watermark. Please retry the operation with auto-delete pool full high watermark greater than auto-delete snapshot space high watermark. 
Unable to create/modify pool. Pool = %2. Auto-Delete snapshot space high watermark = %3 > Auto-Delete Pool Full High Watermark = %4. 
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8532L == MLU_POOLMGR_ERROR_AUTO_DELETE_SNAP_HWM_EXCEEDS_POOL_HWM);
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to destroy a system pool
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_OPERATION_NOT_ALLOWED_ON_SYSTEM_POOL
Language     = English
An attempt to destroy storage pool %2 was rejected because this pool is required by the system.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8533L == MLU_POOLMGR_ERROR_OPERATION_NOT_ALLOWED_ON_SYSTEM_POOL);

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to create a storage pool
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_CONSTRUCTING_TAU_SLICE
Language     = English
An error occurred accessing the Storage Pool. Please resolve any hardware problems and retry the operation. If the problem persists, please contact your service provider.
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8534L == MLU_POOLMGR_ERROR_CONSTRUCTING_TAU_SLICE);

;
;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Internal information only
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to obtain scratch insurance at this time, but pool has
;//                     enough scratch space to support this operation if other, currently
;//                     in-use scratch space is freed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_ERROR_INSUFFICIENT_SCRATCH_AT_CURRENT_TIME
Language     = English
Internal use only.
.
;
;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8535L == MLU_POOLMGR_ERROR_INSUFFICIENT_SCRATCH_AT_CURRENT_TIME);
;

;//============================================
;//======= END POOL MANAGER ERROR MSGS ========
;//============================================
;//

;//=============================================
;//======= START SLICE MANAGER ERROR MSGS ======
;//=============================================
;//
;// Slice Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An operation was attempted on an uninitialized collection
;//
MessageId    = 0x8700
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_UNINITIALIZED_COLLECTION
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Collection %2 has not been initialized.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8700L == MLU_SLICE_MANAGER_ERROR_UNINITIALIZED_COLLECTION);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to uninitialize a resource collection that was still being used.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_RC_STILL_IN_RM_COLLECTION_LIST
Language     = English
Internal Information Only. Attempt to uninitialize Collection %2 while it is still being used.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to destroy a non-empty collection
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_NON_EMPTY_COLLECTION
Language     = English
Internal Information Only. Attempt to destroy non-empty Collection %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Reservations could not be handled due to lack of storage
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INSUFFICIENT_STORAGE
Language     = English
Internal Information Only. Insufficient storage to suspend allocations for LUN %2 in Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request to the the Slice Manager failed because the LUN to which the command was directed had an invalid permit.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_PERMIT_OWNERSHIP
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid permit %2 specified for LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An LUN in the Pool could not be accessed. All flare errors are mapped to this one.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_CANNOT_ACCESS_LU
Language     = English
An error occurred while accessing the Storage Pool. Please resolve any hardware problems and retry the operation.
A hardware problem was detected with LUN %2 while accessing Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN provided to the Slice Manager was too small to hold a single slice plus its meta data.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_DEVICE_TOO_SMALL
Language     = English
Internal Information Only. FLU %2 cannot be added to Pool %3 because its size is only %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The Slice Manager got an error while trying to reinitialize an LUN.  The client should fix the LUN and try to add the LUN again.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_IO_ERROR_DURING_INITIALIZATION
Language     = English
An error occurred while accessing the Storage Pool. Please resolve any hardware problems and retry the operation.
A hardware problem was detected with LUN %2 while initializing Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: While examing the metadata in the LUN an inconsistency was found in the metadata.
;//                     The client should remove the LUN and provide slice descriptions for the affected LUN.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_META_DATA
Language     = English
An internal error occurred while accessing the Storage Pool. Please contact your service provider.
Internal Information Only. A unrecoverable error was detected while accessing Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: There are no more free slices on this LUN
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INSUFFICIENT_SLICES
Language     = English
There is insufficient free space in the pool to complete the operation. Please add storage to the Pool and retry.
Internal Information only: No available storage in Pool %2 which is required to %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A client tried to commit a slice that has been already been taken by another request.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_SLICE_NOT_AVAILABLE
Language     = English
Internal Information Only. An attempt was made to commit Slice %2 which has already been commited.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client tried to remove an LUN that had slices allocated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_SLICES_ALLOCATED
Language     = English
An internal error occurred while removing drives from the Storage Pool. Please contact your service provider.
Internal Information Only. An attempt was made to remove LUN %2 from Pool %3 while it was still in use.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client attempted an operation that requires that the LUN be suspeneded when it was not.
;//                     This could either be a remove LUN command or an override slice command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_ALLOCATIONS_NOT_SUSPENDED
Language     = English
An internal error occurred while accessing the Storage Pool. Please contact your service provider.
Internal Information Only. LUN %2 in Pool %3 must be suspended first.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: There were no more acceptable resources available in the Resource Manager.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_NO_MORE_ACCEPTABLE_RESOURCES
Language     = English
Internal Information Only. There were no more acceptable resources available.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to remove a pool that still contained LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_NON_EMPTY_POOL
Language     = English
An internal error occurred while destroying the Storage Pool. Please contact your service provider.
Internal Information Only. Attempted to destroy Pool %2 while it still contained LUNs.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to override slices on an LUN that was not added with the force flag.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_LU_NOT_FORCE_ADDED
Language     = English
Internal Information Only. Attempted to override a Slice for LUN %2 in Pool %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to reinit an LUN that neede recovery.
;//                     The LUN must be added with the force flag and all slices overridden.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_RECOVERY_NEEDED_ON_LU_FORCE_ADD_REQUIRED
Language     = English
Internal Information Only. Attempted to reinitialize LUN %2 in Pool %3 before running recovery.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid Slice Id was given to the Slice Manager.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_SLICE_ID
Language     = English
An internal error occurred while accessing the Storage Pool. Please contact your service provider.
Internal Information Only. Invalid Slice ID %2 was specified for the Pool.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid CBFS_INFO structure was given to the Slice Manager.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_CBFS_SLICE_INFO
Language     = English
An internal error occured while accessing the Storage Pool. Please contact your service provider.
Internal Information Only. Invalid CBFS Slice Info was specified for the Pool.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error while removing an FLU
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_UNABLE_TO_REMOVE_FLU
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Mismatch data while updating tsector
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_TSECTOR_RESERVATION_DATA
Language     = English
Internal Information Only. Mismatch data while updating tsector
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Only one tsector can be in use by a owner at a time.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_OWNER_ALREADY_USING_A_TSECTOR
Language     = English
Internal Information Only. Only one tsector can be in use by a owner at a time.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8715L == MLU_SLICE_MANAGER_ERROR_OWNER_ALREADY_USING_A_TSECTOR);

;
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU Slice Manager metadata I/O error
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_META_DATA_IO_ERROR
Language     = English
Internal Information Only. MLU Slice Manager metadata I/O error: status %2, operation %3, device id %4, offset %5.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8716L == MLU_SLICE_MANAGER_META_DATA_IO_ERROR);


;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A programming error involving grouped slice allocation was detected.
;//
MessageId    = 0x8717
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_GROUPED_SLICE_ALLOCATION_LOGIC_ERROR
Language     = English
Internal Information Only. A programming error involving grouped slice allocation was detected.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Interleaved LUN creation was attempted simultaneously with non-grouped
;//                     LUN creation.
;//
MessageId    = 0x8718
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_ACTIVE_NONINTERLEAVED_LUN_CREATION
Language     = English
An interleaved LUN couldn't be created because creation of one or more normal LUNs was ongoing. Wait until normal LUN creation is complete, then try the operation again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Contiguous LUN creation was attempted simultaneously with non-grouped
;//                     LUN creation.
;//
MessageId    = 0x8719
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_ACTIVE_NONCONTIGUOUS_LUN_CREATION
Language     = English
A contiguous LUN couldn't be created because creation of one or more normal LUNs was ongoing. Wait until normal LUN creation is complete, then try the operation again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The creation of the set of interleaved LUNs this LUN belongs to was
;//                     interrupted.
;//
MessageId    = 0x871A
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INTERRUPTED_INTERLEAVED_CREATION
Language     = English
Interleaved LUN %2 can't become ready because creation of the set it belongs to was interrupted. Destroy this and any other LUNs in the set, then try creating the set again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The provisioning for a contiguous LUN was interrupted.
;//
MessageId    = 0x871B
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INTERRUPTED_CONTIGUOUS_CREATION
Language     = English
Contiguous LUN %2 can't become ready because provisioning of its storage was interrupted. Destroy the LUN, then try creating it again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted concurrent slice allocation group creation.
;//
MessageId    = 0x871C
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_CONCURRENT_INTERLEAVED_CREATION
Language     = English
An interleaved LUN couldn't be created because creation of the previous interleaved LUN set or contiguous LUN was not completed. If the previous creation was interrupted, destroy the LUN(s) associated with that creation, then try the operation again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempted concurrent slice allocation group creation.
;//
MessageId    = 0x871D
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_CONCURRENT_CONTIGUOUS_CREATION
Language     = English
A contiguous LUN couldn't be created because creation of the previous contiguous LUN or interleaved LUN set was not completed. If the previous creation was interrupted, destroy the LUN(s) associated with that creation, then try the operation again.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A grouped LUN couldn't be created because one or more of its group
;//                     members failed to be created.
;//
MessageId    = 0x871E
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INCOMPLETE_SLICE_ALLOCATION_GROUP
Language     = English
One or more members of the set of interleaved LUNs to which this LUN belongs were not able to be created.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: There was a problem manipulating a compact slice identifier.
;//
MessageId    = 0x871F
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_INVALID_COMPACT_SLICE_ID
Language     = English
Internal Information Only. There was a problem manipulating a compact slice identifier.
.

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Trying to release a slice which offset exceeds device's length.
;//                                 The slice may have already been released before and the device
;//                                 has been shrunk.
;//
MessageId    = 0x8720
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_SLICE_MANAGER_ERROR_SLICE_OFFSET_EXCEEDS_DEVICE_LENGTH
Language     = English
Internal Information Only. The input slice's offset exceeds device's length.
.

;
;//=============================================
;//======= END SLICE MANAGER ERROR MSGS ========
;//=============================================
;//

;//==============================================
;//======= START OBJECT MANAGER ERROR MSGS ======
;//==============================================
;//
;// Object Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid transaction identifier was passed in.
;//
MessageId    = 0x8800
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_TRANSACTION_ID
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid Transaction ID %2 was specified while %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8800L == MLU_OBJMGR_ERROR_INVALID_TRANSACTION_ID);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid object identifier was passed in.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_OBJECT_ID
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid Object ID %2 was specified while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A duplicate entry was found in the object database
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_DUPLICATE_OID
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Duplicate Object ID %2 was specified while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An existence relationship cannot be established because the maximum limit for the number 
;//                     of existence requisite objects has been reached.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_REQUISITE_LIMIT_REACHED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Cannot add relationship from Object %2 to Object %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request to wait on a state that is not a target state was received.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_WAIT_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid wait state %2 specified from Object %3 to Object %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to destroy a permanent object is being rejected.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_CANNOT_DESTROY_PERMANENT_OBJECT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Cannot destroy Object %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Object manager encountered an error while loading objects from KDBM.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_UNABLE_TO_LOAD_PERSISTENT_OBJECTS
Language     = English
Internal Information Only. Unable to access objects from PSM.  The feature will be degraded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Object manager was unable to obtain vault permit.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_UNABLE_TO_OBTAIN_VAULT_PERMIT
Language     = English
Internal Information Only. Error %2 occurred while accessing the Vault permit while committing transaction %3.  Dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Client initialize returned error.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_CLIENT_INIT_FAILED
Language     = English
An internal error occurred. Please resolve any hardware problems and retry the operation.
Internal Information Only. Error %2 occurred while initializing object %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: We do not allow more that 2^31 objects of a particular type to be created. It
;//                     is pratically impossible for the user to see this error.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_MAX_OBJECT_ID_REACHED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. There are no more available Object IDs.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Unable to communicate with peer.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_TRANSPORT_FAILURE
Language     = English
Internal Information Only. Error %2 occurred while communicating with the peer while committing transaction %3.  Dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An object with the specified object id was not found.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_OBJECT_NOT_FOUND
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid object %2 was specified while %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested object type is not valid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_OBJECT_TYPE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid object type %2 was specified while creating an object.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The requested operation is not valid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_REQUEST
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Cannot perform command %2 on object %3 because %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The activate request cannot be completed because the object is not fully deactivated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_NOT_DEACTIVATED
Language     = English
An internal error occurred. Please resolve any hardware problems and retry the operation.
Internal Information Only. Unable to activate object %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The transaction is impaired due to abnormal conditions on commit such as PSM failure or degraded driver.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_IMPAIRED_TRANSACTION
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Transaction %2 could not be committed.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request to transition to an invalid state was received.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_INVALID_NEXT_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid next state %2 specified for Object %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Request to activate an object with an outstanding activation
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_OBJECT_ACTIVE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Activation Failed for Object %3. Prior Activation still pending. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Group member is in an error state or can't be accessed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_GROUPMGR_ERROR_MEMBER_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Group Object %3 failed. Member is in an error state or not accessible. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Group member being removed from group isn't in group.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_GROUPMGR_ERROR_OBJECT_NOT_IN_GROUP
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Object %3 is not a member of Group Object %4. 
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8813L == MLU_GROUPMGR_ERROR_OBJECT_NOT_IN_GROUP);

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: MluObjMgrUnloadODBClass() called with invalid class
;//
MessageId    = 0x8814
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_UNLOAD_OBD_CLASS_INVALID_CID
Language     = English
Internal use only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8814L == MLU_OBJMGR_ERROR_UNLOAD_OBD_CLASS_INVALID_CID);

;// --------------------------------------------------
;// Introduced In: R34                                      
;//
;// Usage: Return to Unisphere and Logged in Event log
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to exceed maximum number of specific objects supported by the vault
;//
;// Generated value should be: 0xE12D8815L
;//
MessageId    = 0x8815
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_NO_FREE_SPACE_IN_VAULT_TABLE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Unable to create an additional object of class %2 because all %3 slots in its vault table are in use.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8815L == MLU_OBJMGR_ERROR_NO_FREE_SPACE_IN_VAULT_TABLE);

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: OBD Reflection Revive failed.  See MLU_REFLECT_OBJ_OP_F in 
;//                     MluObjMgrProcessUpdateObjectFromPeer().
;//                     
;//
MessageId    = 0x8816
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_OBJMGR_ERROR_REFLECT_REVIVE_FAILED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8816L == MLU_OBJMGR_ERROR_REFLECT_REVIVE_FAILED);

;//==============================================
;//======= END OBJECT MANAGER ERROR MSGS ========
;//==============================================
;//

;//======================================================
;//======= START I/O COORDINATOR/MAPPER ERROR MSGS ======
;//======================================================
;//
;// I/O Coordinator/Mapper "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = 0x8900
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_WRITE_MAP_OUT_OF_SPACE
Language     = English
Internal Information Only.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8900L == MLU_IO_MAPPER_ERROR_WRITE_MAP_OUT_OF_SPACE);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_INITIATE_MAPPING_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_MAPPING_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_OPERATION_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_START_XCHANGE_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_ABORT_XCHANGE_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_COMMIT_XCHANGE_FAILED
Language     = English
Internal Information Only.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_FAIL
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_ABORT_XCHANGE_AND_FAIL
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal Only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_MAPPER_ERROR_INSUFFICIENT_SLICES
Language     = English
.


;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8909L == MLU_IO_MAPPER_ERROR_INSUFFICIENT_SLICES);
;

;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only.
;//
;// Severity: Error
;//
;// Symptom of Problem: Lost extents found on CBFS file system for MLU
;//
;// Description:
;//     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_IO_COORD_LOST_EXTENT_MARK_BLOCK_BAD
Language     = English
Internal Information Only. Invalidating %2 sector(s) on FLU %3 at offset %4 due to lost extents found on CBFS FS for LUN %5 at LBA %6, as a result of LUN recovery run in the past.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D890AL == MLU_IO_COORD_LOST_EXTENT_MARK_BLOCK_BAD);
;

;//======================================================
;//======= END I/O COORDINATOR/MAPPER ERROR MSGS ========
;//======================================================
;//

;//=============================================
;//======= START VAULT MANAGER ERROR MSGS ======
;//=============================================
;//
;// Vault Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Layers below Vault Manager returned an error.
;//
MessageId    = 0x8980
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_BACKING_STORE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while accessing the Vault.  The driver will be degraded and dirty objects will be impounded.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8980L == STATUS_MLU_VAULTMGR_ERROR_BACKING_STORE);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client requested a Vault operation with an invalid permit.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_PERMIT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid permit %2 was used while accessing the Vault.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Table's record size and the client's persistent data size are not equal
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_DATA_SIZE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Unable to find the OID in buffer %2 and table %3.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client specified a table index that is out of range.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_TABLE_INDEX
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid table index %2 was used while accessing the Vault.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Table is full. No more persistent data can be stored in that table.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_NO_SPACE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Table %2 is full.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client requested for a data that is not is not present in the vault.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_DATA_NOT_FOUND
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Unable to locate record %2 in table %3.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client is trying to read data beyond the table size.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_NO_MORE_DATA
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Attempted to read record index %2 from table %3 of size %4.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client is trying to read records sequentially with an invalid iterator.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_ITERATOR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Permit %2 does not match the iterator permit %3.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client is trying to read relationship records with an invalid iterator.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_RELATION_ITERATOR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid permit %2.  The driver will be degraded and dirty objects will be impounded.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A client specified an intent other than abort or commit to the vault changes.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = STATUS_MLU_VAULTMGR_ERROR_INVALID_COMMIT_INTENT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid commit intent %2 using prmit %3.  The driver will be degraded and dirty objects will be impounded.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8989L == STATUS_MLU_VAULTMGR_ERROR_INVALID_COMMIT_INTENT);
;
;//=============================================
;//======= END VAULT MANAGER ERROR MSGS ========
;//=============================================
;//

;//==============================================
;//======= START BUFFER MANAGER ERROR MSGS ======
;//==============================================
;//
;// Buffer Manager "Error" messages.
;//

;//==============================================
;//======= END BUFFER MANAGER ERROR MSGS ========
;//==============================================
;//

;//=======================================
;//======= START C-CLAMP ERROR MSGS ======
;//=======================================
;//
;// C-Clamp "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = 0x8a80
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_CCLAMP_INVALID_VOLUME_ID
Language     = English
Invalid storage volume ID.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8A80L == MLU_CCLAMP_INVALID_VOLUME_ID);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_CCLAMP_UNIT_IS_NOT_QUIESCED
Language     = English
Invalid storage volume ID.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_CCLAMP_UNIT_IS_NOT_QUIESCED_FOR_THIS_REASON_OBSOLETE
Language     = English
Invalid storage volume ID.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8A82L == MLU_CCLAMP_UNIT_IS_NOT_QUIESCED_FOR_THIS_REASON_OBSOLETE);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU received I/O error for CBFS metadata I/O
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_CCLAMP_META_DATA_IO_ERROR
Language     = English
Internal Information Only. MLU received CBFS metadata I/O error: status %2, operation %3, FLU %4, Offset %5.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8A83L == MLU_CCLAMP_META_DATA_IO_ERROR);
;
;//=======================================
;//======= END C-CLAMP ERROR MSGS ========
;//=======================================
;//

;//=======================================
;//======= START MLU ADMIN ERROR MSGS ======
;//=======================================
;//
;// MLU ADMIN "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = 0x8b00
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_REQUIRED_PARAMETER_MISSING
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Expected parameter %1 not specified for the %2 command.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8B00L == MLU_ADMIN_REQUIRED_PARAMETER_MISSING);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_DATA_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid value specified for %1 during %2 operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_WORKLIST_OP_TYPE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid worklist type %1 specified.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_LU_OBJ_FOR_OPERATIONS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid object type %1 specified for %2 operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_TLD_OBJECT_ID
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid TLD object type %1 specified for %2 operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_TLD_OPERATION_TYPE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid TLD operation type %1 specified.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_POOL_SET_OP_TYPE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid Pool operation type %1 specified.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_OBJ_OP_TYPE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid object operation type %1 specified.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_INVALID_POLL_RESPONSE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Invalid poll response %1 specified.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_NDU_IN_PROGRESS
Language     = English
Operation cannot be completed, NDU in progress.
Operation %1 cannot be completed, NDU in progress.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_BASE_PACKAGE_NOT_COMMITTED
Language     = English
Operation cannot be completed, Base package not committed. Commit the Base package and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_BASE_ENTRY_NOT_FOUND_IN_TOC
Language     = English
An internal error occurred. Please contact your service provider.
Internal information only. Can not get Base entry from ToC file.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     Too many or too few members were specified in the command.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_INVALID_NUMBER_OF_CG_MEMBERS
Language     = English
Either no Consistency Group members or too many Consistency Group members were specified in the command.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     The user supplied text string is too long.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_TEXT_STRING_TOO_LONG
Language     = English
The user supplied text string is too long.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//     The command specified more than one object. Retry the operation with one object at a time.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_ADMIN_ERROR_ONLY_ONE_ALLOWED
Language     = English
The command specified more than one object. Retry the operation with one object at a time.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8B0EL == MLU_ADMIN_ERROR_ONLY_ONE_ALLOWED);
;
;//=======================================
;//======= END MLU ADMIN ERROR MSGS ========
;//=======================================
;//

;//=======================================
;//== START RECOVERY MANAGER ERROR MSGS ==
;//=======================================
;//
;// RECOVERY MANAGER "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if an operation is
;// requested on a pool undergoing recovery. The operation cannot
;// be allowed until pool recovery completes.
;//
MessageId    = 0x8c00
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_RECOVERY_EXISTS
Language     = English
Storage Pool recovery is in progress. This operation is not allowed until recovery completes.
Recovery in progress on Storage Pool %3.
Internal information only. Failed operation %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C00L == MLU_RM_ERROR_POOL_RECOVERY_EXISTS);
;
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_EXCEEDED_MAX_RECOVERY_SESSIONS
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FAILED_TO_CREATE_SESSIONS
Language     = English
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery process ID specified is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PROCESS_OBJECT_ID_INVALID
Language     = English
The specified recovery process ID is invalid. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery process object not found for specified Pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_RECOVERY_NOT_FOUND
Language     = English
The specified Storage Pool recovery process does not exist.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Cancel recovery failed because the recovery is either completed, failed or was cancelled earlier.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVERY_ABORT_FAILED
Language     = English
Recovery cannot be cancelled because it is either completed, failed or was cancelled earlier.
Recovery %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: FS recovery process object not found for the specified FS Object.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_RECOVERY_NOT_FOUND
Language     = English
The specified file system recovery process does not exist.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if an operation is
;// requested on a LUN undergoing recovery. The operation cannot
;// be allowed until LUN recovery completes.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_RECOVERY_EXISTS
Language     = English
LUN recovery is in progress. This operation is not allowed until recovery completes.
Internal information only. Failed operation %2 recovery in progress on LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if FS recovery throttle rate requested exceeds 100%
;//
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_THROTTLE_OUT_OF_RANGE
Language     = English
Specified throttle exceeds 100 percent. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified recovery process is still running. The requested operation is prohibited until it has run to completion.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVERY_STILL_RUNNING
Language     = English
Recovery in progress. Retry operation once recovery has finished.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while disabling file system.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery was unable to disable LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_UNABLE_TO_DISABLE_LUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery was unable to disable LUNs (%3).
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error waiting for a LUN to disable.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_WAIT_FOR_DISABLE_LU
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 waiting on LUN %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error iterating LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_ITERATE_LUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 iterating on LUNs.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error accessing FS object.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_ACCESS_OBJECT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 accessing file system object.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error waiting for a file system to disable.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_FS_WAIT_FOR_DISABLE_FS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 waiting on file system.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error calculating scratch space required.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_GET_SCRATCH_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 calculating scratch space required.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while calculating scratch space required.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_GET_SCRATCH_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because scratch space required exceeds max allowed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_GET_SCRATCH_EXCEEDED_MAX
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
scratch space required (%3 slices) exceeds max allowed (%4 slices).
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error acquiring scratch slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_ACQUIRE_SLICES_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 acquiring scratch slices.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while acquiring scratch slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_ACQUIRE_SLICES_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because slice data could not be acquired.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INVALID_FLUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 because
slice data could not be acquired.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because recovery could not reinitialize slice iterator.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INVALID_ITERATOR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 because
recovery encountered error %3 reinitializing slice iterator.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while setting slice info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_SET_SLICE_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because recovery encountered an error listing slice info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_SLICE_MGR_LIST_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 listing slice data.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed due to error setting slice info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_SET_SLICE_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 because
of error %3 while setting slice info.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because pool requires recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVER_FS_POOL_RECOVERY_REQUIRED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
pool requires recovery (%3).
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVER_FS_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 with error %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while recovering file system.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVER_FS_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while clearing slice info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_CLEAR_SLICE_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while releasing scratch slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RELEASE_SLICES_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error releasing scratch slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RELEASE_SLICES_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 releasing scratch slices.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state while disabling pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to error accessing pool object.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_ACCESS_OBJECT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of error %3 accessing pool object.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery encountered an error waiting for pool to disable.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_WAIT_FOR_DISABLE_POOL
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery encountered error %3 waiting on pool.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery was unable to disable LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_UNABLE_TO_DISABLE_LUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery was unable to disable LUNs (%3).
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery encountered an error waiting for a LUN to disable.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_WAIT_FOR_DISABLE_LU
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery encountered error %3 waiting on LUN %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery encountered an error iterating LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_ITERATE_LUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery encountered error %3 iterating on LUNs.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery encountered an error waiting for a file system to disable.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_WAIT_FOR_DISABLE_FS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery encountered error %3 waiting on file system %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state while setting slice info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_SET_SLICE_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVER_POOL_ERROR
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2
with error %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RECOVER_POOL_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state while clearing slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_CLEAR_SLICES_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed because recovery was unable to enable LUNs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_PREPARE_POOL_UNABLE_TO_ENABLE_LUS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
recovery was unable to enable LUNs (%3).
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery session failed to start.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_BEGIN_SESSION_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 with status %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to acquire scratch slices for recovery session.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_SESSION_ACQUIRE_SLICES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed to acquire scratch slices
for %2 with status %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery process was cancelled by user.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_ABORT_RECOVERY
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery of %2 cancelled by user.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Check for Recovery object on an invalid Object ID.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INVALID_OID
Language     = English
The specified recovery process ID is invalid. Please contact your service provider.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Delete Pool Recovery Object failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_DELETE_RECOVER_POOL_FAILED
Language     = English
Internal information only. Delete Storage Pool %2 Recovery Object %3 failed
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Delete lu Recovery Object failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_DELETE_RECOVER_LU_FAILED
Language     = English
Internal information only. Delete LUN %2 Recovery Object %3 failed
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Two or more file systems are claiming the same slice. Each file system needs a copy of the slice.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_DUPLICATE_SLICE_REQUIRED
Language     = English
Two or more file systems are claiming the same slice. Each file system needs a copy of the slice. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A file system is missing a slice. A new slice must be allocated to the file system to fill in
;// the hole.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_REPLACEMENT_SLICE_REQUIRED
Language     = English
A file system is missing a slice. A new slice must be allocated to the file system to fill in the hole. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Received an invalid request during recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_UPDATE_SLICE_INVALID
Language     = English
Received an invalid request during recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Received a request to update slice info for an invalid pool.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_UPDATE_SLICE_FLU_POOL_MISMATCH
Language     = English
Received a request to update a slice from another pool. Please contact your service provider.
Internal information only. Update slice FLU pool OID %2 does not match
pool OID being recovered %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Received an invalid request during recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_EXPAND_SCOPE_INVALID
Language     = English
Received an invalid request during recovery. Please contact your service provider.
Internal information only. Expand scope type %d is invalid.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice update specifies a file system that belongs to a different pool. Cross pool contamination has occurred.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_UPDATE_SLICE_FS_POOL_MISMATCH
Language     = English
Received a request to assign a slice from this pool to a file system in another pool. Please contact your service provider.
Internal information only. Update slice FS pool OID %2 does not match
pool OID being recovered %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed due to unexpected object state while waiting for a session.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_WAIT_FOR_SESSION_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state while waiting for a session.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_WAIT_FOR_SESSION_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Pool recovery failed due to unexpected object state while synching metadata.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_SYNC_METADATA_UNEXPECTED_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for pool %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because file system metadata could not be updated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_SLICE_UPDATE_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 because
recovery encountered error %3 updating file system slice info.
.


;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: This error is returned if an operation is
;// requested on a pool undergoing verification. The operation cannot
;// be allowed until pool verify completes.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_POOL_VERIFY_EXISTS
Language     = English
Storage Pool verify is in progress. This operation is not allowed until verify completes.
Verify in progress on Storage Pool %3.
Internal information only. Failed operation %2.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because the file system could not be found.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_NOT_FOUND
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for %2 because
file system %3 could not be found %4.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Not enough free space available in pool to start recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INSUFFICIENT_POOL_SPACE
Language     = English
Not enough free space available in pool to start recovery.
Expand the Storage Pool and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because there were not enough resources available.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INSUFFICIENT_RESOURCES
Language     = English
Recovery failed due to insufficient resources. Wait for other recoveries to
complete and try again. If the problem persists, please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Recoveries stuck at 0% progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_SESSION_INCONSISTENT_STATE
Language     = English
Recovery encountered an internal error and cannot proceed.
In order to resume recovery, reboot the SP.
.

;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to throttle recovery failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_THROTTLE_RECOVERY_FAILED
Language     = English
Attempt to throttle recovery failed.
.

;// --------------------------------------------------
;// Introduced In: R29
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because recovery encountered an error adding slice to file system.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_INCOMPLETE_FS_RECOVERY_ADD_SLICE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Add Slice failed for file system %2 because
of unexpected state %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File system recovery failed because CBFS File System Recovery requested a recovery Slice, and
;//                     the installed version of MLU does not support that request, or is not committed. 
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FS_REQUEST_RECOVERY_SLICE_NOT_SUPPORTED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery Slice Requested for file system %2 
when installed version of MLU does not support Recovery Slice Requests,
or is not committed. Please revert or commit the Bundle and recover the
FS again.
.

;//
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Marking File system/Pool for Recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_MARKING_FOR_RECOVERY
Language     = English
Internal Information Only. Marking %4 for recovery. (%2, %3, %5).
.

;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery session object should not begin any recovery before FLUs are added to Slice Manager. 
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RETRY_RECOVERY_LATER
Language     = English
The recovery operation could not proceed because resources it needs have not yet become available. Please retry the operation.
Internal Information Only. Recovery object %2 failed with status %3.
.

;//
;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Unisphere
;//
;// Severity: Info
;//
;// Symptom of Problem: Root Slice was released during Pool recovery.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_INFO_ROOT_SLICE_RELEASED
Language     = English
Internal Information Only. FS (%2) root slice (%3, %4, %5) released during pool recovery.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because recovery encountered an error shifting all FS family reservations to the recovery family.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_RESERVATIONS_NOT_COLLECTED
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery encountered error %3 accounting for pool reservations in the
file system.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because recovery encountered an error trying to clean the dedicated pool scratch slices.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FAILED_TO_CLEAN_POOL_SCRATCH_SPACE
Language     = English
An internal error occurred. Please check for hardware problems affecting
pool %2 and contact your service provider. Internal Information Only.
Recovery failed because the pool recovery scratch slices for pool %3 could
not be cleaned prior to recovery.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C48L == MLU_RM_ERROR_FAILED_TO_CLEAN_POOL_SCRATCH_SPACE);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because recovery encountered an error trying to get StorageVolumeHandle from UFS64
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FAILED_TO_GET_STORAGE_VOLUME_HANDLE
Language     = English
The recovery operation could not proceed because it failed to get critical resources. 
Internal Information Only. Recovery object %2 failed with status %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C49L == MLU_RM_ERROR_FAILED_TO_GET_STORAGE_VOLUME_HANDLE);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because VDM Object is being deactivated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_VDM_DEACTIVATING_RETRY_RECOVERY_LATER
Language     = English
The recovery operation could not proceed because it failed to get critical resources. 
Internal Information Only. Recovery object %2 failed with status %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C4AL == MLU_RM_ERROR_VDM_DEACTIVATING_RETRY_RECOVERY_LATER);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because unable to disable UFS64
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_UNABLE_TO_DISABLE_UFS64
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Recovery failed for file system %2 because
recovery was unable to disable UFS64 (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C4BL == MLU_RM_ERROR_UNABLE_TO_DISABLE_UFS64);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because it encountered an error when trying to release Volume Handle for UFS64
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FAILED_TO_RELEASE_STORAGE_VOLUME_HANDLE
Language     = English
The recovery operation could not proceed because it failed to release critical resources.
Internal Information Only. Recovery object %2 failed with status %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C4CL == MLU_RM_ERROR_FAILED_TO_RELEASE_STORAGE_VOLUME_HANDLE);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_FAILED_TO_GET_SCRATCH_SLICES
Language     = English
The recovery operation could not proceed because it failed to get scratch slices.
Internal Information Only. Recovery object %2 for object %3(Size: %4) failed to get %5 scratch slices. 
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C4DL == MLU_RM_ERROR_FAILED_TO_GET_SCRATCH_SLICES);
;
;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery failed because it encountered a VDM error when trying to recover the file system
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_RM_ERROR_VDM_ERROR_RETRY_RECOVERY_LATER
Language     = English
The recovery operation could not proceed because it failed to get critical resources.
Internal Information Only. Recovery object %2 failed with status %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8C4EL == MLU_RM_ERROR_VDM_ERROR_RETRY_RECOVERY_LATER);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Info
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Informational
Facility     = Mlu
SymbolicName = MLU_RM_GET_CURRENT_SCRATCH_SLICES_STATUS
Language     = English
Internal Information Only. Crrently the two largest FSs are: FS %2, size %3, reserved %4 scratch slices; FS %5, size %6, reserved %7 scratch slices. Pre-reserved slices: Pool %8, Pool priv family %9.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0x612D8C4FL == MLU_RM_GET_CURRENT_SCRATCH_SLICES_STATUS);


;
;//=======================================
;//==  END RECOVERY MANAGER ERROR MSGS  ==
;//=======================================
;//

;//=========================================
;//======= START VU MANAGEMENT ERROR MSGS ===
;//=========================================
;//
;// VU Management "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: A request for the file ID for a LUN was received, but the file had not yet been created.
;//
MessageId    = 0x8D00
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FILE_HAS_NOT_BEEN_CREATED
Language     = English
Internal Information Only. The CBFS file has not yet been created for LUN %2.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8D00L == MLU_VUMGMT_ERROR_FILE_HAS_NOT_BEEN_CREATED);
;
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The MLU failed during creation because the CBFS file could not be created.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_COULD_NOT_CREATE_FILE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while creating file %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The file allocation limit could not be for this MLU.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_COULD_NOT_SET_ALLOCATION_LIMIT
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while setting allocation limit %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind an LUN with a WWN that already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_WWN_EXISTS
Language     = English
Unable to create the specified LUN because it already exists. Please contact your service provider.
LU %2 already exists with the WWN %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind an LUN with a name that already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_NICE_NAME_EXISTS
Language     = English
Unable to create the LUN because the specified name is already in use. Please retry the operation using a unique name.
LU %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of supported LUNs has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_MAX_LU_COUNT_EXCEEDED
Language     = English
Unable to create the LUN because the maximum number of LUNs already exists on the array.
Could not create LUN %2 because there are already %3 LUNs on the array.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The allocation SP parameter for a Lu is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_ALLOC_SP_OUT_OF_RANGE
Language     = English
Unable to create the LUN because of an internal error. Please contact your service provider.
Invalid Allocation SP value %s specified while creating LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN offset parameter for a LUN is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_OFFSET_OUT_OF_RANGE
Language     = English
Unable to create the LUN because the specified LUN offset value is invalid. Please retry the operation with a valid value.
Could not create LUN %2 because of an invalid allocation SP value of %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The size parameter for a Lu is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_SIZE_OUT_OF_RANGE
Language     = English
Unable to create the LUN because the specified LUN size is invalid. Please retry the operation with a valid size.
Could not create LUN %2 because of an invalid size of %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received for an LUN that does not exist.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_DOES_NOT_EXIST
Language     = English
The specified LUN does not exist.
Could not perform a %2 operation on LUN %3 because it does not exist.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN is offline due to FS Failure
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_LU_OFFLINE_DUE_TO_FS_FAILURE
Language     = English
Possible hardware problem or storage pool out of space. Resolve any hardware issues and retry. If the problem persists, please contact your service provider.
Internal information only: LUN %2 is offline because FS %3 has failed.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN is offline due to Pool being failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_LU_OFFLINE_DUE_TO_POOL_FAILURE
Language     = English
A problem was detected while accessing the Storage Pool. Please resolve any hardware problems. If the problem persists, please contact your service provider.
Internal information only. LUN %2 is offline because Pool %3 is offline.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN is faulted due to Pool fault
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_LU_FAULTED_DUE_TO_POOL_FAULT
Language     = English
A problem was detected while accessing the Storage Pool. Please resolve any hardware problems. If the problem persists, please contact your service provider.
Internal information only. LUN %2 is faulted because Pool %3 is faulted.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Now
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_REQUIRES_RECOVERY_NOW
Language     = English
Primary LUN requires recovery. Please contact your service provider.
Internal information only. File system %2 requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Asap
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_REQUIRES_RECOVERY_ASAP
Language     = English
Primary LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The File System Requires Recovery Later
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_REQUIRES_RECOVERY_LATER
Language     = English
Primary LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery has completed and must be acknowledged by the user.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_REQUIRES_RECOVERY_ACK
Language     = English
LUN recovery must be acknowledged before LUN can be brought online. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN is offline
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_OFFLINE
Language     = English
LUN %2 is unable to service IO due to a storage pool problem. Please resolve any hardware issues. If the problem persists, please contact your service provider.
Internal Information only: LUN OID %3.
.


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN operation is progress
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_OPERATION_IN_PROGRESS
Language     = English
Unable to perform the specified operation because there is an operation in progress.
LUN %2 is being %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid FS Object ID
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_ID_INVALID
Language     = English
Internal Information Only. The FS object ID %3 associated with the LUN %2 is invalid
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: MLU destruction operation is progress
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_DESTRUCTION_IN_PROGRESS
Language     = English
Unable to perform the specified operation because the LUN is being destroyed.
LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN initialization operation is progress
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_CONSTRUCTION_IN_PROGRESS
Language     = English
Unable to perform the specified operation because the LUN is being initialized. Retry the operation after LUN initialization completes. 
LUN %2.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN trespass is completed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_TRESPASS_TIMED_OUT
Language     = English
A trespass operation on LUN %2 has timed out. It will be completed shortly. Internal Information only: OID %3
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN trespass loss request could not be processed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_TOL_LU_ALREADY_DEACTIVATED
Language     = English
Internal Information Only: Deactivating LUN %2 during a trespass loss failed because the LUN was already deactivated. OID %3
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: IO requests on a LUN are timing out.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_NOT_DEACTIVATED_ON_TRESPASS
Language     = English
The Virtual Provisioning driver is unable to service IOs on LUN %2 because an internal operation is currently in progress. Internal Information only: OID %3
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Unbind of a LUN failed
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_NOT_DEACTIVATED_ON_UNBIND
Language     = English
The Virtual Provisioning driver is unable to process an unbind operation because an internal operation is currently in progress. Please wait and retry the unbind.
LUN %2 OID %3
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: LUN consumed space persisted may be incorrect
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_LU_SIZE_DISCREPANCY_FOUND
Language     = English
Internal Information Only: LUN %2 consumed space; Internal = %3 Reported = %4
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation cannot proceed due to VU Object in an error state.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_VU_OBJECT_IN_ERROR_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. VU Object %2 is in an error state.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The operation attempted by user cannot proceed as the VU is not attached.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_VU_NOT_ATTACHED
Language     = English
Operation cannot proceed as the LUN is not attached.
Error VU %2 is not attached.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: VU expansion failed due to an error while extending the file. Refer to the event log for detailed error info.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_UNABLE_TO_EXTEND_FILE
Language     = English
An internal error caused LUN expansion operation to fail. Please contact your service provider.
Internal Information Only. VU expansion failed due to error %2
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: VU object failed due to an an error while notifying compression manager.
;//                     VU Object will go to the error state because of the failure. Please contact your service provider.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FAILED_TO_NOTIFY_COMPRESSION_MGR
Language     = English
Internal Information Only. Failed to notify compression manager.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform a shrink operation when the LUN is currently expanding. Wait
;//                     Until the expansion is completed before attempting to shrink again.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_NOT_ALLOWED_VU_CURRENTLY_EXPANDING
Language     = English
Shrink operation is not allowed on the LUN as the LUN is currently expanding.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform a shrink operation when the LUN is currently shrinking. Wait
;//                     Until the previous shrink operation is completed before attempting to shrink again.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_NOT_ALLOWED_VU_SHRINK_IN_PROGRESS
Language     = English
LUN is not allowed to shrink due to a shrink operation already in progress.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform a shrink operation when the LUN is currently expanding. Wait
;//                     Until the expansion is completed before attempting to shrink again.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPANSION_NOT_ALLOWED_VU_CURRENTLY_EXPANDING
Language     = English
Expansion operation is not allowed on the LUN as the LUN is currently expanding.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform a shrink operation when the LUN is currently shrinking. Wait
;//                     Until the previous shrink operation is completed before attempting to shrink again.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPANSION_NOT_ALLOWED_VU_SHRINK_IN_PROGRESS
Language     = English
LUN expansion not allowed due to a shrink operation in progress.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to destroy a LUN when the LUN expansion is 
;//                     in progress. Wait until the expansion is completed before
;//                     attempting to destroy the LUN.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DESTROY_NOT_ALLOWED_EXPANSION_IN_PROGRESS
Language     = English
Destroy operation not allowed as LUN expansion is in progress
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to destroy a LUN when the LUN shrink is 
;//                     in progress. Wait until the shrink is completed before
;//                     attempting to destroy the LUN.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DESTROY_NOT_ALLOWED_SHRINK_IN_PROGRESS
Language     = English
Destroy operation not allowed as LUN shrink is in progress
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Unused
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to expand a LUN while an add or remove compression operation was in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPAND_NOT_ALLOWED_COMPRESSION_IN_PROGRESS
Language     = English
Cannot expand a LUN that has an add or remove compression operation in progress.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Unused
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to shrink a LUN while an add or remove compression operation was in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_NOT_ALLOWED_COMPRESSION_IN_PROGRESS
Language     = English
Cannot shrink a LUN that has an add or remove compression operation in progress.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to locate the specified LUN.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_VU_NOT_FOUND
Language     = English
The specified LUN does not exist.
.

;// --------------------------------------------------
;// Introduced In: R31
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind an LUN with a name that already exists.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SETNAME_NICE_NAME_EXISTS
Language     = English
Unable to set Nice Name.
LUN %2 already exists with the name %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of supported LUNs has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_MAX_FSL_COUNT_EXCEEDED
Language     = English
Internal Information Only. Unable to create the FSL LUN because the maximum number of Feature Storage LUNs already exists on the array.
Could not create FSL LUN %2 because there are already %3 FSL LUNs on the array.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform an expand operation on a Feature Storage LUN.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPANSION_NOT_ALLOWED_ON_FSL
Language     = English
LUN expansion not allowed on a Feature Storage LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to perform a shrink operation on a Feature Storage LUN.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_NOT_ALLOWED_ON_FSL
Language     = English
LUN shrink not allowed on a Feature Storage LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A request was received to bind a FSL LUN with no speicified name.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_NICE_NAME_NOT_SPECIFIED
Language     = English
Unable to create FSL LUN because the LUN name is not specified.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Fixture notification function returned error.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_VU_FIXTURE_NOTIFICATON
Language     = English
Fixture notification function failed.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specifed alias LUN was not found.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_ALIAS_NOT_FOUND
Language     = English
Internal Information Only. Alias LUN %2 not found.
.


;// --------------------------------------------------
;// Introduced In: R31
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN specified for recovery is not the primary LUN.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_INCORRECT_LUN_TYPE_FOR_RECOVERY
Language     = English
Recovery could not be performed on this LUN because it is not a primary LUN. Find the affiliated primary LUN and perform recovery on it.
Internal information only. Recovery could not be performed on LUN %2 (OID %3) because it is not a primary LUN.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The specifed replica file cannot be promoted.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_UNABLE_TO_PROMOTE_FILE
Language     = English
Internal use only. An error was encountered promoting a replica file.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The specifed replica file cannot be demoted.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_UNABLE_TO_DEMOTE_FILE
Language     = English
Internal use only. An error was encountered demoting a replica file.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi.
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of supported SnapLUs has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_MAX_SNAPLU_COUNT_EXCEEDED
Language     = English
Unable to create the snapshot mount point because the maximum number of snapshot mount points already exists on the array.
Internal Information Only. Could not create snapshot mount point %2 because there are already %3 snapshot mount points on the array.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LU expansion cannot be started due to the LU not being in a ready state.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPAND_LU_NOT_READY
Language     = English
The LUN could not be expanded because it is not in a ready state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LU shrink cannot be started due to the LU not being in a ready state.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_LU_NOT_READY
Language     = English
The LUN could not be shrunk because it is not in a ready state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LU shrink cannot be started because the creation of a snapshot to the LU in in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_SHRINK_LU_SNAP_IN_PROGRESS
Language     = English
The LUN could not be shrunk because a snapshot of it is being created. Wait for the snapshot creation to complete and then retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: LU expansion cannot be started because the creation of a snapshot to the LU in in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_EXPAND_LU_SNAP_IN_PROGRESS
Language     = English
The LUN could not be expanded because a snapshot of it is being created. Wait for the snapshot creation to complete and then retry the operation.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to UNISPHERE
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to change LUN size failed because the system is already 
;//                     processing the maximum allowed number of such operations.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_NO_MORE_LUN_RESIZE_OPERATIONS_ALLOWED
Language     = English
The LUN size could not be changed because the system is already processing the maximum allowed number of such operations. Wait for one of the outstanding size changes to complete and then retry the operation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8D37L == MLU_VUMGMT_ERROR_NO_MORE_LUN_RESIZE_OPERATIONS_ALLOWED);


;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specifed alias source LUN is a Snap LU.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_ALIAS_SOURCE_IS_SNAPLU
Language     = English
Internal Information Only. Failed creating Alias destination lun. Alias Source LUN %2 is a Snap LUN.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Alias destination lun can not be created because the maximum Alias Lun limit is reached.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_ALIAS_LU_MAX_LIMIT_REACHED
Language     = English
Internal Information Only. Failed creating Alias destination Lun. Maximum Alias LUN limit reached.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The alias source LUN already has a destination lun.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_ALIAS_SOURCE_INVALID
Language     = English
Internal Information Only. Failed creating Alias destination LUN. Alias source LUN %2 already has a destination LUN %3.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN was in use by Replication.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_DELETE_ERROR_LUN_IN_USE_BY_REPLICATION
Language     = English
The LUN being referred is in use by Replication.  Hence, LUN delete cannot be performed.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The Deduplication File System Requires Recovery Now
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DEDUP_FS_REQUIRES_RECOVERY_NOW
Language     = English
Deduplication primary LUN requires recovery. Please contact your service provider.
Internal information only. Deduplication file system %2 requires recovery.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The Deduplication File System Requires Recovery Asap
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DEDUP_FS_REQUIRES_RECOVERY_ASAP
Language     = English
Deduplication primary LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The Deduplication File System Requires Recovery Later
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DEDUP_FS_REQUIRES_RECOVERY_LATER
Language     = English
Deduplication primary LUN requires recovery. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovery has completed and must be acknowledged by the user.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_DEDUP_FS_REQUIRES_RECOVERY_ACK
Language     = English
Acknowledge the deduplication LUN recovery to bring all deduplication LUNs in this pool online. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Delete of TLU recovery object and acknowledge of recovery must specify the same LUN that recovery was initiated on.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_FS_DELETE_OR_ACK_RECOVERY_INVALID_TLU
Language     = English
Acknowlege the deduplication LUN recovery using the same LUN that recovery was initiated on. Please contact your service provider.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attemped to set IsReplicationDestination property to false while the LUN was an active destination resource for replication.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_VU_IN_USE_AS_ACTIVE_REPLICATION_DESTINATION
Language     = English
The LUN cannot be barred from use as a replication destination because there is already an active replication session using it as its destination.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attemped to recover a LUN being used by an active replication session.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_RECOVER_ERROR_IN_USE_BY_REPLICATION
LANGUAGE     = ENGLISH
The LUN cannot be recovered bacause it is in use by an active replication session.
.

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to expand a LUN while it was in use by replication feature.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_EXPAND_ERROR_LUN_IN_USE_BY_REPLICATION
Language     = English
The LUN cannot be expanded because it is being used by replication.
.


;// --------------------------------------------------
;// Introduced In: PSI5
;//
;// Usage: Returned to Uni
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN would be used as replication destination, hence cannot be expanded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_EXPAND_ERROR_LUN_MARKED_AS_REPLICATION_DESTINATION
Language     = English
The LUN cannot be expanded because it is designated as a replication destination and cannot be written to or modified.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to expand a LUN while it was in use by replication feature.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_SHRINK_ERROR_LUN_IN_USE_BY_REPLICATION
Language     = English
The LUN cannot be shrunk because it is being used by replication.
.


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The LUN would be used as replication destination, hence cannot be shrunk.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_SHRINK_ERROR_LUN_MARKED_AS_REPLICATION_DESTINATION
Language     = English
The LUN cannot be shrunk because it is designated as a replication destination and cannot be written to or modified.
.
;// --------------------------------------------------
;// Introduced In: PSI6
;//
;// Usage: Event Log and Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: The maximum number of supported Lower Deck File Systems has been exceeded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_MAX_LDFS_COUNT_EXCEEDED
Language     = English
Unable to create the storage resource because the maximum number of Lower Deck File Systems already exist.
Could not create %2 because there are already %3 LDFSs on the array.
.


;// --------------------------------------------------
;// Introduced In: KH+SP1
;//
;// Usage: Event Log only
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to release the Device Id associated with the VU.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_VUMGMT_ERROR_RELEASE_DEVICE_ID_DURING_DESTROY
Language     = English
Failed to release the associated Device ID during a destroy operation.
.
;
;//=========================================
;//======= END VU MANAGEMENT ERROR MSGS =====
;//=========================================
;//

;//=========================================
;//======= START FILE MANAGEMENT ERROR MSGS =
;//=========================================
;//
;// File Management "Error" messages.
;//
;// --------------------------------------------------
;// INTRODUCED IN: R30
;//
;// Usage: Event Log and Returned To Navi
;//
;// Severity: Error
;//
;// Symptom of problem: The LUN creation encountered an error because CBFS file could not be created.
;//
MessageId    = 0x8E00
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_CREATE_FILE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while creating %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E00L == MLU_FILEMGMT_ERROR_COULD_NOT_CREATE_FILE);
;
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The file properties could not be obtained from cbfs.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_GET_FILE_PROPS
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while obtaining properties of %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The cbfs file could not be trucated.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_TRUNCATE_FILE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while truncating %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The cbfs file could not be deleted.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_DELETE_FILE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while deleting %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: New file size after expansion will be less than the current exported file size
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_EXP_SIZE_NOT_GREATER_THAN_EXPORTED_SIZE
Language     = English
Expansion LUN size must be greater than current LUN size.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: Operation cannot proceed due to the file object being in an error state. 
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_FILE_OBJECT_IN_ERROR_STATE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. File Object %2 is in an error state.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to shrink LUN to an unsupported size
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_INVALID_SHRUNK_SIZE
Language     = English
The new capacity specified is not supported for the shrink operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The cbfs file could not be expanded.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_EXPAND_FILE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while expanding %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to expand LUN to an unsupported size
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_EXP_SIZE_EXCEEDS_MAX_SIZE_ALLOWED
Language     = English
The new capacity specified is not supported for the expand operation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal information only
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to cancel an outstanding truncate operation with CBFS failed.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_FAILED_TO_CANCEL_TRUNCATE_OPERATION
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while cancelling truncate operation %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Internal
;//
;// Severity: Error
;//
;// Symptom of Problem: If the Compression Flag is set then the Actual File Size should be 
;//                     MLU_FILEOBJ_MAX_SIZE_MODE_FILE_SIZE_IN_SECTORS, else it should be
;//                     the Exported File Size (rounded up to CBFS Block Size).
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_FILE_SIZE_NOT_CONSISTENT_WITH_COMPRESSION_FLAG
Language     = English
Internal Information Only. %2  %3.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to expand a LUN while an add or remove compression operation was in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_EXPAND_NOT_ALLOWED_COMPRESSION_IN_PROGRESS
Language     = English
Cannot expand a LUN that has an add or remove compression operation in progress.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: User attempted to shrink a LUN while an add or remove compression operation was in progress.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_SHRINK_NOT_ALLOWED_COMPRESSION_IN_PROGRESS
Language     = English
Cannot shrink a LUN that has an add or remove compression operation in progress.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Returned if CBFS file mode conversion fails.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_CBFS_MODE_CONVERT_FAILED
Language     = English
An internal error occurred. Please contact your service provider.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_FILE_IS_BLACKED_OUT
Language     = English
The operation cannot be performed because the resource is 'Preparing'.  Wait for the resource's Current Operation to complete 'Preparing' and retry the operation.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Internal Information Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Returned if a cancel mapped request is made to a file 
;//                     which is already in mapped mode
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_MODE_CONVERSION_COMPLETED
Language     = English
Internal information only. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Logged if file promotion fails
;//                     
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_PROMOTE_FILE
Language     = English
An internal error occurred. Please gather SPcollects and contact your service provider.
Internal Information Only. Error %2 while promoting %3.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File creation encountered an error because CBFS File Id was not found
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_VERIFY_FILEID_EXISTENCE
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information Only. Error %2 occurred while creating %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E11L == MLU_FILEMGMT_ERROR_COULD_NOT_VERIFY_FILEID_EXISTENCE);

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Logged if Snap creation fails
;//                     
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_TOL_RECEIVED_DURING_SNAP_CREATION
Language     = English
An internal error occurred. Please retry the operation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E12L == MLU_FILEMGMT_TOL_RECEIVED_DURING_SNAP_CREATION);


;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Logged if precreate file fails
;//                     
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_CBFS_ERROR_PRECREATE_FILE
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E13L == MLU_FILEMGMT_CBFS_ERROR_PRECREATE_FILE);


;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Logged if preextend file fails
;//                     
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_CBFS_ERROR_PREEXTEND_FILE
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E14L == MLU_FILEMGMT_CBFS_ERROR_PREEXTEND_FILE);


;// Introduced In: R33
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Logged if an async pre-operation (precreate or preextend) fails.
;//                     
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_CBFS_ERROR_PREOPERATION_FILE
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E15L == MLU_FILEMGMT_CBFS_ERROR_PREOPERATION_FILE);

;// --------------------------------------------------
;// Introduced In: R33
;//
;// Usage: Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: File deletion encountered an error because we could not determine
;//			if the file had been created.
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_COULD_NOT_VERIFY_FILEID_WAS_CREATED_BEFORE_DELETION
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E16L == MLU_FILEMGMT_ERROR_COULD_NOT_VERIFY_FILEID_WAS_CREATED_BEFORE_DELETION);

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to precreate file failed because the lower deck 
;//                     file system is not mounted.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_CBFS_ERROR_PRECREATE_FILE_FS_NOT_MOUNTED
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E17L == MLU_FILEMGMT_CBFS_ERROR_PRECREATE_FILE_FS_NOT_MOUNTED);

;// --------------------------------------------------
;// Introduced In: KH+
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The call to precreate file failed because the peer SP is not up.
;//                     
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FILEMGMT_ERROR_PRECREATE_FILE_PEER_NOT_READY
Language     = English
An internal error occurred. Please contact your service provider.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8E18L == MLU_FILEMGMT_ERROR_PRECREATE_FILE_PEER_NOT_READY);

;//=========================================
;//======= END FILE MANAGEMENT ERROR MSGS ===
;//=========================================
;//

;//=========================================
;//======= START FAMILY MANAGEMENT ERROR MSGS =
;//=========================================

;//
;// Family Management "Error" messages.
;//
;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS returned an error on get_family_properties call
;//
MessageId    = 0x8F00
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FAMILYMGMT_ERROR_COULD_NOT_GET_FAMILY_PROPERTIES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information only: CBFS error %2 while getting family %3 props.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F00L == MLU_FAMILYMGMT_ERROR_COULD_NOT_GET_FAMILY_PROPERTIES);
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: CBFS returned an error on set_family_properties call
;//
MessageId    = 0x8F01
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FAMILYMGMT_ERROR_COULD_NOT_SET_FAMILY_PROPERTIES
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information only: CBFS error %2 while setting family %3 props.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F01L == MLU_FAMILYMGMT_ERROR_COULD_NOT_SET_FAMILY_PROPERTIES);
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: After recovery, insurance is insufficient to cover liability.
;//
MessageId    = 0x8F02
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_FAMILYMGMT_ERROR_CANT_COVER_LIABILITY_AFTER_RECOVERY
Language     = English
An internal error occurred. Please contact your service provider.
Internal Information only: After recovery, family liability exceeds insurance.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F02L == MLU_FAMILYMGMT_ERROR_CANT_COVER_LIABILITY_AFTER_RECOVERY);
;

;
;//============================================
;//======= END FAMILY MANAGEMENT ERROR MSGS ===
;//============================================
;//

;//=========================================
;//==== START LAG MANAGEMENT ERROR MSGS ====
;//=========================================

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid 'Member Count' was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F80
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_MEMBER_COUNT
Language     = English
The expected member count provided for the LUN Allocation Group is invalid.
An invalid expected member count was given (%2) as part of LUN Allocation Group creation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F80L == MLU_LAG_ERROR_INVALID_MEMBER_COUNT);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid 'Member LUN Type' was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F81
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_MEMBER_TYPE
Language     = English
The expected member type provided for the LUN Allocation Group is invalid.
An invalid expected member type was given (%2) as part of LUN Allocation Group creation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F81L == MLU_LAG_ERROR_INVALID_MEMBER_TYPE);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid 'Member LUN Capacity' was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F82
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_MEMBER_CAPACITY
Language     = English
The expected member capacity provided for the LUN Allocation Group is invalid.
An invalid expected member capacity was given (%2) as part of LUN Allocation Group creation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F82L == MLU_LAG_ERROR_INVALID_MEMBER_CAPACITY);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid 'Member LUN Max Offset' was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F83
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_MEMBER_OFFSET
Language     = English
The expected maximum member offset provided for the LUN Allocation Group is invalid.
An invalid expected expected maximum member offset was given (%2) as part of LUN Allocation Group creation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F83L == MLU_LAG_ERROR_INVALID_MEMBER_OFFSET);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An already in use name was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F84
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_NAME_IN_USE
Language     = English
Unable to create the LUN Allocation Group because the specified name is already in use. Please retry the operation using a unique name.
LAG %2 already exists with the name %3.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F84L == MLU_LAG_ERROR_NAME_IN_USE);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid LAG OID was specified while attempting
;//                     an operation.
;//
MessageId    = 0x8F85
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_OBJECT_ID
Language     = English
The given LUN Allocation Group Object ID is invalid.
An invalid LUN Allocation Group Object ID (%2) was specified.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F85L == MLU_LAG_ERROR_INVALID_OBJECT_ID);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified LAG could not be found.
;//
MessageId    = 0x8F86
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_OBJECT_NOT_FOUND
Language     = English
The given LUN Allocation Group Object was not found.
The given LUN Allocation Group Object (%2) was not found.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F86L == MLU_LAG_ERROR_OBJECT_NOT_FOUND);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A mismatch was found for the 'Member LUN Capacity' field
;//                     between the LUN creation input buffer and the LAG object.
;//
MessageId    = 0x8F87
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MISMATCH_IN_MEMBER_CAPACITY
Language     = English
The given LUN capacity value was not expected for the LUN Allocation Group.
The given LUN capacity (%2) value was not expected for the LUN Allocation Group (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F87L == MLU_LAG_ERROR_MISMATCH_IN_MEMBER_CAPACITY);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A mismatch was found for the 'Member LUN Offset' field
;//                     between the LUN creation input buffer and the LAG object.
;//
MessageId    = 0x8F88
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MISMATCH_IN_MEMBER_OFFSET
Language     = English
The given LUN offset value was not expected for the LUN Allocation Group.
The given LUN offset (%2) value was not expected for the LUN Allocation Group (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F88L == MLU_LAG_ERROR_MISMATCH_IN_MEMBER_OFFSET);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: A mismatch was found for the 'Member LUN Type' field
;//                     between the LUN creation input buffer and the LAG object.
;//
MessageId    = 0x8F89
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MISMATCH_IN_MEMBER_TYPE
Language     = English
The given LUN type was not expected for the LUN Allocation Group.
The given LUN type (%2) value was not expected for the LUN Allocation Group (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F89L == MLU_LAG_ERROR_MISMATCH_IN_MEMBER_TYPE);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The creation of a new LAG object would exceed the feature
;//                     limits.
;//
MessageId    = 0x8F8A
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_FEATURE_COUNT_EXCEEDED
Language     = English
The creation of a new LUN Allocation Group is not permitted as it would exceed the limits for the feature.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8AL == MLU_LAG_ERROR_FEATURE_COUNT_EXCEEDED);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: The creation of a new LAG member has specified a different
;//                     pool than was used for LAG object creation.
;//
MessageId    = 0x8F8B
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MISMATCH_IN_POOL_ID
Language     = English
The given Storage Pool was not expected for the LUN Allocation Group.
The given Storage Pool (%2) was not expected for the LUN Allocation Group (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8BL == MLU_LAG_ERROR_MISMATCH_IN_POOL_ID);

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An invalid 'Member Count' was specified while attempting
;//                     to create a new LAG object.
;//
MessageId    = 0x8F8C
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_ADD_MEMBER_WOULD_EXCEED_EXPECTED_MEMBER_COUNT
Language     = English
The addition of a LUN to the LUN Allocation Group would exceed its expected member count.
The addition of a LUN to the LUN Allocation Group (%2) would exceed its expected member count (%3).
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8CL == MLU_LAG_ERROR_ADD_MEMBER_WOULD_EXCEED_EXPECTED_MEMBER_COUNT);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to destroy a LAG with existing members is 
;//                     not allowed. 
;//                     
;//
MessageId    = 0x8F8D
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MEMBER_LUNS_EXIST
Language     = English
The operation can not be performed as the LUN Allocation Group members are still initializing.
The operation can not be performed as the LUN Allocation Group (%2) has member LUNs.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8DL == MLU_LAG_ERROR_MEMBER_LUNS_EXIST);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to create a LAG with an invalid member tier preference.
;//                     
;//
MessageId    = 0x8F8E
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_INVALID_MEMBER_TIER_PREFERENCE
Language     = English
The operation can not be performed as the LUN Allocation Group member tier preference is invalid.
The operation can not be performed as the LUN Allocation Group member tier preference (%2) is invalid.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8EL == MLU_LAG_ERROR_INVALID_MEMBER_TIER_PREFERENCE);
;

;// --------------------------------------------------
;// Introduced In: R35
;//
;// Usage: Event Log and Returned to Navi
;//
;// Severity: Error
;//
;// Symptom of Problem: An attempt to add a member LUN to a lag with a different tier preference.
;//                     
;//
MessageId    = 0x8F8F
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_LAG_ERROR_MISMATCH_IN_TIER_PREFERENCE
Language     = English
The operation can not be performed as the LUN Allocation Group member's tier preference does not match the group.
The operation can not be performed as the LUN Allocation Group member's tier preference (%2) does not match the group (%3). 
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12D8F8FL == MLU_LAG_ERROR_MISMATCH_IN_TIER_PREFERENCE);
;

;
;//============================================
;//======= END LAG MANAGEMENT ERROR MSGS ======
;//============================================
;//

;//============================
;//======= CRITICAL MSGS ======
;//============================
;//
;//

;//=========================================
;//======= START COMMON CRITICAL MSGS ======
;//=========================================
;//
;// Common "Critical" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = 0xC000
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_COMMON_BUGCHECK_SPIN_LOCK_TIME_EXCEEDED
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DC000L == MLU_COMMON_BUGCHECK_SPIN_LOCK_TIME_EXCEEDED);
;
;//=========================================
;//======= END COMMON CRITICAL MSGS ========
;//=========================================
;//

;//============================================
;//======= START EXECUTIVE CRITICAL MSGS ======
;//============================================
;//
;// Executive "Critical" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:
;//
;// Severity: Error
;//
;// Symptom of Problem: The MLU driver tried to shutdown CBFS on a K10 Attach Shutdown, but recevied
;//                     an error indicating that there were outstanding API calls or mounted file systems.
;//
MessageId    = 0xC100
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_CRITICAL_OUTSTANDING_OPS_TO_CBFS_DURING_SHUTDOWN
Language     = English
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DC100L == MLU_EXECUTIVE_CRITICAL_OUTSTANDING_OPS_TO_CBFS_DURING_SHUTDOWN);
;
;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Used for CBFS Critical messages
;//
MessageId    = +1
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_EXECUTIVE_CRITICAL_EVENT_FROM_CBFS
Language     = English
Requires immediate attention from user. CBFS Event: %2, Msg: %3.
Please gather SPcollects and contact your service provider.
.


;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DC101L == MLU_EXECUTIVE_CRITICAL_EVENT_FROM_CBFS);
;
;//============================================
;//======= END EXECUTIVE CRITICAL MSGS ========
;//============================================
;//

;//==============================================
;//======= START MLU MANAGER CRITICAL MSGS ======
;//==============================================
;//
;// MLU Manager "Critical" messages.
;//

;//==============================================
;//======= END MLU MANAGER CRITICAL MSGS ========
;//==============================================
;//

;//=============================================
;//======= START FS MANAGER CRITICAL MSGS ======
;//=============================================
;//
;// File System Manager "Critical" messages.
;//

;//=============================================
;//======= END FS MANAGER CRITICAL MSGS ========
;//=============================================
;//

;//===============================================
;//======= START POOL MANAGER CRITICAL MSGS ======
;//===============================================
;//
;// Pool Manager "Critical" messages.
;//
;// --------------------------------------------------
;// Introduced In: R28
;//
;// Usage:Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: The storage pool is about to run out of free space
;//
MessageId    = 0xC500
Severity     = Error
Facility     = Mlu
SymbolicName = MLU_POOLMGR_CRITICAL_STORAGE_POOL_FREE_SPACE_ALMOST_EXHAUSTED
Language     = English
The Storage Pool %2 has only %3 percent free space left.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DC500L == MLU_POOLMGR_CRITICAL_STORAGE_POOL_FREE_SPACE_ALMOST_EXHAUSTED);


;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: For User Log only
;//
;// Severity: Error
;//
;// Symptom of Problem: System Pool has gone offline
;//
MessageId    = +1
Severity     = Error
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_POOLMGR_CRITICAL_SYSTEM_POOL_OFFLINE
Language     = English
An internal system service (pool %2) is offline. Some system services may not be available.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DC501 == MLU_POOLMGR_CRITICAL_SYSTEM_POOL_OFFLINE);


;
;//===============================================
;//======= END POOL MANAGER CRITICAL MSGS ========
;//===============================================
;//

;//================================================
;//======= START SLICE MANAGER CRITICAL MSGS ======
;//================================================
;//
;// Slice Manager "Critical" messages. 0xC6xx
;//

;//================================================
;//======= END SLICE MANAGER CRITICAL MSGS ========
;//================================================
;//

;//=================================================
;//======= START OBJECT MANAGER CRITICAL MSGS ======
;//=================================================
;//
;// Object Manager "Critical" messages. 0xC7xx
;//

;//=================================================
;//======= END OBJECT MANAGER CRITICAL MSGS ========
;//=================================================
;//

;//=========================================================
;//======= START I/O COORDINATOR/MAPPER CRITICAL MSGS ======
;//=========================================================
;//
;// I/O Coordinator/Mapper "Critical" messages. 0xC8xx
;//

;//=========================================================
;//======= END I/O COORDINATOR/MAPPER CRITICAL MSGS ========
;//=========================================================
;//

;//================================================
;//======= START VAULT MANAGER CRITICAL MSGS ======
;//================================================
;//
;// Vault Manager "Critical" messages. 0xC9xx
;//

;//================================================
;//======= END VAULT MANAGER CRITICAL MSGS ========
;//================================================
;//

;//=================================================
;//======= START BUFFER MANAGER CRITICAL MSGS ======
;//=================================================
;//
;// Buffer Manager "Critical" messages. 0xCAxx
;//

;//=================================================
;//======= END BUFFER MANAGER CRITICAL MSGS ========
;//=================================================
;//

;//==========================================
;//======= START C-CLAMP CRITICAL MSGS ======
;//==========================================
;//
;// C-Clamp "Critical" messages. 0xCBxx
;//

;//==========================================
;//======= END C-CLAMP CRITICAL MSGS ========
;//==========================================
;//

;//==========================================
;//======= START MLU ADMIN CRITICAL MSGS ======
;//==========================================
;//
;// MLU ADMIN "Critical" messages. 0xCCxx
;//

;//==========================================
;//======= END MLU ADMIN CRITICAL MSGS ========
;//==========================================
;//

;//=========================================
;//======= START VU MANAGEMENT CRITICAL MSGS ===
;//=========================================
;//
;// VU Management "Critical" messages. 0xCDxx
;//
;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: User Log
;//
;// Severity: Error
;//
;// Symptom of Problem: System LU has gone offline
;//
MessageId    = 0xCD00
Severity     = Error
;//SSPG C4type=User
Facility     = Mlu
SymbolicName = MLU_VUMGMT_CRITICAL_SYSTEM_LU_OFFLINE
Language     = English
An internal system service required for metrics (LUN %2) is offline. System metrics are not available.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE12DCD00L == MLU_VUMGMT_CRITICAL_SYSTEM_LU_OFFLINE);
;

;//=========================================
;//======= END VU MANAGEMENT CRITICAL MSGS =====
;//=========================================
;//

;//=========================================
;//======= START FILE MANAGEMENT CRITICAL MSGS =
;//=========================================
;//
;// FILE Management "Critical" messages. 0xCExx
;//

;//=========================================
;//======= END FILE MANAGEMENT CRITICAL MSGS ===
;//=========================================
;//

;//======================================
;//======= START FAMILY MANAGEMENT CRITICAL MSGS =
;//======================================
;//
;// FAMILY Management "Critical" messages. 0xCFxx
;//


;//======================================
;//======= END FAMILY MANAGEMENT CRITICAL MSGS ===
;//======================================
;//

;//================================================
;//======= START COMPRESSION MANAGER CRITICAL MSGS =
;//================================================
;//
;// Compression Manager "Critical" messages. 0xD0xx
;//

;//==============================================
;//======= END COMPRESSION MANAGER CRITICAL MSGS =
;//==============================================
;//

;#undef MLU_MSG_ID_ASSERT
;
;#endif //__K10_MLU_MESSAGES__
