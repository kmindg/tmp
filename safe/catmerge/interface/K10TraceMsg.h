//**************************************************************************
// Copyright (C) EMC Corporation 1989-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//
//	01-10-00 DMZ	Remove SPID from log interfaces
//	01-10-00 HEW	Add s_szDefaultTraceFile.  Here because only needed by this .cpp
//	01-28-00 DMZ	Add trace exception, make use of K10_TRACE_LEVEL
//	03-15-00 HEW	Fix  s_szDefaultTraceFile
//	06-27-00 HEW	Eliminate use of CString class. Remove unused methods and defines.
//	07-13-00 HEW	Add methods to select the ktrace buffer.
//	08-14-00 HEW	TT 1632 Add TraceVersion() method- records version data
//	09-21-00 BY		Task 1938 (Pragma clean-up)
//	06-10-08 EDC	Added GetProcess( ) method.
//	10-23-09 REH	Changes for Coverity defects.

//*************************************************************************

#ifndef K10TRACEMSG_H
#define K10TRACEMSG_H

//	The class provides the following services:	Tracing

#include "wtypes.h"
#include "K10Exception.h"
#include "K10AppConstants.h"
#include "ktrace.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include "EmcUTIL_Device.h"

#ifdef _MSC_VER
#endif

#define MAX_PROC_NAME			16			// Max proc address name

class CSX_CLASS_EXPORT K10TraceMsg
{
public:

	//	Constructors and Destructor
	K10TraceMsg(const char *pszProcess);
	~K10TraceMsg();

	//	The message written to the trace file is constructed from:
	//		If an integer parm is supplied, it is converted to a string and processed like
	//			the text form of the functions.

	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, const char *pszParm);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, int iParm);
#ifndef ALAMOSA_WINDOWS_ENV
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, unsigned int iParm);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, long iParm);
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, unsigned long iParm);
#ifdef _WIN64
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, ULONG_PTR iParm);
#else /* _WIN64 */
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, const char *pszText, ULONG64 iParm);
#endif /* _WIN64 */
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - PCPC */
	void K10Trace(K10_TRACE_LEVEL iTraceLevel, K10Exception &exp);
	void TraceVersion(const char * cFile, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4);

	//  Miscellaneous functions

#ifdef _DEBUG
#define K10DBGTRACETEXT(level, text) K10Trace(level, text);
#define K10DBGTRACEPARM(level, text, parm) K10Trace(level, text, parm);
#else
#define K10DBGTRACETEXT(level, text)
#define K10DBGTRACEPARM(level, text, parm)
#endif

	
	//-----------------Function to access the current value of the trace level
	K10_TRACE_LEVEL GetTraceLevel() {return m_iTraceLevel;}
	void SetTraceLevel(K10_TRACE_LEVEL iTraceLevel) {m_iTraceLevel = iTraceLevel;}
	TCHAR* GetProcess( ) { return m_tcProcess; };

private:
	TCHAR	m_tcProcess[MAX_PROC_NAME];	// Process name, or as much as we have room for
	bool	m_bInitialized;				//	true if object successfully initialized
    EMCUTIL_RDEVICE_REFERENCE	m_hKtraceDev;				//  HANDLE to ktrace device
	K10_TRACE_LEVEL		m_iTraceLevel;				//	Current trace level
    EMCUTIL_STATUS                  status;
//	KTRACE_ring_id_T	m_iTraceBuffer;	// Which buffer stores messages

	//	Internal functions
	void TraceMessage(const char * cMessage);
	void TraceToBuffer(const char *cTemp, int iBuf, bool bRealData, DWORD dw1, DWORD dw2, 
		DWORD dw3, DWORD dw4);
    void TraceToBuffer( int iBuf, DWORD index, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4);

	EMCUTIL_RDEVICE_REFERENCE K10TraceInitDev();				// Return handle to trace device
};

#endif




