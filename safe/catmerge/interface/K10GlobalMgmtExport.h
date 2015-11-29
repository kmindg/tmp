// K10GlobalMgmtExport.h
//
// Copyright (C) 1998 Data General Corporation
// Copyright (C) 2011 EMC Corp
//
//
// Header for K10GlobalMgmt exports
//
//	Revision History
//	----------------
//	24 Jul 98	D. Zeryck	Initial version
//	10 Sep 98	D. Zeryck	Moved typedefs into k10defs.h
//	11 Nov 98	D. Zeryck	Added byteswap object
//	 2 Dec 98	D. Zeryck	K10LayerDriverUtil
//	16 Dec 98	D. Zeryck	add bool bRegen to get generics list, "mapped" to "consumed"
//							K10_DVR_NAME defined as driver names, ownerId -> consumerId
//	25 Mar 99	D. Zeryck	Moved gloal mgmt private stuff to non-export header
//	31 Mar 99	D. Zeryck	Added K10PsmFile support
//	24 Jun 99	D. Zeryck	No more byte swap code for disk admin interface
//	23 Jul 99	D. Zeryck	btIsFitered to btIsLayered, add CheckStackOps
//	31 Aug 99	D. Zeryck	Add Get(Peer)SpId
//	21 Oct 99	D. Zeryck	Sanitize better for export
//	15 Nov 99	D. Zeryck	Add HRESULT GetArrayWWN( K10_WWN &wwn );
//	25 Jan 00	D. Zeryck	Fix up errors - add ispec
//	 5 Mar 00	H. Weiner	Add severity bits to error codes.
//	 8 Mar 00	D. Zeryck	Add error codes for simple reg
//	13 Apr 00	D. Zeryck	Add SP Resume getting
//	 2 Jun 00	D. Zeryck	Bug 942
//	 4 Aug 00	D. Zeryck	Bug 8715 (remedy)
//	22 Aug 00	D. Zeryck	Bug 1676
//	21 Aug 00	B. Yang		Task 1938	
//  03 Jan 03   J. Jin      Add interface to IDM
//  09 Jan 03   J. Jin      Add changeId method.
//  02 Jun 03   K. Keisling DIMS 87998 Overload PSM OpenWrite method to allow
//                          input of open data flag instead of defaulting to
//                          PSM_DATA_CLEAR.
//   7 Aug 03	E. Petsching	Add support for user enabled packages.
//	11 Dec 03	H. Weiner	Add GenerateArrayIscsiName().
//	13 Jul 05	R. Hicks	Add SetTargetWWN().
//	27 Dec 05	M. Brundage	DIMS 137265: Add "driver in stack multiple times" support
//	25 Jan 06	K. Boland	DIMS 137946: Added comment to K10PSMFile destructor indicating 
//							that PSM file is closed WITHOUT a commit
//	24 Jul 06	R. Hicks	DIMS 151240: Change K10_REVISION minor field to a ULONG
//	16 May 08	E. Carter	DIMS 196493: Add RegenerateDatabase( ) method.
//	23 Sep 08	R. Singh	Added Expand() function prototype in K10PsmFile interface class.
//	26 Dec 08	R. Singh	Added modified Expand() function prototype in K10PsmFile interface class.
//	05 Jan 09	D. Hesi		Keeping both versions of Expand() function changes
//	06 Jan 09	R. Hicks	DIMS 217632: Add K10GlobalNameUtil constructor to use
//							existing FlareData object
//         12 Jan 09   R. Singh    Removed WCHAR version Expand() function

//	12 Mar 09	R. Hicks	DIMS 221996 - Allow K10PsmFile clients to specify 
//							PSM file timeout.
//	14 Sep 09	R. Hicks	DIMS 237258: Add K10CommandProcessing class
//	17 Dec 09	R. Hicks	DIMS 237258: Moved K10CommandProcessing to K10CommandProcessing.h
//	01 Feb 10	R. Hicks	Add btHasCompositeData to K10DriverInfo
//	23 Jul 10	R. Hicks	Add deltaPollToken to K10DriverInfo
//	30 Aug 10	C. Hopkins	Added IsAutoInsert() routine to assist in supressing reporting of
//							Admin Auto Insert drivers in LUN objects
//	08 Nov 10	R. Hicks	ARS 390024: Add separate list for features which are not layered drivers
//	21 Mar 11	D. Hesi		ARS 408728: Updated GenerateTargetWWN() and SetTargetWWN() to include IDM Range

#ifndef _K10GlobalMgmtExport_h_
#define _K10GlobalMgmtExport_h_

//----------------------------------------------------------------------INCLUDES

#include "k10defs.h"

#ifndef CaptivePointer_h
#include "CaptivePointer.h"
#endif

#ifndef _K10GlobalMgmtErrors_h_
#include "K10GlobalMgmtErrors.h"
#endif

#include "user_generics.h"

#include "IdmInterface.h"
#include "psm.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//------------------------------------------------------------------------DEFINES

#define K10_LUNEXT_INFO_VERSION	1		// K10_LunExtendedInfo

// The StackOps value for layered drivers uses a string to hold DbId/opcode
// pairs. Each pair is delimited by the following characters (! is terminal)
#define K10_STACKOPS_TOKEN_SEPERATORS	";!"

// The StackOps value for layered drivers uses a string to hold DbId/opcode
// pairs. Each pair is separated by the following character
#define K10_STACKOPS_SUBTOKEN_SEPERATOR	':'

#define NDU_STATUS_DATA_AREA_NAME "ndu-status"

//-----------------------------------------------------------------------TYPEDEFS

// Used to get data from Flare about the SP & array
//
#define K10_CHASSIS_SER_NO_LEN 16

typedef csx_uchar_t	CHASSIS_SERIAL_NUMBER[K10_CHASSIS_SER_NO_LEN]; // NOT null-term!

// This structure is used by the Redirector to give info about an object
// btIsConsumed		- If true, 'owner' is consuming the object for IO
//					  it CANNOT be made available to higher driver/TCD
// btNoExtend	- a creating or consuming driver has marked this as non-extendable - do not
//				allow layering of this device
// btIsLayered	- at least one layered driver has this in its IO stack--
//					unavailable for Snap Cache, etc.
// exporterId		- Driver which is exporting this object
// creatorId		- Driver which created, and is responsible for storing nice name
// consumerId			- Driv er which has consumed this object
// object Name		- name the last driver in the IO stack gives the object
// comment			- 'nice' name
// luId			- GUID of object
// dwcreatorStamp	- a 32-bit value used by the creator

typedef struct K10_Lun_Extended_Info {
	unsigned short			version;
	unsigned char			btIsConsumed;
	unsigned char			btNoExtend;
	unsigned char			btIsLayered;
	unsigned char			reserved[3];
	K10_DVR_NAME			consumerId;
	K10_DVR_NAME			exporterId;
	K10_DVR_NAME			creatorId;
	K10_DVR_OBJECT_NAME		objectName;
	K10_LOGICAL_ID			comment;
	K10_LU_ID				luId;	
	unsigned long			dwCreatorStamp;

} K10_LunExtendedInfo; 

//
// This structure is used to return info on drivers installed on the K10.
//
// driverScmName -	null-term ASCII name used by SCM to ID the driver. It
//					is what shows up in "drivers" applet, and is used in 
//					the DriverOrder list in the Registry
// driverLibName -	null-term ASCII name used by IK10Admin system to load
//					the library's COM library.
// btIsInitiator -	If nonzero, this is a IO initiator and needs to get quiesced/
//					unquiesced along with TCD. Example: RemoteMirror.
// btAutoInsert  -	If nonzero, this is an auto insert driver
// btHasCompositeData -	If nonzero, this driver supplies composite feature data
// location			Which driver position (location) in stack this entry
//					refers to.  Location 0 is lowest legal position in device
//					stack, location 1 is next higher, etc.

typedef struct K10_Driver_Info {
	K10_DVR_SCM_NAME	driverScmName;
	K10_DVR_NAME		driverLibName;
	DWORD			driverErrorMask;
	csx_uchar_t		btIsInitiator;
	csx_uchar_t		btAutoInsert;
	csx_uchar_t		btHasCompositeData;
	csx_uchar_t		bUserDisabled;
	csx_uchar_t		location;
	ULONGLONG		deltaPollToken;
	K10_DVR_EXCL_LIST	driverExclList;
} K10DriverInfo;

typedef struct _K10_RESUME
{
    CHAR	wwn[8];
    CHAR	manufacturing_locale[16];
    CHAR	manufacturing_date[10];
    CHAR	vendor_name[20];
    CHAR	serial_number[12];
    CHAR	revision[8];
    CHAR	part_number[12];

} K10_RESUME, *PK10_RESUME;

#define K10_REV_CHAR_MISSING		0xFF
#define K10_REV_LONG_MISSING		0xFFFFFFFF
typedef struct _K10_REVISION {
	UCHAR	major;
	ULONG	minor;
	ULONG	pass;
	ULONG	hardware_characteristics;
	UCHAR	software_options;
	UCHAR	distribution_type;	
} K10_REVISION;
//------------------------------------------------------------------------CLASSES

// This class is used to access, and hold, the Device Map. It is multithread
// friendly, and uses a static structure so only 1 instance is necessary.

// Note on return values and exceptions:
//	This class will use the long return values to pass back errors which are
//	received via GetLastError() from Win32 functions. For errors pertaining to
//	class semantics (file size wrong, for example), this class will throw a 
//	K10Exception.
//
class K10DeviceMapImpl;

class CSX_CLASS_EXPORT K10DeviceMap
{
public:

	K10DeviceMap();

	// calls ReleaseDeviceMap to release locks
	//
	~K10DeviceMap();

	// gets state of one object
	//
	long GetObjectState( K10_LU_ID & objectId, K10_LunExtendedInfo & extInfo );

private:
	K10DeviceMapImpl	*pImpl;
};



//--------------------------------------------------------------K10GlobalNameUtil
class K10GlobalNameUtilImpl;

class CSX_CLASS_EXPORT K10GlobalNameUtil  
{
public:

	K10GlobalNameUtil();
	// can optionally provide existing FlareData object, cast to void
	K10GlobalNameUtil(void *pFD);
	~K10GlobalNameUtil();
	
	// For LUs and VirtualArrays
	//Generate a single Type 6 (SIX) WWN using the K10_WWID struct as a container
	//
	//
	// pId		IN/OUT		Points to a K10_LU_ID structure; will be
	//						zerod & initialized with valid WWN
	//
	HRESULT GenerateTargetWWN( K10_LU_ID *pId );

	HRESULT GenerateTargetWWN(K10_LU_ID *pId, DWORD &lunNumber, IDMRANGE idmRange);

	HRESULT SetTargetWWN(K10_WWID * pId, DWORD &lunNumber, IDMRANGE idmRange);

	// For Ports
	// Generate a pair of Type 5 (FIVE) WWNs using the K10_WWID struct as a container
	//
	//
	// spId		IN			ID of this SP, retrieved form GetSpId
	//
	// portId	IN			ID of this port; unique to the SP only (e.g.,
	//						it is the port value obtained from TCD, NOT from
	//						the storage centric interface)
	//						MUST BE > 0 and < 7!!
	//
	// pId		IN/OUT		Points to a K10_LU_ID structure; will be
	//						zeroed & initialized with valid WWN
	//
	HRESULT GeneratePortWWN( char spId, UCHAR portId, K10_WWID *pId );

	// Generate the single Type 6 (SIX) WWN using the K10_WWID struct as a container
	//
	// NOTE: this is NOT GetArrayWWN! GetArrayWWN is used to retrieve from CMI.
	//		This fn reads the Flare database to determine what it should be.
	//
	HRESULT GenerateArrayWWN( K10_WWN *pWwn );

	HRESULT GetChassisSerialNumber( CHASSIS_SERIAL_NUMBER &serNo );

    HRESULT GetWWNSeed( UINT_32E *pWwnSeed );
    
	void GetSpSignatures( ULONG &lSig, ULONG &lPeerSig );

	// Throws
	void GetSpId( char &cId );

	// Throws
	void GetPeerSpId( char &cId );

	//Get from CMI the single Type 6 (SIX) WWN using the K10_WWID struct as a container
	//
	// Throws
	void GetArrayWWN( K10_WWN *pWwn );

	//
	// Get Software Rev info. Throws
	//
	void GetSoftwareRevision( K10_REVISION &Revision);

	// Check the SSN. 
	//
	bool ValidateSystemSSN( const char* pSSN );

    // Get the HA mode for x1-lite system 
    //
    HRESULT X1LEnableHA(BOOL &bEnable);

	// Generate the iSCSI name string for the array. 
	// All iSCSI ports on all SPs will use it.
	HRESULT GenerateArrayIscsiName( char *iSCSIname );

	// If we need per port iSCSI names, use this
	HRESULT GeneratePortIscsiName( char spId, UCHAR portId, char *name );

private:
	K10GlobalNameUtilImpl *mpImpl;
};



//---------------------------------------------------------------K10LayerDriverUtil
//
// Use this class if your component needs to know the driver ordering and the
// drivers installed on this K10
//
class CSX_CLASS_EXPORT K10LayerDriverUtil  
{
public:
	K10LayerDriverUtil();
	~K10LayerDriverUtil();

	// Get the ordered list of all known drivers
	//
	void GetDriverOrder( CCharPtrList& cplOrder, DWORD *pdwDriverOrderCount);

	// Get the ordered list of installed drivers, and of their libraries.
	//
	void GetInstalledDrivers(CListPtr<K10DriverInfo>& cplDrivers, DWORD * pdwDriverCount);

	void GetInstalledDrivers(CListPtr<K10DriverInfo>** ppcplDrivers, DWORD * pdwDriverCount);

	void GetInstalledDrivers(CListPtr<K10DriverInfo>** ppcplDrivers, DWORD * pdwDriverCount,
		CListPtr<K10DriverInfo>** ppcplFeatures, DWORD * pdwFeatureCount);

	// Determine which driver (candidate vs test) "is before" the other.
	// The tuple <driver name, location> identifies a unique position in 
	// the device stack.
	// Returns true if candidate is before test (lower in device stack).
	// Returns false if candidate is after test (higher in device stack).
	// Throws if neither is found.
	//
	bool IsBefore(	K10_DVR_NAME& driverCandidate, 
					unsigned char candidateLocation,
					K10_DVR_NAME& driverTest, 
					unsigned char testLocation );

	// Verify that the location value specified is legal for driver
	//
	bool ValidLocation( K10_DVR_NAME& driverName, unsigned char location);

	bool IsAutoInsert(K10_DVR_NAME& driverName);



private:

	void InitInternalCache(void);

	NPtr < CListPtr<K10DriverInfo> > mnpcplDrivers;

	NPtr < CListPtr<K10DriverInfo> > mnpcplFeatures;
};


//---------------------------------------------------------------K10PsmFileImpl

//  Possible PSM file types.

typedef enum
{
    psm_NoFile,     //  The file wasn't found.
    psm_Unknown,    //  The file type cannot be determined.
    psm_UserPsm,    //  The file does not have a K10PsmFile header.
    psm_K10PsmFile, //  The file has a K10PsmFile header.
} psm_filetype;

class K10PsmFileImpl;

class CSX_CLASS_EXPORT K10PsmFile  
{
public:
	K10PsmFile();
	//
	// NOTE: The destructor closes the PSM file WITHOUT committing changes.
	//
	~K10PsmFile();
	//
	// bCommit: set to false if you do not want changes committed on close
	//
	void Close(bool bCommit = true);
	//
	// Read from beginning of PSM file.  Must call OpenRead first.
	// Throws
	//
	void Read( void *pBuffer, DWORD dwBytesToRead, DWORD *pBytesRead);

    //
    // Read the header for a file, but no data just yet.  Subsequent calls
    // to ReadTail will get the data.
    // Throws
    //
    DWORD ReadHeader( );

    //
    // Read up to one chunk of data to the current offset.
    // Throws
    //
    void ReadTail( void *pBuffer, DWORD dwBytesToRead, DWORD *pBytesRead );

	//
	// Write from beginning of PSM file. Must call OpenWrite first.
	// Throws
	//
	void Write( void *pBuffer, DWORD dwBufSize, DWORD *pBytesWritten);

    //
    // Write the header for a file, but no data just yet.  Subsequent calls
    // to WriteAppend will fill in the data.
    // Throws
    //
    void WriteHeader( DWORD dwBufSize );

    //
    // Write up to one chunk of data to the current offset.
    // Throws
    //
    void WriteAppend( void *pBuffer, DWORD dwBufSize, DWORD *pBytesWritten );

    //
    // Copies a file from disk into psm.
    //
    HRESULT PutFile( const char *pDataArea, const char *pFilename );

    //
    // Specifies if this is a write once data area.
    //

    HRESULT PutFile( const char *pDataArea, const char *pFilename, bool bWriteOnce  );

    //
    // Specifies if this data area should be added with a header in PSM. DIMS# 149387
    //

    HRESULT PutFile( const char *pDataArea, const char *pFilename, bool bWriteOnce, bool bAddPSMHeader );

    //
    // Copies a file from psm to disk.
    //
    HRESULT GetFile( const char *pDataArea, const char *pFilename );
    HRESULT GetRawFile( const char *pDataArea, const char *pFilename );
    //  Use "Unknown" when the file may or may not have a K10PsmFile header.
    HRESULT GetUnknownFile( const char *pDataArea, const char *pFilename );

    //
    // Retrieves the contents of the data area to memory.  The caller must free
    // the buffer allocated by this function.
    //
    HRESULT GetContents( const char *pDataArea, void **pBuffer, DWORD &Size );
    HRESULT GetRawContents( const char *pDataArea, void **pBuffer, DWORD &Size );
    //  Use "Unknown" when the contents may or may not have a K10PsmFile header.
    HRESULT GetUnknownContents( const char *pDataArea, void **pBuffer, DWORD &Size );
    void    FreeBuffer( void *pBuffer );

	HRESULT OpenRead( const char *pFilename );
	HRESULT OpenRead( const char *pFilename, DWORD dwTimeoutInMinutes );

	// defaults PSM open DataFlag to PSM_DATA_CLEAR - see PSM_OPEN_IN_BUFFER
	HRESULT OpenWrite( const char *pFilename, DWORD dwMaxSize );

	// allows user spec of PSM open DataFlag - see PSM_OPEN_IN_BUFFER
	HRESULT OpenWrite( const char *pFilename, DWORD dwMaxSize, UCHAR psmDataFlag);
	HRESULT OpenWrite( const char *pFilename, DWORD dwMaxSize, UCHAR psmDataFlag,
		DWORD dwTimeoutInMinutes );

	DWORD GetSize();

    psm_filetype GetType( const char *pDataArea );

	//
	// Remove a data area. May be recreated w/OpenWrite()
	//
	HRESULT Remove( const char *pFilename );
	HRESULT Expand( const char *pBuffer, DWORD dwNewSize );

private:
	
	K10PsmFile & operator=( CONST K10PsmFile &K10PsmFile );
	NPtr <K10PsmFileImpl> mpImpl;
};

//--------------------------------------------------------------K10IDMUtil
class K10IDMUtilImpl;

class CSX_CLASS_EXPORT K10IDMUtil  
{
public:
    K10IDMUtil();
    ~K10IDMUtil();

    HRESULT AssignID(K10_LU_ID wwnId, DWORD &lunId, IDMRANGE range = IDM_LOW);
    HRESULT AssignID(K10_LU_ID wwnId, csx_u64_t &lunId, IDMRANGE range = IDM_LOW);
    HRESULT SetID(K10_LU_ID wwnId, DWORD lunId);
    HRESULT ReleaseID(K10_LU_ID wwnId);
    DWORD LookUpByWWN(K10_LU_ID wwnId);
    HRESULT GetWWNTable(csx_uchar_t **ppIdmTable, long *lpBufSize);
    HRESULT DeleteTable();
    DWORD ChangeID(K10_LU_ID wwnId, IDMRANGE range = IDM_LOW);
    HRESULT RegenerateDatabase( );

protected:
    HRESULT DoIoctl(long lIoctlCommand, PVOID pInputBuffer, long lInputBufferLen,
                    PVOID pOutputBuffer, long lOutputBufferLen);

private:
    K10IDMUtilImpl *mpImpl;
};

//--------------------------------------------------------------K10CommandProcessing

enum K10_COMMAND_PROCESSING_COMMAND_TYPE {
	K10_COMMAND_PROCESSING_COMMAND_TYPE_NONE = 0,
	K10_COMMAND_PROCESSING_COMMAND_TYPE_NEEDS_CPU = 1,
	K10_COMMAND_PROCESSING_COMMAND_TYPE_PROCESSING_COMPLETED = 2,
	K10_COMMAND_PROCESSING_COMMAND_TYPE_RESET_CPU_THROTTLE = 3,
	K10_COMMAND_PROCESSING_COMMAND_TYPE_MAX = K10_COMMAND_PROCESSING_COMMAND_TYPE_RESET_CPU_THROTTLE
};

class K10CommandProcessingImpl;

#endif
