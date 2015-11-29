//K10RegistryUtil.h
//**************************************************************************
// Copyright (C) Data General Corporation 1989-1997
// All rights reserved.
// Licensed material -- property of Data General Corporation
//
//
//	10 Jan 00	D. Zeryck	Get rid of SP ID from logging.
//	20 Jan 00	D. Zeryck	Remove reg fns from K10App, CString removal
//	31 Jan 00	D. Zeryck	USe K10_TRACE_LEVEL
//	21 Feb 00	H. Weiner	Provide functions without the "mirrored" arg
//     18 May 00       D. Zeryck       CLARIION-->EMC
//
//*************************************************************************

#ifndef K10REG_UTIL_H
#define K10REG_UTIL_H

//------------------ Includes -----------------------------------------------------

#include "K10AppConstants.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//------------------ Fwd Defines --------------------------------------------------
class K10RegistryUtilImpl;

class CSX_CLASS_EXPORT K10RegistryUtil
{
public:

	//	Constructors and Destructor
	//	The bMasterProcess is true only for a process that does not 
	K10RegistryUtil(const char *pszProcess );
	~K10RegistryUtil();

	//---------------- Registry support ---------------------------------------------------
	//	The registry access functions access HKEY_LOCAL_MACHINE using a key or the	
	//	following structure
	//		Software\EMC\prefix\\component
	//	The prefix defaults to K10 but may be spedified by placing the value as 
	//	K10Root=value as a REG_SZ in Software\EMC.	The component subkey defaults to 
	//	the process name specified in the constructor for the K10App object, but may be 
	//	altered by the SetRegistryKey function.
	
	//	These functions support the registry formats REG_SZ and REG_DWORD.	
	//	Conversions from string to int are performed if required.	Following are
	//	the format rules (returns K10APP_OK unless otherwise specified):

	//	Function	Value	Registry key	Action
	//		get		string	none			returns K10APP_NOT_FOUND
	//		get		string	REG_SZ			returns string
	//		get		string	REG_DWORD		converts DWORD to string and returns string
	//		get		int		none			returns K10APP_NOT_FOUND
	//		get		int		REG_SZ			if value not convertible to int, returns 
	//										K10APP_INVALID_FORMAT, otherwise converts 
	//										string to int and returns int
	//		get		int		REG_DWORD		returns integer value
	
	//		set		string	none			creates key of type REG_SZ and sets the value
	//		set		string	REG_SZ			sets the value
	//		set		string	REG_DWORD		if value not convertible to int, returns
	//										K10APP_INVALID_FORMAT
	//		set		int		none			create key of type REG_DWORD, set the value
	//		set		int		REG_SZ			convert the integer to string, set the value
	//		set		int		REG_DWORD		sets the value

	//	set & get	any		not REG_SZ or	returns K10APP_INVALID_TYPE
	//						REG_DWORD

	//	Functions to set and retrieve the	prefix and component key values or registry
	//	object
	const char * szGetRegistryKey();


	void SetRegistryKey(const char *pszRegistryKey);

	//	Functions to set and retrieve values for specific keys

///////// New versions start here /////////////////////
	int	GetRegistryValue(const char *pszKey,
						char **ppResult);			//	where to place result - rel w/GLobalFree

	int	GetRegistryValue(const char *pszKey, 
						int *piValue);

	int	GetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						char **ppResult);			//	where to place result - rel w/GLobalFree

	int	GetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						int *piValue);				//	where to place result

	int	GetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						__int64 *pllValue);		//	where to place result

	int	SetRegistryValue(const char *pszKey, 
						const char *pszValue);

	int	SetRegistryValue(const char *pszKey, 
						int iValue);

	int	SetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						const char *pszResult);		//	value to set

	int	SetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						int iValue);				//	value to set

	int	SetRegistryValue(const char *pszRootKey,		//	directory, overrides default
						const char *pszKey,			//	key of entry
						__int64 llValue);			//	value to set

	K10_TRACE_LEVEL GetTraceLevel();

	void GetLogFile(char **ppEventLog);			// Free with GlobalFree

	const char * GetDirectory();

	int CreateRegistryKey(const char *pszFullDir,	//	Full directory
								 const char *pszSubDir);	//	Subdirectory to be added


private:
	K10RegistryUtilImpl *mpImpl;
};

#endif
