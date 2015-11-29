;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_MISC_UTIL_MESSAGES_H
;#define K10_MISC_UTIL_MESSAGES_H
;
;//
;//++
;// File:            K10MiscUtilMessages.h (MC)
;//
;// Description:     This file defines K10 Misc Util Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  K10_MISC_UTIL_xxx and K10_MISC_UTIL_ADMIN_xxx.
;//
;// History:         27-Mar-01       MWagner     Created
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( K10MiscUtil= 0x125 : FACILITY_K10_MISC_UTIL )

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
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_INFO_GENERIC
Language	= English
Generic K10_MISC_UTIL Information.
.

;
;#define K10_MISC_UTIL_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_INFO_GENERIC)
;


;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_WARNING_GENERIC
Language	= English
Generic K10_MISC_UTIL Warning.
.

;
;#define K10_MISC_UTIL_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_WARNING_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_WARNING_BITMAP_BIT_OUT_OF_RANGE
Language	= English
A Bitmap was asked to perform an operation on a Bit that is not in the Bitmap.
.

;
;#define K10_MISC_UTIL_ADMIN_WARNING_BITMAP_BIT_OUT_OF_RANGE                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_WARNING_BITMAP_BIT_OUT_OF_RANGE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_WARNING_MEMORY_WAREHOUSE_NO_MORE_PALLETS
Language	= English
The Warehouse is out of unrequisitioned Pallets.
.

;
;#define K10_MISC_UTIL_ADMIN_WARNING_MEMORY_WAREHOUSE_NO_MORE_PALLETS                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_WARNING_MEMORY_WAREHOUSE_NO_MORE_PALLETS)
;

;//-----------------------------------------------------------------------------
;//
;//  Introduced In: Release 22
;//
;//  Usage:         Internal
;// 
;//  Severity:      Warning
;//
;//  Symptom of Problem: Someone tried to Drain an IRP pool while IRPS were still
;//                      in use.
;//
;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_WARNING_IRP_POOL_DRAIN_IRPS_IN_USE
Language	= English
K10MiscUtilIrpPoolDrain() was called while there were IRPs in Use. 
.

;
;#define K10_MISC_UTIL_ADMIN_WARNING_IRP_POOL_DRAIN_IRPS_IN_USE                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_WARNING_IRP_POOL_DRAIN_IRPS_IN_USE)
;

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_GENERIC
Language	= English
Generic K10_MISC_UTIL Error Code.
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_UNALLOCATED_PALLET
Language	= English
K10 Misc Util Memory Warehouse is Freeing an Unallocated Pallet.
[1] PWarehouse
[2] Pallet
[3] __LINE__
[4] Bit Number
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_WAREHOUSE_FREEING_UNALLOCATED_PALLET         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_UNALLOCATED_PALLET)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_PALLET_OUT_OF_PURVIEW
Language	= English
K10 Misc Util Memory Warehouse is Returning a Pallet not in the Warehouse
[1] PWarehouse
[2] Pallet
[3] PWarehouse->Pallets
[4] PWarehouse->Pallets + (NumberOfPallets * PalletSize)
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_WAREHOUSE_FREEING_PALLET_OUT_OF_PURVIEW         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_PALLET_OUT_OF_PURVIEW)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_MISALIGNED_PALLET
Language	= English
K10 Misc Util Memory Warehouse is Returning a Pallet not at a Pallet boundary.
[1] PWarehouse
[2] Pallet
[3] Pallet Boundary
[4] Bit Number
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_WAREHOUSE_FREEING_MISALIGNED_PALLET         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_FREEING_MISALIGNED_PALLET)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_COULD_NOT_PROBE_AND_LOCK_PAGES
Language	= English
The Memory Sheriff could not probe and lock the PagedPool pages.
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_SHERIFF_COULD_NOT_PROBE_AND_LOCK_PAGES         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_COULD_NOT_PROBE_AND_LOCK_PAGES)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_GET_ADDRESS_INVALID_PHASE
Language	= English
The Memory Sherrif was asked to get the address from an invalid Warrant.
[1] PWarrant
[2] 
[3] __LINE__
[4] Invalid Phase
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_SHERIFF_GET_ADDRESS_INVALID_PHASE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_GET_ADDRESS_INVALID_PHASE)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_UNLOCK_AND_FREE_INVALID_PHASE
Language	= English
The Memory Sherrif was asked to unlock and free memory from an invalid Warrant.
[1] PWarrant
[2] 
[3] __LINE__
[4] Invalid Phase
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_SHERIFF_UNLOCK_AND_FREE_INVALID_PHASE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_SHERIFF_UNLOCK_AND_FREE_INVALID_PHASE)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_ALLOCATING_INVALID_SELF_ID
Language	= English
The Memory Warehouse is Requisitioning a Pallet, and found that the Pallet Self ID is wrong. 
[1] PWarehouse
[2] Pallet
[3] __LINE__
[4] Invalid Self Id
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_MEMORY_WAREHOUSE_ALLOCATING_INVALID_SELF_ID         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_MEMORY_WAREHOUSE_ALLOCATING_INVALID_SELF_ID)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_ERROR_EXIT_IRQL_NOT_EQUAL_TO_ENTRY_IRQL
Language	= English
A function is exiting with an IRQL different from its entrance IRQL.
[1] Entrance Irql
[2] Exit Irql
[3] __LINE__
[4] Current Thread
.
;
;#define K10_MISC_UTIL_ADMIN_ERROR_EXIT_IRQL_NOT_EQUAL_TO_ENTRY_IRQL         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_ERROR_EXIT_IRQL_NOT_EQUAL_TO_ENTRY_IRQL)
;

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= K10MiscUtil
SymbolicName	= K10_MISC_UTIL_CRITICAL_ERROR_GENERIC
Language	= English
Generic K10_MISC_UTIL Critical Error Code.
.

;
;#define K10_MISC_UTIL_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10_MISC_UTIL_CRITICAL_ERROR_GENERIC)
;
;
;
;#endif
;
;//++
;//.End K10MiscUtilMessages.h
;//--
