/*******************************************************************************
 * Copyright (C) EMC Corporation, 1999 - 2005
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ClassDataStorage.h
 *
 * This header file defines externally usable structures and prototypes for 
 * the Class Data Storage (CDS) library.
 *
 ******************************************************************************/

#ifndef _CLASSDATASTORAGE_H_
#define _CLASSDATASTORAGE_H_

#include "EmcPAL.h"

// CDS_DISTRIB_SEM contains objects needed by DLS/DLU.

typedef struct _CDS_DISTRIB_SEM
{

	DLU_SEMAPHORE	DistributedSemaphore;
	DLS_LOCK_NAME	DLockName;

} CDS_DISTRIB_SEM, *PCDS_DISTRIB_SEM;


/*************************************************************************************************
 *	CdsStart()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is called to initialize the CDS library.  It is called once
 *				by the driver that wishes to utilize any functions within this library.
 *				If it succeeds, it should not be called again.
 *
 * PARAMETERS:	ConnectName is a character string that identifies the caller.  It is used
 *				in subsequent debug print statements.
 *
 *				DebugLevel specifies the level of debug statements to be printed.
 *
 *				PsmFilename points to a wide-char string that contains the PSM file name.
 *
 *				FileSize specifies the size of the PSM file (in bytes).
 *
 *				Insignia specifies a value to be written in the header of the PSM file.
 *
 *				RWBuffer points to a non-paged buffer that CDS will use to read/write the PSM file.
 *
 *				BufferAndMaxBucketSize specifies the size of the RWBuffer (and the maximum size of 
 *				the largest data bucket).
 *
 *				pPalClient specifies the caller's EMC PAL client.  The caller must supply a pointer
 *				to a registered PAL client.
 *
 * RETURNS:		STATUS_SUCCESS, STATUS_INSUFFICIENT_RESOURCES, or other NTSTATUS.
 *
 * NOTES:		Calling this function more than once (per driver) will cause a panic.
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CdsStart(PUCHAR, ULONG, csx_pchar_t, ULONG, ULONG, PVOID, ULONG, PEMCPAL_CLIENT);

#define CDS_CONNECT_NAME_SIZE	8

#define CDS_DBG_NONE	0
#define CDS_DBG_LOW		1
#define CDS_DBG_MED		2
#define CDS_DBG_HI		3

/*************************************************************************************************
 *	CdsGetDebugLevel()
 *************************************************************************************************
 *
 * DESCRIPTION:	Returns the current debug value for CDS.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Current debug value.
 *
 *************************************************************************************************/

extern ULONG CdsGetDebugLevel(VOID);

/*************************************************************************************************
 *	CdsSetDebugLevel()
 *************************************************************************************************
 *
 * DESCRIPTION:	Sets the current debug value for CDS.
 *
 * PARAMETERS:	DebugValue.
 *
 * RETURNS:		Nothing.
 *
 *************************************************************************************************/

extern VOID CdsSetDebugLevel(UCHAR);

/*************************************************************************************************
 *	CdsTransactionStart()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is called to begin a CDS transaction.  At a minimum, this results in
 *				the acquisition of the CDS distributed lock.  If specified to do so, this function
 *				will also read the PSM file header and the specified set of data buckets.  The data
 *				buckets are copied into individual buffers. These buffers are pointed to by a "memory 
 *				ledger."  A memory ledger (and associated data buckets) are only used within CDS, and
 *				since CDS executes at passive level, the Memory Ledgers and associated Data Buckets 
 *				are allocated from paged pool.
 *
 * PARAMETERS:	BucketNumber specifies which data bucket should be read.  Valid values is any single
 *				bucket number, CDS_ALL_BUCKETS, or CDS_NO_BUCKETS.
 *
 *				CurrentTransactionNumber points to a location where the Transaction Number (read from
 *				the PSM file header) will be copied.  If the PSM file can't be read or appears 
 *				uninitialize (insignia doesn't match), the Transaction Number will be zero.  If no 
 *				buckets are to be read, the Transaction number will be 1.
 *
 * RETURNS:		STATUS_INSUFFICIENT_RESOURCES or other NTSTATUS codes will be returned if the transaction 
 *				cannot be started.  STATUS_NO_SUCH_FILE will be returned if the specified file does not exist.  
 *				This should not be interpretted as an error (transaction did start).  STATUS_SUCCESS otherwise.
 *
 * NOTES:		CdsStart() must have completed successfully prior to calling this function.
 *				Only one CDS transaction can transpire at a time.  Calling this function
 *				prior to calling CdsStart() or prior to a previous transaction being 
 *				completed will cause a panic.
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CdsTransactionStart(ULONG, PULONG);

#define CDS_ALL_BUCKETS					0xEEEE
#define CDS_NO_BUCKETS					0xFFFF

/*************************************************************************************************
 *	CdsForceInitialTransactionNumber()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function allows the caller to force an initial value for the "Transactions"
 *				and was added to support the case where a new CDS file is being created to 
 *				replace a different/obsolete one, and the new file needs to inherit the transaction
 *				count from the old one.
 *
 * PARAMETERS:	InitialTransactionNumber specifies the initial value for the "Transactions."
 *
 * RETURNS:		Nothing.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The current "Transactions" must be 0 when this function is called.
 *
 *************************************************************************************************/

extern VOID CdsForceInitialTransactionNumber(ULONG);

/*************************************************************************************************
 *	CdsGetClassBucketVersion()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function returns the version of the specified data bucket.
 *
 * PARAMETERS:	ClassBucketNumber specifies which data bucket we are to return the version of.
 *
 * RETURNS:		The version of the specified data bucket, or zero if the bucket doesn't exist.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket whose version is being requested must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *************************************************************************************************/

extern ULONG CdsGetClassBucketVersion(ULONG);

/*************************************************************************************************
 *	CdsGetClassBucketOffset()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function returns the PSM file offset of the specified data bucket.
 *
 * PARAMETERS:	ClassBucketNumber specifies which data bucket we are to return the offset for.
 *
 * RETURNS:		The file offset of the specified data bucket, or zero if the bucket doesn't exist.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket whose offset is being requested must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *************************************************************************************************/

extern ULONG CdsGetClassBucketOffset(ULONG);

/*************************************************************************************************
 *	CdsSetClassBucketVersion()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is called to set the version of the specified data bucket.
 *
 * PARAMETERS:	ClassBucketNumber specifies which data bucket we are to set the version for.
 *
 *				Version is the new version of the specified bucket.
 *
 * RETURNS:		STATUS_SUCCESS.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket whose version is being set must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CdsSetClassBucketVersion(ULONG, ULONG);

/*************************************************************************************************
 *	CdsGetClassBucketSize()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function returns the size of the specified data bucket.
 *
 * PARAMETERS:	ClassBucketNumber specifies which data bucket we are to return the size of.
 *
 * RETURNS:		The size of the specified data bucket, or 0 if the data bucket does not exist.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket whose version is being requested must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *************************************************************************************************/

extern ULONG CdsGetClassBucketSize(ULONG);

/*************************************************************************************************
 *	CdsReadClassBucket()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function copies the specified data bucket into the supplied buffer.
 *
 * PARAMETERS:	ClassBucketNumber specifies which data bucket we are to copy from.
 *
 *				Buffer points to the destination data buffer.
 *
 *				Size specifies the size of the supplied data buffer.
 *
 * RETURNS:		The number of bytes copied.  This will be zero if the data bucket doesn't exist.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket whose version is being requested must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *************************************************************************************************/

extern ULONG CdsReadClassBucket(ULONG, PUCHAR, ULONG);

/*************************************************************************************************
 *	CdsGetWriteBuffer()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function returns a pointer to CDS' PSM write buffer.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Our PSM write buffer pointer.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *
 *************************************************************************************************/

extern PVOID CdsGetWriteBuffer(VOID);

/*************************************************************************************************
 *	CdsWriteClassBucket()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function will copy the supplied data to the specified data bucket.
 *
 * PARAMETERS:	BucketNo specifies which data bucket we are to copy the data to.
 *
 *				Buffer points to the source data buffer.
 *
 *				Size is the size of the supplied data buffer.
 *
 *				Offset is the sector offset where this data belongs in the PSM file.
 *
 * RETURNS:		STATUS_INSUFFICIENT_RESOURCES is returned if we need to allocate/expand the data 
 *				bucket but can't get the memory to so do.  Otherwise STATUS_SUCCESS will be returned.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *				The bucket being written must be within the current set of buckets.
 *				The current set of buckets is specified when calling CdsTransactionStart().
 *
 *				If the "Size" argument is 0, the data bucket will be deallocated.
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CdsWriteClassBucket(ULONG, PVOID, ULONG, ULONG);

/*************************************************************************************************
 *	CdsCommit()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function has the contents of the memory ledger written out to PSM.
 *
 * PARAMETERS:	NewTransactionNumber points to a location where the "new" transaction number
 *				is to be copied.  If the function can successfully store the memory ledger
 *				out in the PSM file, this Transaction number will be one greater than it was
 *				prior to this function being called.
 *
 * RETURNS:		Bad NTSTATUS from calls to the PSM driver.  Otherwise STATUS_SUCCESS is returned.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CdsCommit(PULONG);

/*************************************************************************************************
 *	CdsTransactionFinish()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function completes a previously started transaction.  The memory ledger
 *				and all associated data buckets are deallocated.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Nothing.
 *
 * NOTES:		CdsTransactionStart() must have completed successfully prior to calling this function.
 *
 *************************************************************************************************/

extern VOID CdsTransactionFinish(VOID);

/*************************************************************************************************
 *	CdsEnd()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is the complement of CdsStart().  It is called when a driver 
 *				will no longer require the services of CDS.  This function is likely to be
 *				called by the drivers Unload function.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Nothing.
 *
 * NOTES:		CdsStart() must have completed successfully prior to calling this function.
 *				This function is not to be called while there is an active transaction.
 *
 *************************************************************************************************/

extern VOID CdsEnd(VOID);

/*************************************************************************************************
 *	CDSCreateDistributedSemaphore()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is called to create (and open) a distributed semaphore.
 *
 * PARAMETERS:	SemPtr points to a caller-allocated CDS_DISTRIB_SEM.
 *
 *				SemName points to a character string, which is used to name the semaphore.
 *				This string must be between 2 and 8 characters.
 *
 * RETURNS:		STATUS_INVALID_PARAMETER or other NTSTATUS codes will be returned if the 
 *				initialization fails.  Otherwise STATUS_SUCCESS is returned.
 *
 * NOTES:		Distributed semaphores are provided by the DLU driver.  DLU utilizes the 
 *				DLS driver to provide this simple semaphore "adstraction."  A distributed 
 *				semaphore can be obtained, thus causing a thread on the other SP to pend if
 *				it tries to obtain the same "distributed" semaphore.  This semaphore can 
 *				not be used to serialize threads on the same SP.
 *
 *				If this function returns good status, it should not be called again (DLS/DLU will panic).
 *
 *************************************************************************************************/

extern EMCPAL_STATUS CDSCreateDistributedSemaphore(PCDS_DISTRIB_SEM, PUCHAR);

/*************************************************************************************************
 *	CDSDestroyDistributedSemaphore()
 *************************************************************************************************
 *
 * DESCRIPTION:	This function is called to close a distributed semaphore.
 *
 * PARAMETERS:	SemPtr points to the CDS_DISTRIB_SEM which CDSCreateDistributedSemaphore() initialized.
 *
 * RETURNS:		Nothing.
 *
 *************************************************************************************************/

extern VOID CDSDestroyDistributedSemaphore(PCDS_DISTRIB_SEM);

// Macros to get and release the CDS distributed semaphore.

#define CDS_GET_SEM(x)		\
DistLockUtilWaitForSemaphore(&((PCDS_DISTRIB_SEM)(x))->DistributedSemaphore, 0)

#define CDS_RELEASE_SEM(x)	\
DistLockUtilReleaseSemaphore(&((PCDS_DISTRIB_SEM)(x))->DistributedSemaphore, 0)


#endif //_CLASSDATASTORAGE_H_
