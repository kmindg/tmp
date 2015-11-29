// SimpUtil.h
//
// Copyright (C) 1997	Data General Corporation
//
//
// Simple, robust utilities.
// By simple I mean lightweight, easy to use and NO MFC!
//
//
//	Revision History
//	----------------
//	23 Sep 97	D. Zeryck	Initial version.
//	08 May 00	D. Zeryck	Add ListKeys
//	07 Jun 00	H. Weiner	Add ListValues
//	11 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	23 Feb 01	H. Weiner	Add CreateVolatileSubKey(), DeleteSubKey(), CreateSubKeyEx()
// 
#ifndef _simputil_h_
#define _simputil_h_


#include <ostream>
#include "CaptivePointer.h"
#include "SafeHandle.h"

//***************************************************************
//
// This code implements simple access to any Registry key.
//
//***************************************************************
//-----------------------------------Class Declarations
class SimpleReg {

	public:
	
	SimpleReg();
	~SimpleReg();

	//--------------
	//	open key on remote machine
	//
	HRESULT OpenKey( char *pKey, const void * hBaseKey, char *pMachine  );

	//--------------
	//	open key on local machine
	//
	HRESULT OpenKey( char *pKey, const void * hBaseKey );

	//--------------
	//	open subkey - parent already open
	//
	HRESULT OpenSubKey( char *pSubKey );

	//--------------
	//	open parent - parent already open
	//
	HRESULT OpenParentKey( );

	//--------------
	//	open sibling key - key already open
	//
	HRESULT OpenSiblingKey( char *pSiblingKey );

	//--------------
	//	Read REG_SZ value - Remote
	//
	HRESULT ReadValue( char *pKey, const void * hBaseKey, char *pValueName,
						    char **hValue, char *pMachine );
	//--------------
	//	Read REG_SZ value - Local
	//
	HRESULT ReadValue( char *pKey, const void * hBaseKey, char *pValueName,
						    char **hValue );
	//--------------
	//	Read REG_SZ value - Key is open
	//
	HRESULT ReadValue( char *pValueName, char **hValue );

	//--------------
	//	Read REG_unsigned long value - Remote
	//
	HRESULT ReadValue( char *pKey, const void * hBaseKey, char *pValueName,
						    LPDWORD pValue, char *pMachine );
	//--------------
	//	Read REG_unsigned long value - Local
	//
	HRESULT ReadValue( char *pKey, const void * hBaseKey, char *pValueName,
						    LPDWORD pValue );

	//--------------
	//	Read REG_unsigned long value - key is open
	//
	HRESULT ReadValue( char *pValueName, LPDWORD pValue );

	//--------------
	//	Read BINARY value - key is open
	//
	HRESULT ReadValue( char *pValueName, unsigned char *pValue,  long dataSize );

	//--------------
	//	create subkey - parent already open
	//
	HRESULT CreateSubKey( char *pSubKey );

	//--------------
	//	Set REG_unsigned long value - key is open
	//
	HRESULT WriteValue( char *pValueName, unsigned int Value );

	//--------------
	//	Set REG_SZ value - key is open
	//
	HRESULT WriteValue( char *pValueName, char *pValue, long dataSize, bool bMulti );

	//--------------
	//	Set binary value - key is open
	//
	HRESULT WriteValue( char *pValueName, unsigned char *pValue, long dataSize );

	//--------------
	// Get a list of the keys/values under the open key.
	// Fails if not initialized
	//
	HRESULT ListKeys( CCharPtrList &cpKeyList, DWORD &numSubkeys);
	HRESULT ListValues( CCharPtrList &cpKeyList, DWORD &numSubkeys);

	//--------------
	//	create subkey - parent already open
	//
	HRESULT CreateVolatileSubKey( char *pSubKey );
	
	//--------------
	//	delete subkey - parent already open
	//
	HRESULT DeleteSubKey( char *pSubKey );
	
	private:

	HRESULT ReadValueAlloc( char *pValueName, void **hData, DWORD * lpDataSize);
	HRESULT ReadValue( char *pValueName, void *pData, DWORD * lpDataSize);

	//---------------------
	// Make sure we don't leak by freeing resources before reassignment
	//
	HRESULT InitSelf( char *pKeyName, const void * hBaseKey);
	//--------------
	//	create subkey - parent already open -- with flag for volatile key
	//
	HRESULT CreateSubKeyEx( char *pSubKey, bool bIsVolatile);

	bool mbIsRemote;
	bool mbInited;
	const void *mhKey;
	const void *mhBaseKey;
	char *mpKeyName;
};



//***************************************************************
//
// This code implements simple endian-safe conversions of data.
//
//***************************************************************
//-----------------------------------Class Declarations
class SimpleConvert {

	public:
	
	SimpleConvert();
	~SimpleConvert(){}

	//--------------
	//	Convert long to modepage: put in Big-endian format
	//
	void	IntToModepage( unsigned char *pMP, DWORD dwValue);

	//--------------
	//	Convert short to modepage: put in Big-endian format
	//
	void	IntToModepage( unsigned char *pMP, unsigned short wValue);

	//--------------
	//	Convert modepage to ULONG: put in little-endian format
	//
	void	ModepageToInt( unsigned char *pMP, ULONG *pValue);

	//--------------
	//	Convert modepage to USHORT: put in little-endian format
	//
	void	ModepageToInt( unsigned char *pMP, unsigned short *pValue);

	//
	// Convert endianness of double DWORD
	//
	void	SwapEndianDwordLong( unsigned char *pDW );

	//
	// Convert endianness of DWORD
	//
	void	SwapEndianDword( unsigned char *pDW );

	//--------------
	//	Convert endianness of WORD
	//
	void	SwapEndianWord( unsigned char *pW );
};



class SafeIoctl {

public:
	SafeIoctl(){};
	~SafeIoctl(){};

	// returns: S_OK, E_FAIL (if sh not init) WAIT_TIMEOUT, or win32 error

	long DoSafeIoctl(SafeHandle &sh,  unsigned long dwIoControlCode,
		void * lpInBuffer,  unsigned long nInBufferSize,  
		void * lpOutBuffer,   unsigned long nOutBufferSize,   
			DWORD *lpBytesReturned);
};


#endif //_simputil_h_
