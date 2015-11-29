/*******************************************************************************
 * Copyright (C) EMC Corporation, 1999 - 2009
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * cds_private.h
 *
 * This header file defines internally usable structures and prototypes for 
 * the Class Data Storage (CDS) library.
 *
 ******************************************************************************/

#define TRC_COMP_CDS
#include "trc_api.h"

#ifndef _CDS_PRIVATE_H_
#define _CDS_PRIVATE_H_

#define CDS_SECTOR_SIZE	512

// There is one CDS_BUCKET_INFO for every CDS data bucket.

typedef struct _CDS_BUCKET_INFO
{
	ULONG	Version;
	ULONG	Length;
	ULONG	BucketOffset;
	PUCHAR	BucketPtr;

} CDS_BUCKET_INFO, *PCDS_BUCKET_INFO;

// A CDS_MEMORY_LEDGER is used to keep track of all data buckets.

typedef struct _CDS_MEMORY_LEDGER
{

	ULONG				Transactions;
	ULONG				BucketCount;
	CDS_BUCKET_INFO		BucketInfo[1];

} CDS_MEMORY_LEDGER, *PCDS_MEMORY_LEDGER;

// The macro CDS_LEDGER_SIZE() determines the size of a memory ledger for a specific number of data buckets.

#define CDS_LEDGER_SIZE(BucketCount)	\
	(sizeof(CDS_MEMORY_LEDGER) - sizeof(CDS_BUCKET_INFO) +	(BucketCount) * sizeof(CDS_BUCKET_INFO))

// All known CDS library revisions.

#define CDS_FIRST_LIB_REV	0x00000001
#define CDS_SECOND_LIB_REV	0x00000002
#define CDS_CURRENT_LIB_REV	0x00000003

// Free build print macros

#define FTRC(args)			TRCAPX_print args
#define FTRC1(args)			TRCAPX_printf1 args
#define FTRC2(args)			TRCAPX_printf2 args
#define FTRC3(args)			TRCAPX_printf3 args
#define FTRC4(args)			TRCAPX_printf4 args

// Debug print macros

#if 	DBG

#define TRC(args)			TRCAPX_print args
#define TRC1(args)			TRCAPX_printf1 args
#define TRC2(args)			TRCAPX_printf2 args
#define TRC3(args)			TRCAPX_printf3 args
#define TRC4(args)			TRCAPX_printf4 args

#else

#define TRC(args)				
#define TRC1(args)				
#define TRC2(args)				
#define TRC3(args)				
#define TRC4(args)				

#endif

// CdsX() enter and non-error exit, and one-time events.

#define LOW_TRC(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_LOW) { TRCAPX_print args; }
#define LOW_TRC1(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_LOW) { TRCAPX_printf1 args; }
#define LOW_TRC2(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_LOW) { TRCAPX_printf2 args; }
#define LOW_TRC3(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_LOW) { TRCAPX_printf3 args; }
#define LOW_TRC4(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_LOW) { TRCAPX_printf4 args; }

// Support functions.

#define MED_TRC(args)	\
if(CdsGlobal.DebugLevel >= CDS_DBG_MED) { TRCAPX_print args; }
#define MED_TRC1(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_MED) { TRCAPX_printf1 args; }
#define MED_TRC2(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_MED) { TRCAPX_printf2 args; }
#define MED_TRC3(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_MED) { TRCAPX_printf3 args; }
#define MED_TRC4(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_MED) { TRCAPX_printf4 args; }

// Gory details.

#define HI_TRC(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_HI) { TRCAPX_print args; }
#define HI_TRC1(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_HI) { TRCAPX_printf1 args; }
#define HI_TRC2(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_HI) { TRCAPX_printf2 args; }
#define HI_TRC3(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_HI) { TRCAPX_printf3 args; }
#define HI_TRC4(args)	\
	if(CdsGlobal.DebugLevel >= CDS_DBG_HI) { TRCAPX_printf4 args; }

// Macros to obtain and release the CDS library lock.

#define CDS_DRIVER_LOCK()								\
	EmcpalSemaphoreWait(&CdsGlobal.TransactionSem, EMCPAL_TIMEOUT_INFINITE_WAIT)

#define CDS_DRIVER_UNLOCK()	\
	EmcpalSemaphoreSignal(&CdsGlobal.TransactionSem, 1);

#define CDS_XACTION_END()	{				\
	CDS_DRIVER_LOCK();						\
	CdsGlobal.TransactionStarted = FALSE;	\
	CDS_DRIVER_UNLOCK();					\
}

// There is one CDS_FILE_INFO per data bucket.

typedef struct _CDS_FILE_INFO
{

	ULONG	Version;
	ULONG	Length;
	ULONG	Reserved;
	ULONG	DataOffset;

} CDS_FILE_INFO, *PCDS_FILE_INFO;

// The CDS_FILE_HEADER is the header of a PSM file being stored or
// retrieved by CDS.

typedef struct _CDS_FILE_HEADER
{

	ULONG			PsmToolFileType;
	ULONG			PsmToolFileSize;
	ULONG			DriverInsignia;
	ULONG			LibraryRevision;
	ULONG			Reserved[4];
	ULONG			TransactionNumber;
	ULONG			NumberOfElements;
	CDS_FILE_INFO	ElementInfo[1];

} CDS_FILE_HEADER, *PCDS_FILE_HEADER;

// The macro CDS_FILE_HEADER_SIZE() determines the size of a file header for a specific number of data buckets.

#define CDS_FILE_HEADER_SIZE(BucketCount)		\
	(sizeof(CDS_FILE_HEADER) - sizeof(CDS_FILE_INFO) +	(BucketCount) * sizeof(CDS_FILE_INFO))

#define MAX_ELEMENTS_IN_CDS_FILE		61
#define CDS_TOTAL_HEADER_SPACE_IN_FILE	(512 * 2)
#define CDS_MAX_IRP_STACK_SIZE_FOR_PSM	4

/******************************************************************************
 * In lieu of an NT Message Catalog file CDS, we will define a set of
 * error codes to be used as bugcheck codes to perform FastFail within
 * this library.
 *
 *  FastFail bugcheck codes are 32 bit values laid out as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +---+-+-+-----------------------+-------------------------------+
 *  |Sev|C|R|     Facility          |               Code            |
 *  +---+-+-+-----------------------+-------------------------------+
 *
 *      Sev: The severity code.  Must be 11 (Error) for our bugcheck codes.
 *      C: The Customer code flag.  Must be set for our bugcheck codes.
 *      R: A reserved bit.  Must be 0 for our bugcheck codes.
 *      Facility:  The facility code.  Unique value assigned to each component.
 *      Code: Status code.  Bugcheck codes use the range 0x8000-0xBFFF.
 *
 *****************************************************************************/

#define FACILITY_CDS	0x120

#define CDS_FF(Code)	(((0xE000 | FACILITY_CDS) << 16) | (Code))

#define CDSFF_START_ALREADY_CALLED					CDS_FF(0x8000)
#define CDSFF_XACTION_START_CDS_NOT_STARTED			CDS_FF(0x8001)
#define CDSFF_XACTION_START_XACTION_ACTIVE			CDS_FF(0x8002)
#define CDSFF_GET_VERSION_CDS_NOT_STARTED			CDS_FF(0x8003)
#define CDSFF_GET_VERSION_XACTION_NOT_STARTED		CDS_FF(0x8004)
#define CDSFF_SET_VERSION_CDS_NOT_STARTED			CDS_FF(0x8005)
#define CDSFF_SET_VERSION_XACTION_NOT_STARTED		CDS_FF(0x8006)
#define CDSFF_GET_SIZE_CDS_NOT_STARTED				CDS_FF(0x8007)
#define CDSFF_GET_SIZE_XACTION_NOT_STARTED			CDS_FF(0x8008)
#define CDSFF_READ_CDS_NOT_STARTED					CDS_FF(0x8009)
#define CDSFF_READ_XACTION_NOT_STARTED				CDS_FF(0x800A)
#define CDSFF_WRITE_CDS_NOT_STARTED					CDS_FF(0x800B)
#define CDSFF_WRITE_XACTION_NOT_STARTED				CDS_FF(0x800C)
#define CDSFF_COMMIT_CDS_NOT_STARTED				CDS_FF(0x800D)
#define CDSFF_COMMIT_XACTION_NOT_STARTED			CDS_FF(0x800E)
#define CDSFF_XACTION_FINISH_CDS_NOT_STARTED		CDS_FF(0x800F)
#define CDSFF_XACTION_FINISH_XACTION_NOT_STARTED	CDS_FF(0x8010)
#define CDSFF_CDS_END_CDS_NOT_STARTED				CDS_FF(0x8011)
#define CDSFF_CDS_END_XACTION_ACTIVE				CDS_FF(0x8012)
#define CDSFF_TRANSACTION_OVERLAP					CDS_FF(0x8013)	// Obsolete
#define CDSFF_NO_ACTIVE_TRANSACTION					CDS_FF(0x8014)	// Obsolete
#define CDSFF_ACTIVE_TRANSACTION					CDS_FF(0x8015)	// Obsolete
#define CDSFF_BAD_READ_SIZE							CDS_FF(0x8016)	// Obsolete
#define CDSFF_XACTION_START_BAD_INSIGNIA			CDS_FF(0x8017)	// Obsolete
#define CDSFF_WRITE_INCOMPLETE_WRITE				CDS_FF(0x8019)	// Obsolete
#define CDSFF_INAPPROPRIATE_USE_OF_SERVICE			CDS_FF(0x801A)
#define CDSFF_ILLEGAL_BUCKET_NUMBER					CDS_FF(0x801B)
#define CDSFF_CDS_CALLED_BUT_NOT_STARTED			CDS_FF(0x801C)
#define CDSFF_BAD_WRITE_BUCKET_PARAMETERS			CDS_FF(0x801D)
#define CDSFF_MISSING_BUCKET_DURING_WRITE			CDS_FF(0x801E)
#define CDSFF_UNABLE_TO_RELEASE_DLOCK				CDS_FF(0x801F)
#define CDSFF_DLOCK_NAME_TOO_BIG					CDS_FF(0x8020)
#define CDSFF_BUCKET_LOADED_BUT_SHOULD_NOT_BE		CDS_FF(0x8021)
#define CDSFF_BUCKET_LOADED_WITH_BAD_PARAMS			CDS_FF(0x8022)
#define CDSFF_BUCKET_EXCEEDS_DRIVER_LIMIT			CDS_FF(0x8023)
#define CDSFF_LEDGER_ALREADY_EXISTS					CDS_FF(0x8024)
#define CDSFF_GET_OFFSET_CDS_NOT_STARTED			CDS_FF(0x8025)
#define CDSFF_GET_OFFSET_XACTION_NOT_STARTED		CDS_FF(0x8026)

#endif //_CDS_PRIVATE_H_
