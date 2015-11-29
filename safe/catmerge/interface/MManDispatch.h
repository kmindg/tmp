// MManDispatch.h
//
// Copyright (C) 1999,2009	Data General Corporation
//
//
// TLD dependent part of include for MessageManager users
//
//	Revision History
//	----------------
//	21 Sep 99	H. Weiner	Initial version
//	14 Dec 99	D. Zeryck	pre-Navi mods
//	26 Sep 00	B. Yang		Task 1938
//	05 May 09	E. Carter	Add InvalidateManagementPortData( ) function.
//
#ifndef MMAN_DISPATCH_H
#define MMAN_DISPATCH_H

#include "k10defs.h"
#include "tld.hxx"
#include "CaptivePointer.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define MMANDISPAPI	      CSX_MOD_EXPORT
#define MMANDISPAPI_CLASS CSX_CLASS_EXPORT


//--------------------------------------------------------------------Typedefs
//
// This is used in the INTERNAL (Transact()) method of MManModepageExtended
// ONLY. navi will use TransactList().
//
// LUN-specific transactions are always preceded by an ID of the desired 
// LUN; we don't do that but specify the LUN. This is ignored in non-LUN-
// specific transactions
//
// Note: It is the responsibility
// of the MMan helper classes to insure that ALL THE DATA AVAILABLE IS SUPPLIED!
// The MMan helper classes will allocate all memory, as they exeute in the thread 
// context of the caller, using GlobalAlloc()
#define CDBSize 16
#define UserNameSize 40 // # of char in the localUser;

typedef struct _MmCdb {
	unsigned char 	CDB[16];	// contains the SCSI CDB
	unsigned long	lun;		// the lun that this command is meant for
	unsigned char	*pPage;		// the mode page address, NULL if no data
} MmCdb;

typedef struct _MmCdbMp {
	char localUser[40];
	unsigned char 	CDB[16];	// contains the SCSI CDB
	unsigned long	lun;		// the lun that this command is meant for
	unsigned char	*pPage;		// the mode page address, NULL if no data
} MmCdbMp;

#ifndef ALAMOSA_WINDOWS_ENV
typedef enum _ADMIN_CONNECT_DEST
{
	ADMIN_CONNECT_LOCAL,
	ADMIN_CONNECT_PEER,
	ADMIN_CONNECT_BOTH
} ADMIN_CONNECT_DEST;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

//-----------------------------------------------------------Forward Declarations

class MManAdminDispatchImpl;
class MManModepageExtendedImpl;
#ifndef ALAMOSA_WINDOWS_ENV
class AdminRemote;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

//-----------------------------------------------------------MManModepageExtended
//
//	Name: MManModepageExtended
//
//	Description: 
//		 THIS IS NAVI's ENTREE TO K10
//		Interface class for SCSI vendor-unique modepages,
//		AAS/AAQ, and K10 Standard Admin transactions via TLD list
//
class MMANDISPAPI_CLASS MManModepageExtended {

public:


	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManModepageExtended()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments: None
	//
	//	Returns: N/A
	//
	MManModepageExtended( ); 

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	~MManModepageExtended()
	//
	//	Description:
	//		Destructor
	//
	//	Arguments: N/A
	//
	//	Returns: N/A
	//
	~MManModepageExtended();

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	Transact()
	//
	//	Description:
	//		Scan modepage and determine recipient, connect and send.
	//		On read operations, allocs memory to pCDB.pPage. 
	//		Returns when complete or on error. It may throw a
	//		K10Exception object.
	//
	//	Arguments:
	//		pMmCDB		IN	pointer to MmCdb
	//						
	//	Returns:
	//		S_OK, (System and subsystem errors)
	//
	// Preconditions: See K10 Administrative nterface Design Spec.
	//
	long Transact( MmCdb *pMmCDB );

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	TransactList()
	//
	//	Description:
	//		Scan TLD dispatch data and determine recipient. Connect and send.
	//		On read operations, allocs TLD data and embeds to topmost object. 
	//		Returns when complete or on error. It may throw a
	//		K10Exception object.
	//
	//	Arguments:
	//		pTldList		IN	pointer to TLD class object
	//						
	//	Returns:
	//		S_OK, (System and subsystem errors)
	//
	// Preconditions: See K10 Administrative nterface Design Spec.
	//
	long TransactList( TLD *pTldList );
#ifndef ALAMOSA_WINDOWS_ENV
	long TransactList( TLD *pTldList, ADMIN_CONNECT_DEST SP );
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	InvalidateManagementPortData()
	//
	//	Description:
	//		Set the MManObjMgmtPortIPprop::mManagementPortData.bIsDataInitialized 
	//		flag to FALSE so this struct gets refilled with updated data
	//		before it is used the next time.
	//
	//	Arguments:
	//		None
	//						
	//	Returns:
	//		Nothing
	//
	static void InvalidateManagementPortData( );

	static void DestroyFbeData();


	private:
		MManModepageExtendedImpl	*mpImpl;
#ifndef ALAMOSA_WINDOWS_ENV
	    AdminRemote     *RemoteObject;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

};

// non-COM version of IK10Admin interface
class CSX_CLASS_EXPORT IK10Admin_Export  
{
public:
	IK10Admin_Export() {}
	virtual ~IK10Admin_Export() {}

	virtual long Write( long lDbField, long lDatumId, long lOpCode,  unsigned char *pIn, long lBufSize,
								  long *lpError ) = 0;
	virtual long WriteTldList(	TLD *pTldList, long *lpError ) = 0;
	virtual long Read( long lDbField, long lDatumId, long lOpCode,  unsigned char **ppIn, long *lpBufSize,
								long *lpError) = 0;
	virtual long ReadTldList(TLD *pTldList, TLD **ppTldReturned, long *lpError) = 0;
	virtual long Update( long lDbField, long lDatumId, long lOpCode,  unsigned char *pInOut, long 
								   lBufSize,  long *lpError) = 0;
};

typedef IK10Admin_Export *IK10Admin_ObjPtr;


//-----------------------------------------------------------MManAdminDispatch
//
//	Name: MManAdminDispatch
//
//	Description: 
//		Concrete class for dispatch to any arbitrary admin library via the
//		K10Admin interface.
//
//	Note: 
//		Depends on the name of the coclass registered under Classes to
//		be "ObjectName.ObjectName", with a CurVer key pointing to the current
//		version (e.g., "ObjectName.ObjectName.1").
//

class MMANDISPAPI_CLASS MManAdminDispatch {

public:

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManAdminDispatch()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments:
	//		pObjectName		IN	String name of object, e.g., "MyComponentAdmin"
	//		bDeleteUnloads	IN	Unload admin library from memory in destructor

	//	Returns:
	//		N/A
	//
	//	Preconditions: pObjectName is not null and is null-terminated
	//
	MManAdminDispatch( const char *pObjectName);

	MManAdminDispatch( const char *pObjectName, bool bDeleteUnloads );

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	~MManAdminDispatch()
	//
	//	Description: Destructor
	//
	~MManAdminDispatch();

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	Write()
	//
	//	Description:
	//		Writes buffer to admin library, and receives system and component
	//		errors.
	//
	//	Arguments:
	//		lDbField	IN		Specifier of which database type: component-specific
	//		lDatumId	IN		Speifier of datum to be worked on:  component-specific
	//		lOpCode		IN		Specifier of operation code for this db type
	//		pIn			IN		Pointer to data in Network Byte Order
	//		lBufSize	IN		Size of buffer in pIn
	//		lpError		OUT		Address of long to receive component error code
	//
	//	Returns:
	//		System errors, MMAN_ERROR_XXX (if MMan classes used by library)
	//
	//	Preconditions: pIn is not null,lBufSize is > 0, lpError not NULL
	//
	long Write( long lDbField, long lDatumId, long lOpCode,  unsigned char *pIn, long lBufSize,
								  long *lpError );
//-------------------------Public Method-----------------------------------//
//
//	Name:	WriteTldList()
//
//	Description:
//		Writes a TLD list to admin library, and receives system and component
//		errors.
//
//	Arguments:
//		pTldList	IN		TLD list to send
//		lpError		OUT		Address of long to receive component error code
//
//	Returns:
//		System errors, MMAN_ERROR_XXX (if MMan classes used by library)
//
//	Preconditions: pTldList is not null, lpError not NULL
//
	long WriteTldList(	TLD *pTldList, long *lpError );

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	Read()
	//
	//	Description:
	//		Receives buffer from admin library, which allocates it. Receives system
	//		and component errors.
	//
	//	Arguments:
	//		lDbField	IN		Specifier of which database type: component-specific
	//		lOpCode		IN		Specifier of operation code for this db type
	//		lDatumId	IN		Speifier of datum to be worked on:  component-specific
	//		ppOut		IN		Address of pointer to get allocated
	//		lpBufSize	IN		Address of long to receive size of buffer in pOut
	//		lpError		OUT		Address of long to receive component error code
	//
	//	Returns:
	//		System errors, MMAN_ERROR_XXX (if MMan classes used by library)
	//
	//	Preconditions: ppOut is null, *lpBufSize is zero, lpError not NULL
	//
	long Read( long lDbField, long lDatumId, long lOpCode,  unsigned char **ppIn, long *lpBufSize,
								long *lpError);
//-------------------------Public Method-----------------------------------//
//
//	Name:	ReadTldList()
//
//	Description:
//		Receives TLD list from admin library, which allocates it. Receives system
//		and component errors.
//
//	Arguments:
//		pTldList		IN		TLD list to send, determines what to send back
//		ppTldReturned	OUT		pointer to TLD list received
//		lpError			OUT		Address of long to receive component error code
//
//	Returns:
//		System errors, MMAN_ERROR_XXX (if MMan classes used by library)
//
//	Preconditions: pTldList is not null, **ppTldReturned is not null, *ppTldReturned is null,
//  lpError not NULL
//
long ReadTldList(TLD *pTldList, TLD **ppTldReturned, long *lpError);

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	Update()
	//
	//	Description:
	//		Provides buffer to admin library, which modifies it. This provides a
	//		way to do extensively paramaterized reads, or to return extensive
	//		return data on a write.
	//
	//	Arguments:
	//		lDbField	IN		Specifier of which database type: component-specific
	//		lDatumId	IN		Speifier of datum to be worked on:  component-specific
	//		lOpCode		IN		Specifier of operation code for this db type
	//		pInOut		IN/OUT	Pointer to data in Network Byte Order
	//		lBufSize	IN		Size of buffer in pIn
	//		lpError		OUT		Address of long to receive component error code
	//
	//	Returns:
	//		System errors, MMAN_ERROR_XXX (if MMan classes used by library)
	//
	//	Preconditions: pInOut is not null, lBufSize is nonzero, lpError not NULL
	//
	long Update( long lDbField, long lDatumId, long lOpCode,  unsigned char *pInOut, long 
								   lBufSize,  long *lpError);
private:

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	~MManAdminDispatch()
	//
	//	Description: 
	//		Constructor. Private to prevent uninited construction
	//
	MManAdminDispatch();

	MManAdminDispatchImpl *mpImpl;		// Pointer to implementation class
#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif // _MSC_VER
    CPtr <char> cpObjectName;
#ifdef _MSC_VER
#pragma warning(default:4251)
#endif
    size_t lNameLength;



 
};




#endif
