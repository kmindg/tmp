//**************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//
//	Revision History
//	----------------
//	?? ??? ??	B. Fiske	Initial version.
//	28 Jan 00	D. Zeryck	Redo for Navi spec.
//	03 Mar 00	D. Zeryck	KDirectLogTLD, new error methodology
//	05 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//
//*************************************************************************

#ifndef K10DIRLOG_H
#define K10DIRLOG_H

#include "K10Exception.h"
#include "K10AppConstants.h"
#include "tld.hxx"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//	This header defines the direct event log functionality

//	The following functions write log entries to the NT application log

int CSX_MOD_EXPORT KDirectLog(const char *pszEventLog, 
			   int iMessageNumber,			// used to load msg # in msg DLL, NOT the error
			   K10_LOG_SEVERITY iSeverity,
			   long lErrorCode);	// actual code of error, contains component error mask, etc.

int CSX_MOD_EXPORT KDirectLogText(const char *pszEventLog, 
				   int iMessageNumber, 
				   K10_LOG_SEVERITY iSeverity, 
				   long lErrorCode,
				   const char *pszText);

int CSX_MOD_EXPORT KDirectLogTextText(const char *pszEventLog, 
					   int iMessageNumber, 
					   K10_LOG_SEVERITY iSeverity, 
					   long lErrorCode,
					   const char *pszText1,
					   const char *pszText2);

int CSX_MOD_EXPORT KDirectLogTextTextECode(const char *pszEventLog, 
					   int iMessageNumber, 
					   K10_LOG_SEVERITY iSeverity, 
					   long lErrorCode,
					   long lErrorCode2,
					   const char *pszText1,
					   const char *pszText2);

int CSX_MOD_EXPORT KDirectLogTextTextECodeHide(const char *pszEventLog, 
				   int iMessageNumber, 
				   K10_LOG_SEVERITY iSeverity, 
				   long lErrorCode1,
				   long lErrorCode2,
				   long	lHowToHide,
				   const char *pszText1,
				   const char *pszText2);

int CSX_MOD_EXPORT KDirectLogValue(const char *pszEventLog, 
					int iMessageNumber, 
					K10_LOG_SEVERITY iSeverity, 
					long lErrorCode,
					int iValue);

// Note: ErrorCode is in the exception object
//
int CSX_MOD_EXPORT KDirectLogException(const char *pszEventLog, 
				   int iMessageNumber, 
				   K10_LOG_SEVERITY iSeverity, 
				   K10Exception &exp);

int CSX_MOD_EXPORT KDirectLogTLD( const char *pszEventLog, TLD * pErrorTop);

// Convert our severity enum to Windows Event Type
//
WORD CSX_MOD_EXPORT WinEventType(K10_LOG_SEVERITY Severity);

//	Return values

// Unexported utility functions
const char *DLogGetTypeName( unsigned long dwThisType );
bool GetStringBuffer( char **pszBuffer, const char **ppszStrings, int iSize );

enum kdir_rc
{
	KDL_RC_MIN = 0,
	KDL_OK = KDL_RC_MIN,
	KDL_BAD_FILE,
	KDL_BAD_MESSAGE_NUMBER,
	KDL_MISC_ERROR,
	KDL_RC_MAX = KDL_MISC_ERROR
};

#endif // K10DIRLOG_H
