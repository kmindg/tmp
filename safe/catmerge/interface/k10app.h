//**************************************************************************
// Copyright (C) Data General Corporation 1989-1997
// All rights reserved.
// Licensed material -- property of Data General Corporation
//*************************************************************************

#ifndef K10APP_H_
#define K10APP_H_

//	The K10APP class provides a framework for all the user space processes.
//	An instance of this class should be instantiated when the process initializes.

//	The class provides the following services:
//		Logging
//		Tracing
//		Registry access
//
//	12-22-99	HEW		Add llBigTime()
//	03 03 00	D. Zeryck	New log methodology
//	03 16 00	H. Weiner	Remove last CString usage.
//	14 Aug 00	H. Weiner	REQ 1632 - Pass through trace method that records version data
//	01 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//  01 Jan 02   B. Myers    67962 - Added K10TerminateProcess
//

#include "wtypes.h"
#include "K10AppConstants.h"
#include "K10RegistryUtil.h"
#include "K10Exception.h"
#include "tld.hxx"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//	void UpdateValues();

//------------------ Global Enums ---------------------------------------
// I put this here to simplify life.

enum reg_return_values				//	return values for these functions
{
	K10APP_REG_RETURNED_VALUES_MIN = 0,
	K10APP_OK = K10APP_REG_RETURNED_VALUES_MIN,
	K10APP_NOT_FOUND,
	K10APP_INVALID_VALUE_FORMAT,
	K10APP_SET_VALUE_FAILED,
	K10APP_UNKNOWN_ERROR,
	K10APP_INVALID_TYPE,
	K10APP_REG_RETURNED_VALUES_MAX = K10APP_INVALID_TYPE
};

//------------------ Forward Delcarations --------------------------------

class K10LogMsg;
class K10ProcCtrl;
class K10RegistryUtil;
class K10TraceMsg;

//------------------ Class Definitions ------------------------------------

class CSX_CLASS_EXPORT K10App
{
public:

	//	Constructors and Destructor
	K10App(const char *pszProcess, 
		   const char *pszCommandLine = NULL, 
		   bool bSubordinateProcess = true);
	~K10App();

	//---------------- Logging and tracing functions --------------------------------------

	//	The message written to the trace file is constructed from:

	// iSeverity	not quite redundant: if the accompanying error is not of the K10 format,
	//				substitute with appropriate generic error & put error in payload.
	//

	//		If an integer parm is supplied, it is converted to a string and processed like
	//			the text form of the functions.

	// These use generic message numbers. CRITICAL use ERROR as NT Event log has no critical.

	void K10Log(K10_LOG_SEVERITY iSeverity, long lError);
	void K10Log(K10_LOG_SEVERITY iSeverity,  K10Exception & exp);
	void K10Log(K10_LOG_SEVERITY iSeverity, long lError, const char *pszText);
	void K10Log(K10_LOG_SEVERITY iSeverity, long lError, int iParm);
	void K10Log(K10_LOG_SEVERITY iSeverity, long lError, const char *pszText, int iParm);
	//
	// For Navi
	//
	//	pErrorTop	Pointer to object with tag TAG_K10_ERROR_PACKET
	//
	void K10Log( TLD *pErrorTop );

	// These use specified message numbers
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity, long lError);
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity,  K10Exception & exp);
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity, long lError, const char *pszText);
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity, long lError, 
		const char *pszText, const char *pszText2);
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity, long lError, int iParm);
	void K10LogExp(int iMessageNumber, K10_LOG_SEVERITY iSeverity, long lError, const char *pszText, int iParm);

	//		If the trace level is < m_iActiveTraceLevel, the trace record is ignored.
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, K10Exception & exp);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, const char *pszParm);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, int iParm);
	void TraceVersion(const char * cFile, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4);
//	void K10UseVersionBuffer();
//	void K10UseDefaultBuffer();
	//---------------- End of Logging and Tracing support ---------------------------------
	
	//-----------------Function to notify Everest that an event occurred-------------------
	void K10NotifyEverest();

	//-----------------Function to access the current value of the trace level
	K10_TRACE_LEVEL GetTraceLevel();

	void SetTraceLevel(K10_TRACE_LEVEL iTraceLevel);

	//-----------------Function to dump the private data to the trace file-----------------
	void TracePrivateData();

	//-----------------Registry Access-----------------------------------------------------
	K10RegistryUtil *m_pRegApp;
	
	
	/////////////// Windows-only methods ///////////////
	
	//-----------------Static time counter access utility----------------------------------
	static __int64 llBigTime();	

	//-----------------Functions to allow setting and retrieving the executable root
	const char * GetCommandLine();
	
	//-----------------Functions to support event notification with parent process---------
	HANDLE GetTerminateHandle();
	void SetStartupComplete();
	
	//-----------------Provide a way to kill any process-----------------------------------
	static bool TerminateProcess( const char * ProcessName );

	//	Utility function to get the list of processes to be started.
	//	The function returns the number of processes found and stores the address of
	//	the process buffer in the caller's pointer.  If an error occurs obtaining the
	//	list, the buffer address is set to 0 and the function returns 0.
	int GetProcessList( char **ppszProcessList);
	
private:
	K10TraceMsg		*m_ptraceApp;
	K10LogMsg		*m_plogApp;
#ifdef ALAMOSA_WINDOWS_ENV	
	K10ProcCtrl		*m_pctrlApp;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */		
};



#endif
